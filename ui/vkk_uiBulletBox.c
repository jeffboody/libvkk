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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "vkk_ui"
#include "../../libcc/cc_log.h"
#include "../vkk_ui.h"

/***********************************************************
* private                                                  *
***********************************************************/

#define VKK_UI_BULLETBOX_SPACE 1.25f

static void
vkk_uiBulletBox_size(vkk_uiWidget_t* widget,
                     float* w, float* h)
{
	ASSERT(widget);
	ASSERT(w);
	ASSERT(h);

	vkk_uiBulletBox_t* self = (vkk_uiBulletBox_t*) widget;
	vkk_uiWidget_t*    icon = &self->icon->base;
	vkk_uiWidget_t*    text = &self->text->base;

	float icon_w = *w;
	float icon_h = *h;
	vkk_uiWidget_layoutSize(icon, &icon_w, &icon_h);

	float text_w = *w;
	float text_h = *h;
	vkk_uiWidget_layoutSize(text, &text_w, &text_h);

	*w = VKK_UI_BULLETBOX_SPACE*icon_w + text_w;
	*h = (icon_h > text_h) ? icon_h : text_h;
}

static void
vkk_uiBulletBox_layout(vkk_uiWidget_t* widget,
                       int dragx, int dragy)
{
	ASSERT(widget);

	vkk_uiBulletBox_t* self = (vkk_uiBulletBox_t*) widget;
	vkk_uiWidget_t*    icon = &self->icon->base;
	vkk_uiWidget_t*    text = &self->text->base;

	// initialize the layout
	float x  = 0.0f;
	float y  = 0.0f;
	float t  = widget->rect_draw.t;
	float l  = widget->rect_draw.l;
	float h  = widget->rect_draw.h;
	float iw = icon->rect_border.w;
	float tw = text->rect_border.w;

	// layout icon
	cc_rect1f_t rect_draw =
	{
		.t = t,
		.l = l,
		.w = iw,
		.h = h
	};
	cc_rect1f_t rect_clip;
	vkk_uiWidget_layoutAnchor(icon, &rect_draw, &x, &y);
	cc_rect1f_intersect(&rect_draw,
	                    &widget->rect_clip,
	                    &rect_clip);
	vkk_uiWidget_layoutXYClip(icon, x, y, &rect_clip,
	                          dragx, dragy);

	// layout text
	rect_draw.l = l + VKK_UI_BULLETBOX_SPACE*iw;
	rect_draw.w = tw;
	vkk_uiWidget_layoutAnchor(text, &rect_draw, &x, &y);
	cc_rect1f_intersect(&rect_draw,
	                    &widget->rect_clip,
	                    &rect_clip);
	vkk_uiWidget_layoutXYClip(text, x, y, &rect_clip,
	                          dragx, dragy);
}

static void
vkk_uiBulletBox_drag(vkk_uiWidget_t* widget,
                    float x, float y,
                    float dx, float dy)
{
	ASSERT(widget);

	vkk_uiBulletBox_t* self = (vkk_uiBulletBox_t*) widget;
	vkk_uiWidget_drag((vkk_uiWidget_t*) self->icon,
	                  x, y, dx, dy);
	vkk_uiWidget_drag((vkk_uiWidget_t*) self->text,
	                  x, y, dx, dy);
}

static void vkk_uiBulletBox_draw(vkk_uiWidget_t* widget)
{
	ASSERT(widget);

	vkk_uiBulletBox_t* self = (vkk_uiBulletBox_t*) widget;
	vkk_uiWidget_draw((vkk_uiWidget_t*) self->icon);
	vkk_uiWidget_draw((vkk_uiWidget_t*) self->text);
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_uiBulletBox_t*
vkk_uiBulletBox_new(vkk_uiScreen_t* screen, size_t wsize,
                    vkk_uiBulletBoxFn_t* bbfn,
                    int anchor,
                    vkk_uiBulletBoxStyle_t* bulletbox_style,
                    const char** sprite_array)
{
	ASSERT(screen);
	ASSERT(bbfn);
	ASSERT(bulletbox_style);
	ASSERT(sprite_array);

	vkk_uiTextStyle_t* text_style;
	text_style = &bulletbox_style->text_style;

	if(wsize == 0)
	{
		wsize = sizeof(vkk_uiBulletBox_t);
	}

	vkk_uiWidgetLayout_t widget_layout =
	{
		.anchor = anchor
	};

	cc_vec4f_t clear =
	{
		.a = 0.0f
	};

	vkk_uiWidgetScroll_t scroll =
	{
		.scroll_bar = 0
	};

	vkk_uiWidgetFn_t fn =
	{
		.priv      = bbfn->priv,
		.click_fn  = bbfn->click_fn,
		.drag_fn   = vkk_uiBulletBox_drag,
		.draw_fn   = vkk_uiBulletBox_draw,
		.layout_fn = vkk_uiBulletBox_layout,
		.size_fn   = vkk_uiBulletBox_size,
	};

	vkk_uiBulletBox_t* self;
	self = (vkk_uiBulletBox_t*)
	       vkk_uiWidget_new(screen, wsize, &clear,
	                        &widget_layout, &scroll, &fn);
	if(self == NULL)
	{
		return NULL;
	}

	int wrap = VKK_UI_WIDGET_WRAP_STRETCH_TEXT_VMEDIUM;
	if(text_style->size == VKK_UI_TEXT_SIZE_LARGE)
	{
		wrap = VKK_UI_WIDGET_WRAP_STRETCH_TEXT_VLARGE;
	}
	else if((text_style->size == VKK_UI_TEXT_SIZE_SMALL) ||
	        (text_style->size == VKK_UI_TEXT_SIZE_XSMALL))
	{
		wrap = VKK_UI_WIDGET_WRAP_STRETCH_TEXT_VSMALL;
	}

	// convert spacing to border
	int spacing = text_style->spacing;
	int border  = spacing | VKK_UI_WIDGET_BORDER_HMEDIUM;
	if(spacing < VKK_UI_TEXT_SPACING_MEDIUM)
	{
		border = spacing | VKK_UI_WIDGET_BORDER_HSMALL;
	}

	vkk_uiWidgetLayout_t sprite_layout =
	{
		.border   = border,
		.wrapx    = wrap,
		.wrapy    = wrap,
		.stretchx = 1.0f,
		.stretchy = 1.0f
	};

	vkk_uiSpriteFn_t sfn =
	{
		.priv = NULL
	};

	self->icon = vkk_uiSprite_new(screen, 0, &sfn,
	                              &sprite_layout,
	                              &bulletbox_style->color_icon,
	                              sprite_array);
	if(self->icon == NULL)
	{
		goto fail_icon;
	}

	vkk_uiTextLayout_t text_layout =
	{
		.border = VKK_UI_WIDGET_BORDER_NONE
	};

	vkk_uiTextFn_t tfn =
	{
		.priv = NULL
	};

	self->text = vkk_uiText_new(screen, 0, &tfn,
	                            &text_layout, text_style,
	                            &clear);
	if(self->text == NULL)
	{
		goto fail_text;
	}

	// success
	return self;

	// failure
	fail_text:
		vkk_uiSprite_delete(&self->icon);
	fail_icon:
		vkk_uiWidget_delete((vkk_uiWidget_t**) &self);
	return NULL;
}

vkk_uiBulletBox_t*
vkk_uiBulletBox_newPageItem(vkk_uiScreen_t* screen,
                            vkk_uiBulletBoxFn_t* bbfn,
                            const char** sprite_array)
{
	ASSERT(screen);
	ASSERT(bbfn);
	ASSERT(sprite_array);

	vkk_uiBulletBoxStyle_t style =
	{
		.text_style =
		{
			.font_type = VKK_UI_TEXT_FONTTYPE_REGULAR,
			.size      = VKK_UI_TEXT_SIZE_MEDIUM,
			.spacing   = VKK_UI_TEXT_SPACING_LARGE
		}
	};
	vkk_uiScreen_colorPageItem(screen, &style.color_icon);
	vkk_uiScreen_colorPageItem(screen, &style.text_style.color);

	return vkk_uiBulletBox_new(screen, 0, bbfn,
	                           VKK_UI_WIDGET_ANCHOR_TL,
	                           &style, sprite_array);
}

vkk_uiBulletBox_t*
vkk_uiBulletBox_newInfoItem(vkk_uiScreen_t* screen,
                            const char** sprite_array)
{
	ASSERT(screen);
	ASSERT(sprite_array);

	vkk_uiBulletBoxFn_t bbfn =
	{
		.priv = NULL
	};

	vkk_uiBulletBoxStyle_t style =
	{
		.text_style =
		{
			.font_type = VKK_UI_TEXT_FONTTYPE_REGULAR,
			.size      = VKK_UI_TEXT_SIZE_SMALL,
		}
	};
	vkk_uiScreen_colorPageItem(screen, &style.color_icon);
	vkk_uiScreen_colorPageItem(screen, &style.text_style.color);

	return vkk_uiBulletBox_new(screen, 0, &bbfn,
	                           VKK_UI_WIDGET_ANCHOR_TL,
	                           &style, sprite_array);
}

vkk_uiBulletBox_t*
vkk_uiBulletBox_newFooterItem(vkk_uiScreen_t* screen,
                              vkk_uiBulletBoxFn_t* bbfn,
                              const char** sprite_array)
{
	ASSERT(screen);
	ASSERT(bbfn);
	ASSERT(sprite_array);

	vkk_uiBulletBoxStyle_t style =
	{
		.text_style =
		{
			.font_type = VKK_UI_TEXT_FONTTYPE_BOLD,
			.size      = VKK_UI_TEXT_SIZE_MEDIUM,
			.spacing   = VKK_UI_TEXT_SPACING_XLARGE
		}
	};
	vkk_uiScreen_colorFooterItem(screen, &style.color_icon);
	vkk_uiScreen_colorFooterItem(screen, &style.text_style.color);

	return vkk_uiBulletBox_new(screen, 0, bbfn,
	                           VKK_UI_WIDGET_ANCHOR_TC,
	                           &style, sprite_array);
}

void vkk_uiBulletBox_delete(vkk_uiBulletBox_t** _self)
{
	ASSERT(_self);

	vkk_uiBulletBox_t* self = *_self;
	if(self)
	{
		vkk_uiText_delete(&self->text);
		vkk_uiSprite_delete(&self->icon);
		vkk_uiWidget_delete((vkk_uiWidget_t**) _self);
	}
}

void vkk_uiBulletBox_select(vkk_uiBulletBox_t* self,
                            uint32_t index)
{
	ASSERT(self);

	vkk_uiSprite_select(self->icon, index);
}

void vkk_uiBulletBox_colorIcon(vkk_uiBulletBox_t* self,
                               cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	vkk_uiSprite_color(self->icon, color);
}

void vkk_uiBulletBox_colorText(vkk_uiBulletBox_t* self,
                               cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	vkk_uiText_color(self->text, color);
}

void vkk_uiBulletBox_label(vkk_uiBulletBox_t* self,
                           const char* fmt, ...)
{
	ASSERT(self);
	ASSERT(fmt);

	// decode string
	char    string[256];
	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(string, 256, fmt, argptr);
	va_end(argptr);

	vkk_uiText_label(self->text, "%s", string);
}
