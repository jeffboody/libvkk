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

#define LOG_TAG "vkui"
#include "../../libcc/math/cc_mat4f.h"
#include "../../libcc/math/cc_vec4f.h"
#include "../../libcc/cc_memory.h"
#include "../../libcc/cc_log.h"
#include "../../libcc/cc_timestamp.h"
#include "../vkk_platform.h"
#include "vkui_font.h"
#include "vkui_screen.h"
#include "vkui_text.h"

/***********************************************************
* private                                                  *
***********************************************************/

static int
vkui_text_resize(vkui_text_t* self, size_t size)
{
	ASSERT(self);

	vkui_widget_t* widget = (vkui_widget_t*) self;
	vkui_screen_t* screen = widget->screen;

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
	self->vb_xyuv = vkui_screen_textVb(screen, xyuv_size,
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
vkui_text_size(vkui_widget_t* widget, float* w, float* h)
{
	ASSERT(widget);
	ASSERT(w);
	ASSERT(h);

	vkui_text_t*      self  = (vkui_text_t*) widget;
	vkui_textStyle_t* style = &self->style;

	float size = vkui_screen_layoutText(widget->screen,
	                                    style->size);
	if(widget->layout.wrapx == VKUI_WIDGET_WRAP_SHRINK)
	{
		float width  = (float) 0.0f;
		float height = (float) vkui_text_height(self);
		if(vkui_widget_hasFocus(widget))
		{
			width = (float) vkui_text_width(self, 1);
		}
		else
		{
			width = (float) vkui_text_width(self, 0);
		}
		*w = size*(width/height);
	}
	*h = size;
}

static void vkui_text_draw(vkui_widget_t* widget)
{
	ASSERT(widget);

	vkui_text_t*      self   = (vkui_text_t*) widget;
	vkui_textStyle_t* style  = &self->style;
	vkui_screen_t*    screen = widget->screen;

	int len = strlen(self->string);
	if(vkui_widget_hasFocus(widget))
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
		vkui_screen_sizef(screen, &w, &h);

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

		vkui_font_t* font = vkui_screen_font(screen,
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

		vkui_screen_bind(screen, VKUI_SCREEN_BIND_TEXT);

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
vkui_text_addc(vkui_text_t* self, char c, int i,
               float* _offset)
{
	ASSERT(self);

	float             offset = *_offset;
	vkui_widget_t*    widget = (vkui_widget_t*) self;
	vkui_textStyle_t* style  = &self->style;
	vkui_screen_t*    screen = widget->screen;
	vkui_font_t*      font   = vkui_screen_font(screen,
	                                            style->font_type);

	cc_rect2f_t pc;
	cc_rect2f_t tc;
	cc_rect2f_t vc;
	vkui_font_request(font, c, &pc, &tc, &vc);

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
vkui_text_keyPress(vkui_widget_t* widget, void* priv,
                   int keycode, int meta)
{
	// priv may be NULL
	ASSERT(widget);

	vkui_text_t*   self   = (vkui_text_t*) widget;
	vkui_screen_t* screen = widget->screen;

	vkui_text_enterFn enter_fn = self->enter_fn;
	if(enter_fn == NULL)
	{
		LOGE("enter_fn is NULL");
		return 0;
	}

	size_t len  = strlen(self->string);
	size_t size = len + 1;
	if(keycode == VKK_KEYCODE_ENTER)
	{
		(*enter_fn)(widget, priv, self->string);
	}
	else if(keycode == VKK_KEYCODE_ESCAPE)
	{
		return 0;
	}
	else if(keycode == VKK_KEYCODE_BACKSPACE)
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
		if(vkui_text_resize(self, size + 1))
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
		vkui_text_addc(self, self->string[i], i, &offset);
	}

	// add the cursor
	vkui_text_addc(self, VKUI_FONT_CURSOR, len, &offset);

	if(self->vb_xyuv)
	{
		// 2 triangles, 3 vertices, 4 components => 24
		size_t xyuv_size = 24*size*sizeof(float);
		vkk_renderer_updateBuffer(screen->renderer,
		                          self->vb_xyuv, xyuv_size,
		                          (const void*) self->xyuv);
	}

	vkui_screen_dirty(widget->screen);
	return 1;
}

/***********************************************************
* public                                                   *
***********************************************************/

vkui_text_t*
vkui_text_new(vkui_screen_t* screen, size_t wsize,
              vkui_textLayout_t* text_layout,
              vkui_textStyle_t* text_style,
              vkui_textFn_t* text_fn,
              cc_vec4f_t* color_fill)
{
	ASSERT(screen);
	ASSERT(text_layout);
	ASSERT(text_style);
	ASSERT(text_fn);
	ASSERT(color_fill);

	if(wsize == 0)
	{
		wsize = sizeof(vkui_text_t);
	}

	vkui_widgetLayout_t layout =
	{
		.border   = text_layout->border ? text_layout->border :
		                                  text_style->spacing,
		.anchor   = text_layout->anchor,
		.wrapx    = text_layout->wrapx,
		.stretchx = text_layout->stretchx,
	};

	vkui_widgetScroll_t widget_scroll =
	{
		.scroll_bar = 0
	};

	vkui_widgetPrivFn_t priv_fn =
	{
		.size_fn     = vkui_text_size,
		.keyPress_fn = vkui_text_keyPress,
		.draw_fn     = vkui_text_draw,
	};

	vkui_text_t* self;
	self = (vkui_text_t*)
	       vkui_widget_new(screen, wsize, color_fill, &layout,
	                       &widget_scroll, &text_fn->fn,
	                       &priv_fn);
	if(self == NULL)
	{
		return NULL;
	}

	self->enter_fn = text_fn->enter_fn;
	memcpy(&self->style, text_style, sizeof(vkui_textStyle_t));

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
	if(vkui_text_label(self, "%s", "") == 0)
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
		vkui_widget_delete((vkui_widget_t**) &self);
	return NULL;
}

void vkui_text_delete(vkui_text_t** _self)
{
	ASSERT(_self);

	vkui_text_t* self = *_self;
	if(self)
	{
		vkui_widget_t* widget = (vkui_widget_t*) self;
		vkui_screen_t* screen = widget->screen;

		vkk_uniformSet_delete(&self->us2_multiplyImage);
		vkk_buffer_delete(&self->ub20_multiply);
		vkk_uniformSet_delete(&self->us1_color);
		vkk_buffer_delete(&self->ub10_color);
		vkk_uniformSet_delete(&self->us0_mvp);
		vkk_buffer_delete(&self->ub00_mvp);
		vkui_screen_textVb(screen, 0, self->vb_xyuv);

		FREE(self->xyuv);
		FREE(self->string);

		vkui_widget_delete((vkui_widget_t**) _self);
	}
}

int vkui_text_width(vkui_text_t* self, int cursor)
{
	ASSERT(self);

	vkui_widget_t*    widget = (vkui_widget_t*) self;
	vkui_textStyle_t* style  = &self->style;
	vkui_screen_t*    screen = widget->screen;
	vkui_font_t*      font   = vkui_screen_font(screen,
	                                            style->font_type);

	int   width = 0;
	char* s     = self->string;
	while(s[0] != '\0')
	{
		width += vkui_font_width(font, s[0]);
		++s;
	}

	if(cursor)
	{
		width += vkui_font_width(font, VKUI_FONT_CURSOR);
	}

	return width;
}

int vkui_text_height(vkui_text_t* self)
{
	ASSERT(self);

	vkui_widget_t*    widget = (vkui_widget_t*) self;
	vkui_textStyle_t* style  = &self->style;
	vkui_screen_t*    screen = widget->screen;
	vkui_font_t*      font   = vkui_screen_font(screen,
	                                            style->font_type);
	return vkui_font_height(font);
}

void vkui_text_color(vkui_text_t* self, cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	cc_vec4f_copy(color, &self->style.color);
}

int
vkui_text_label(vkui_text_t* self, const char* fmt, ...)
{
	ASSERT(self);
	ASSERT(fmt);

	vkui_widget_t* widget = (vkui_widget_t*) self;
	vkui_screen_t* screen = widget->screen;

	// decode string
	char tmp_string[256];
	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(tmp_string, 256, fmt, argptr);
	va_end(argptr);

	// fill the string
	size_t len  = strlen(tmp_string);
	size_t size = len + 1;
	if(vkui_text_resize(self, size) == 0)
	{
		return 0;
	}
	snprintf(self->string, size, "%s", tmp_string);

	int   i;
	float offset = 0.0f;
	for(i = 0; i < len; ++i)
	{
		vkui_text_addc(self, self->string[i], i, &offset);
	}

	// add the cursor
	vkui_text_addc(self, VKUI_FONT_CURSOR, len, &offset);

	if(self->vb_xyuv)
	{
		// 2 triangles, 3 vertices, 4 components => 24
		size_t xyuv_size = 24*size*sizeof(float);
		vkk_renderer_updateBuffer(screen->renderer,
		                          self->vb_xyuv, xyuv_size,
		                          (const void*) self->xyuv);
	}

	vkui_screen_dirty(screen);
	return 1;
}
