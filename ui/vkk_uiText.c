/*
 * Copyright (c) 2015 Jeff Boody
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "vkk"
#include "../../libcc/math/cc_mat4f.h"
#include "../../libcc/math/cc_vec4f.h"
#include "../../libcc/cc_memory.h"
#include "../../libcc/cc_log.h"
#include "../../libcc/cc_timestamp.h"
#include "../vkk_platform.h"
#include "../vkk_ui.h"

/***********************************************************
* private                                                  *
***********************************************************/

static int
vkk_uiText_resize(vkk_uiText_t* self, size_t size)
{
	ASSERT(self);

	vkk_uiWidget_t* widget = &self->base;
	vkk_uiScreen_t* screen = widget->screen;

	if(self->size >= size)
	{
		return 1;
	}

	// round to the next largest size
	size_t req_size = 16;
	if(size > 256)
	{
		LOGW("invalid size=%i", (int) size);
		return 0;
	}
	else if(size > 128)
	{
		req_size = 256;
	}
	else if(size > 64)
	{
		req_size = 128;
	}
	else if(size > 32)
	{
		req_size = 64;
	}
	else if(size > 16)
	{
		req_size = 32;
	}

	// check if vb_xyuv is valid
	// 2 triangles, 3 vertices, 4 components => 24
	size_t xyuv_size = 24*req_size*sizeof(float);
	self->vb_xyuv = vkk_uiScreen_textVb(screen, xyuv_size,
	                                    self->vb_xyuv);
	if(self->vb_xyuv == NULL)
	{
		return 0;
	}

	char* string = (char*) REALLOC(self->string, req_size);
	if(string == NULL)
	{
		LOGE("REALLOC failed");
		return 0;
	}
	string[size - 1] = '\0';
	self->string = string;

	// allocate size for string and cursor character
	// which is stored in place of the null character
	// 2 triangles, 3 vertices, 4 components => 24
	float* xyuv = (float*) REALLOC(self->xyuv, xyuv_size);
	if(xyuv == NULL)
	{
		LOGE("REALLOC failed");
		return 0;
	}
	self->xyuv = xyuv;

	// resize was successful
	self->size = req_size;

	return 1;
}

static void
vkk_uiText_size(vkk_uiWidget_t* widget, float* w, float* h)
{
	ASSERT(widget);
	ASSERT(w);
	ASSERT(h);

	vkk_uiText_t*      self  = (vkk_uiText_t*) widget;
	vkk_uiTextStyle_t* style = &self->style;

	float size = vkk_uiScreen_layoutText(widget->screen,
	                                     style->size);
	if(widget->layout.wrapx == VKK_UI_WIDGET_WRAP_SHRINK)
	{
		float width  = (float) 0.0f;
		float height = (float) vkk_uiText_height(self);
		if(vkk_uiWidget_hasFocus(widget))
		{
			width = (float) vkk_uiText_width(self, 1);
		}
		else
		{
			width = (float) vkk_uiText_width(self, 0);
		}
		*w = size*(width/height);
	}
	*h = size;
}

static void
vkk_uiText_click(vkk_uiWidget_t* widget,
                 float x, float y)
{
	ASSERT(widget);

	vkk_uiText_t*   self   = (vkk_uiText_t*) widget;
	vkk_uiScreen_t* screen = widget->screen;

	// acquire focus for input_fn
	vkk_engine_platformCmd(screen->engine,
	                       VKK_PLATFORM_CMD_SOFTKEY_SHOW);
	vkk_uiScreen_focus(screen, widget);

	// optionally call the base click_fn
	vkk_uiWidgetClick_fn click_fn = self->click_fn;
	if(click_fn)
	{
		(*click_fn)(widget, x, y);
	}
}

static void vkk_uiText_draw(vkk_uiWidget_t* widget)
{
	ASSERT(widget);

	vkk_uiText_t*      self   = (vkk_uiText_t*) widget;
	vkk_uiTextStyle_t* style  = &self->style;
	vkk_uiScreen_t*    screen = widget->screen;

	int len = strlen(self->string);
	if(vkk_uiWidget_hasFocus(widget))
	{
		// add the cursor
		double period = 1.0;
		double t      = cc_timestamp();
		if(fmod(t, period) < 0.5*period)
		{
			++len;
		}
	}
	else if(len == 0)
	{
		return;
	}

	cc_vec4f_t* c = &style->color;
	if(self->vb_xyuv && (c->a > 0.0f))
	{
		float w    = 0.0f;
		float h    = 0.0f;
		float x    = widget->rect_draw.l;
		float y    = widget->rect_draw.t;
		float size = widget->rect_draw.h;
		vkk_uiScreen_sizef(screen, &w, &h);

		cc_mat4f_t mvp;
		cc_mat4f_orthoVK(&mvp, 1, 0.0f, w, h, 0.0f, 0.0f, 2.0f);
		cc_mat4f_translate(&mvp, 0, x, y, 0.0f);
		cc_mat4f_scale(&mvp, 0, size, size, 1.0f);
		vkk_renderer_updateBuffer(screen->renderer,
		                          self->ub00_mvp, sizeof(cc_mat4f_t),
		                          (const void*) &mvp);

		vkk_renderer_updateBuffer(screen->renderer,
		                          self->ub10_color, sizeof(cc_vec4f_t),
		                          (const void*) &self->style.color);

		int multiply = 0;
		vkk_renderer_updateBuffer(screen->renderer,
		                          self->ub20_multiply,
		                          sizeof(int),
		                          (const void*) &multiply);

		vkk_uiFont_t* font = vkk_uiScreen_font(screen,
		                                       style->font_type);
		vkk_uniformAttachment_t ua =
		{
			.binding = 1,
			.type    = VKK_UNIFORM_TYPE_IMAGE_REF,
			.image   = font->img21
		};

		vkk_renderer_updateUniformSetRefs(screen->renderer,
		                                  self->us2_multiplyImage,
		                                  1, &ua);

		vkk_uiScreen_bind(screen, VKK_UI_SCREEN_BIND_TEXT);

		vkk_uniformSet_t* us_font[] =
		{
			self->us0_mvp,
			self->us1_color,
			self->us2_multiplyImage,
			screen->us3_tricolor,
		};

		vkk_renderer_bindUniformSets(screen->renderer,
		                             4, us_font);

		vkk_renderer_draw(screen->renderer, 2*3*len,
		                  1, &self->vb_xyuv);
	}
}

static void
vkk_uiText_addc(vkk_uiText_t* self, char c, int i,
                float* _offset)
{
	ASSERT(self);

	float              offset = *_offset;
	vkk_uiWidget_t*    widget = &self->base;
	vkk_uiTextStyle_t* style  = &self->style;
	vkk_uiScreen_t*    screen = widget->screen;
	vkk_uiFont_t*      font   = vkk_uiScreen_font(screen,
	                                            style->font_type);

	cc_rect2f_t pc;
	cc_rect2f_t tc;
	cc_rect2f_t vc;
	vkk_uiFont_request(font, c, &pc, &tc, &vc);

	// compute xyuv
	self->xyuv[24*i +  0] = vc.l + offset;   // tl
	self->xyuv[24*i +  1] = vc.t;
	self->xyuv[24*i +  2] = tc.l;
	self->xyuv[24*i +  3] = tc.t;
	self->xyuv[24*i +  4] = vc.l + offset;   // bl
	self->xyuv[24*i +  5] = vc.b;
	self->xyuv[24*i +  6] = tc.l;
	self->xyuv[24*i +  7] = tc.b;
	self->xyuv[24*i +  8] = vc.r + offset;   // br
	self->xyuv[24*i +  9] = vc.b;
	self->xyuv[24*i + 10] = tc.r;
	self->xyuv[24*i + 11] = tc.b;
	self->xyuv[24*i + 12] = vc.l + offset;   // tl
	self->xyuv[24*i + 13] = vc.t;
	self->xyuv[24*i + 14] = tc.l;
	self->xyuv[24*i + 15] = tc.t;
	self->xyuv[24*i + 16] = vc.r + offset;   // br
	self->xyuv[24*i + 17] = vc.b;
	self->xyuv[24*i + 18] = tc.r;
	self->xyuv[24*i + 19] = tc.b;
	self->xyuv[24*i + 20] = vc.r + offset;   // tr
	self->xyuv[24*i + 21] = vc.t;
	self->xyuv[24*i + 22] = tc.r;
	self->xyuv[24*i + 23] = tc.t;

	// next character offset
	*_offset += vc.r;
}

static int
vkk_uiText_keyPress(vkk_uiWidget_t* widget,
                    int keycode, int meta)
{
	ASSERT(widget);

	vkk_uiText_t*   self   = (vkk_uiText_t*) widget;
	vkk_uiScreen_t* screen = widget->screen;

	vkk_uiWidgetInput_fn input_fn = widget->fn.input_fn;

	size_t len  = strlen(self->string);
	size_t size = len + 1;
	if(keycode == VKK_PLATFORM_KEYCODE_ENTER)
	{
		if(input_fn)
		{
			(*input_fn)(widget, self->string);
		}
	}
	else if(keycode == VKK_PLATFORM_KEYCODE_BACKSPACE)
	{
		if(len > 0)
		{
			self->string[len - 1] = '\0';
			len  -= 1;
			size -= 1;
		}
		else
		{
			return 1;
		}
	}
	else
	{
		if(vkk_uiText_resize(self, size + 1))
		{
			self->string[len]     = (char) keycode;
			self->string[len + 1] = '\0';
			len  += 1;
			size += 1;
		}
		else
		{
			// accept the keypress but ignore error
			return 1;
		}
	}

	int   i;
	float offset = 0.0f;
	for(i = 0; i < len; ++i)
	{
		vkk_uiText_addc(self, self->string[i], i, &offset);
	}

	// add the cursor
	vkk_uiText_addc(self, VKK_UI_FONT_CURSOR, len, &offset);

	if(self->vb_xyuv)
	{
		// 2 triangles, 3 vertices, 4 components => 24
		size_t xyuv_size = 24*size*sizeof(float);
		vkk_renderer_updateBuffer(screen->renderer,
		                          self->vb_xyuv, xyuv_size,
		                          (const void*) self->xyuv);
	}

	vkk_uiScreen_layoutDirty(widget->screen);
	return 1;
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_uiText_t*
vkk_uiText_new(vkk_uiScreen_t* screen, size_t wsize,
               vkk_uiTextFn_t* tfn,
               vkk_uiTextLayout_t* text_layout,
               vkk_uiTextStyle_t* text_style,
               cc_vec4f_t* color_fill)
{
	ASSERT(screen);
	ASSERT(tfn);
	ASSERT(text_layout);
	ASSERT(text_style);
	ASSERT(color_fill);

	if(wsize == 0)
	{
		wsize = sizeof(vkk_uiText_t);
	}

	vkk_uiWidgetLayout_t layout =
	{
		.border   = text_layout->border ? text_layout->border :
		                                  text_style->spacing,
		.anchor   = text_layout->anchor,
		.wrapx    = text_layout->wrapx,
		.stretchx = text_layout->stretchx,
	};

	vkk_uiWidgetScroll_t widget_scroll =
	{
		.scroll_bar = 0
	};

	// implement the click/keyPress functions when accepting
	// input to trigger the softkey, focus and input events
	vkk_uiWidgetClick_fn    click_fn    = tfn->click_fn;
	vkk_uiWidgetKeyPress_fn keyPress_fn = NULL;
	if(tfn->input_fn)
	{
		click_fn    = vkk_uiText_click;
		keyPress_fn = vkk_uiText_keyPress;
	}

	vkk_uiWidgetFn_t fn =
	{
		.priv        = tfn->priv,
		.click_fn    = click_fn,
		.draw_fn     = vkk_uiText_draw,
		.keyPress_fn = keyPress_fn,
		.size_fn     = vkk_uiText_size,
		.input_fn    = tfn->input_fn,
	};

	vkk_uiText_t* self;
	self = (vkk_uiText_t*)
	       vkk_uiWidget_new(screen, wsize, color_fill, &layout,
	                        &widget_scroll, &fn);
	if(self == NULL)
	{
		return NULL;
	}

	memcpy(&self->style, text_style,
	       sizeof(vkk_uiTextStyle_t));
	self->click_fn = tfn->click_fn;

	self->ub00_mvp = vkk_buffer_new(screen->engine,
	                                VKK_UPDATE_MODE_ASYNCHRONOUS,
	                                VKK_BUFFER_USAGE_UNIFORM,
	                                sizeof(cc_mat4f_t), NULL);
	if(self->ub00_mvp == NULL)
	{
		goto fail_ub00_mvp;
	}

	vkk_uniformAttachment_t ua_mvp =
	{
		// layout(std140, set=0, binding=0) uniform uniformMvp
		.binding = 0,
		.type    = VKK_UNIFORM_TYPE_BUFFER,
		.buffer  = self->ub00_mvp
	};

	self->us0_mvp = vkk_uniformSet_new(screen->engine,
	                                  0, 1, &ua_mvp,
	                                  screen->usf0_mvp);
	if(self->us0_mvp == NULL)
	{
		goto fail_us0_mvp;
	}

	self->ub10_color = vkk_buffer_new(screen->engine,
	                                  VKK_UPDATE_MODE_ASYNCHRONOUS,
	                                  VKK_BUFFER_USAGE_UNIFORM,
	                                  sizeof(cc_vec4f_t),
	                                  &text_style->color);
	if(self->ub10_color == NULL)
	{
		goto fail_ub10_color;
	}

	vkk_uniformAttachment_t ua_color =
	{
		// layout(std140, set=1, binding=0) uniform uniformColor
		.binding = 0,
		.type    = VKK_UNIFORM_TYPE_BUFFER,
		.buffer  = self->ub10_color
	};

	self->us1_color = vkk_uniformSet_new(screen->engine,
	                                     1, 1, &ua_color,
	                                     screen->usf1_color);
	if(self->us1_color == NULL)
	{
		goto fail_us1_color;
	}

	self->ub20_multiply = vkk_buffer_new(screen->engine,
	                                     VKK_UPDATE_MODE_ASYNCHRONOUS,
	                                     VKK_BUFFER_USAGE_UNIFORM,
	                                     sizeof(int),
	                                     NULL);
	if(self->ub20_multiply == NULL)
	{
		goto fail_ub20_multiply;
	}

	vkk_uniformAttachment_t ua_multiply =
	{
		// layout(std140, set=2, binding=0) uniform uniformMultiply
		.binding = 0,
		.type    = VKK_UNIFORM_TYPE_BUFFER,
		.buffer  = self->ub20_multiply
	};

	self->us2_multiplyImage = vkk_uniformSet_new(screen->engine,
	                                             2, 1, &ua_multiply,
	                                             screen->usf2_multiplyImage);
	if(self->us2_multiplyImage == NULL)
	{
		goto fail_us2_multiplyImage;
	}

	// initialize string and cursor
	if(vkk_uiText_label(self, "%s", "") == 0)
	{
		goto fail_label;
	}

	// success
	return self;

	// failure
	fail_label:
		vkk_uniformSet_delete(&self->us2_multiplyImage);
	fail_us2_multiplyImage:
		vkk_buffer_delete(&self->ub20_multiply);
	fail_ub20_multiply:
		vkk_uniformSet_delete(&self->us1_color);
	fail_us1_color:
		vkk_buffer_delete(&self->ub10_color);
	fail_ub10_color:
		vkk_uniformSet_delete(&self->us0_mvp);
	fail_us0_mvp:
		vkk_buffer_delete(&self->ub00_mvp);
	fail_ub00_mvp:
		vkk_uiWidget_delete((vkk_uiWidget_t**) &self);
	return NULL;
}

vkk_uiText_t*
vkk_uiText_newPageHeading(vkk_uiScreen_t* screen)
{
	ASSERT(screen);

	vkk_uiTextLayout_t layout =
	{
		.border = VKK_UI_WIDGET_BORDER_NONE,
	};

	vkk_uiTextStyle_t style =
	{
		.font_type = VKK_UI_TEXT_FONTTYPE_BOLD,
		.size      = VKK_UI_TEXT_SIZE_MEDIUM,
		.spacing   = VKK_UI_TEXT_SPACING_MEDIUM
	};
	vkk_uiScreen_colorPageHeading(screen, &style.color);

	vkk_uiTextFn_t fn =
	{
		.priv = NULL
	};

	cc_vec4f_t clear =
	{
		.a = 0.0f,
	};

	return vkk_uiText_new(screen, 0, &fn, &layout,
	                      &style, &clear);
}

vkk_uiText_t*
vkk_uiText_newPageItem(vkk_uiScreen_t* screen)
{
	ASSERT(screen);

	vkk_uiTextLayout_t layout =
	{
		.border = VKK_UI_WIDGET_BORDER_NONE,
	};

	vkk_uiTextStyle_t style =
	{
		.font_type = VKK_UI_TEXT_FONTTYPE_REGULAR,
		.size      = VKK_UI_TEXT_SIZE_MEDIUM,
		.spacing   = VKK_UI_TEXT_SPACING_MEDIUM
	};
	vkk_uiScreen_colorPageItem(screen, &style.color);

	vkk_uiTextFn_t fn =
	{
		.priv = NULL
	};

	cc_vec4f_t clear =
	{
		.a = 0.0f,
	};

	return vkk_uiText_new(screen, 0, &fn, &layout,
	                      &style, &clear);
}

vkk_uiText_t*
vkk_uiText_newPageSubheading(vkk_uiScreen_t* screen)
{
	ASSERT(screen);

	vkk_uiTextLayout_t layout =
	{
		.border = VKK_UI_WIDGET_BORDER_NONE,
	};

	vkk_uiTextStyle_t style =
	{
		.font_type = VKK_UI_TEXT_FONTTYPE_BOLD,
		.size      = VKK_UI_TEXT_SIZE_SMALL,
		.spacing   = VKK_UI_TEXT_SPACING_MEDIUM
	};
	vkk_uiScreen_colorPageHeading(screen, &style.color);

	vkk_uiTextFn_t fn =
	{
		.priv = NULL
	};

	cc_vec4f_t clear =
	{
		.a = 0.0f,
	};

	return vkk_uiText_new(screen, 0, &fn, &layout,
	                      &style, &clear);
}

vkk_uiText_t*
vkk_uiText_newPageTextInput(vkk_uiScreen_t* screen,
                            vkk_uiTextFn_t* tfn)
{
	ASSERT(screen);
	ASSERT(tfn);

	vkk_uiTextLayout_t layout =
	{
		.border   = VKK_UI_WIDGET_BORDER_LARGE,
		.wrapx    = VKK_UI_WIDGET_WRAP_STRETCH_PARENT,
		.stretchx = 1.0f
	};

	vkk_uiTextStyle_t style =
	{
		.font_type = VKK_UI_TEXT_FONTTYPE_REGULAR,
		.size      = VKK_UI_TEXT_SIZE_MEDIUM,
		.spacing   = VKK_UI_TEXT_SPACING_MEDIUM
	};
	vkk_uiScreen_colorPageItem(screen, &style.color);

	cc_vec4f_t fill;
	vkk_uiScreen_colorPageEntry(screen, &fill);

	return vkk_uiText_new(screen, 0, tfn, &layout,
	                      &style, &fill);
}

vkk_uiText_t*
vkk_uiText_newInfoHeading(vkk_uiScreen_t* screen)
{
	ASSERT(screen);

	vkk_uiTextLayout_t layout =
	{
		.border = VKK_UI_WIDGET_BORDER_NONE,
	};

	vkk_uiTextStyle_t style =
	{
		.font_type = VKK_UI_TEXT_FONTTYPE_BOLD,
		.size      = VKK_UI_TEXT_SIZE_SMALL,
	};
	vkk_uiScreen_colorPageHeading(screen, &style.color);

	vkk_uiTextFn_t tfn =
	{
		.priv = NULL
	};

	cc_vec4f_t clear =
	{
		.a = 0.0f,
	};

	return vkk_uiText_new(screen, 0, &tfn, &layout,
	                      &style, &clear);
}

vkk_uiText_t*
vkk_uiText_newInfoItem(vkk_uiScreen_t* screen)
{
	ASSERT(screen);

	vkk_uiTextLayout_t layout =
	{
		.border = VKK_UI_WIDGET_BORDER_NONE,
	};

	vkk_uiTextStyle_t style =
	{
		.font_type = VKK_UI_TEXT_FONTTYPE_REGULAR,
		.size      = VKK_UI_TEXT_SIZE_SMALL,
	};
	vkk_uiScreen_colorPageItem(screen, &style.color);

	vkk_uiTextFn_t tfn =
	{
		.priv = NULL
	};

	cc_vec4f_t clear =
	{
		.a = 0.0f,
	};

	return vkk_uiText_new(screen, 0, &tfn, &layout,
	                      &style, &clear);
}

void vkk_uiText_delete(vkk_uiText_t** _self)
{
	ASSERT(_self);

	vkk_uiText_t* self = *_self;
	if(self)
	{
		vkk_uiWidget_t* widget = &self->base;
		vkk_uniformSet_delete(&self->us2_multiplyImage);
		vkk_buffer_delete(&self->ub20_multiply);
		vkk_uniformSet_delete(&self->us1_color);
		vkk_buffer_delete(&self->ub10_color);
		vkk_uniformSet_delete(&self->us0_mvp);
		vkk_buffer_delete(&self->ub00_mvp);
		vkk_uiScreen_textVb(widget->screen, 0, self->vb_xyuv);

		FREE(self->xyuv);
		FREE(self->string);

		vkk_uiWidget_delete((vkk_uiWidget_t**) _self);
	}
}

int vkk_uiText_width(vkk_uiText_t* self, int cursor)
{
	ASSERT(self);

	vkk_uiWidget_t*    widget = &self->base;
	vkk_uiTextStyle_t* style  = &self->style;

	vkk_uiFont_t* font;
	font = vkk_uiScreen_font(widget->screen,
	                         style->font_type);

	int   width = 0;
	char* s     = self->string;
	while(s[0] != '\0')
	{
		width += vkk_uiFont_width(font, s[0]);
		++s;
	}

	if(cursor)
	{
		width += vkk_uiFont_width(font, VKK_UI_FONT_CURSOR);
	}

	return width;
}

int vkk_uiText_height(vkk_uiText_t* self)
{
	ASSERT(self);

	vkk_uiWidget_t*    widget = &self->base;
	vkk_uiTextStyle_t* style  = &self->style;

	vkk_uiFont_t* font;
	font = vkk_uiScreen_font(widget->screen,
	                         style->font_type);
	return vkk_uiFont_height(font);
}

void vkk_uiText_color(vkk_uiText_t* self, cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	cc_vec4f_copy(color, &self->style.color);
}

int
vkk_uiText_label(vkk_uiText_t* self, const char* fmt, ...)
{
	ASSERT(self);
	ASSERT(fmt);

	vkk_uiWidget_t* widget = &self->base;
	vkk_uiScreen_t* screen = widget->screen;

	// decode string
	char tmp_string[256];
	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(tmp_string, 256, fmt, argptr);
	va_end(argptr);

	// fill the string
	size_t len  = strlen(tmp_string);
	size_t size = len + 1;
	if(vkk_uiText_resize(self, size) == 0)
	{
		return 0;
	}
	snprintf(self->string, size, "%s", tmp_string);

	int   i;
	float offset = 0.0f;
	for(i = 0; i < len; ++i)
	{
		vkk_uiText_addc(self, self->string[i], i, &offset);
	}

	// add the cursor
	vkk_uiText_addc(self, VKK_UI_FONT_CURSOR, len, &offset);

	if(self->vb_xyuv)
	{
		// 2 triangles, 3 vertices, 4 components => 24
		size_t xyuv_size = 24*size*sizeof(float);
		vkk_renderer_updateBuffer(screen->renderer,
		                          self->vb_xyuv, xyuv_size,
		                          (const void*) self->xyuv);
	}

	vkk_uiScreen_layoutDirty(screen);
	return 1;
}

const char* vkk_uiText_string(vkk_uiText_t* self)
{
	ASSERT(self);

	return self->string;
}
