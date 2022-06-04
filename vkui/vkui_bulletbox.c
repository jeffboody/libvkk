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

#define LOG_TAG "vkui"
#include "../../libcc/cc_log.h"
#include "vkui_bulletbox.h"
#include "vkui_screen.h"
#include "vkui_sprite.h"
#include "vkui_text.h"
#include "vkui_widget.h"

/***********************************************************
* private                                                  *
***********************************************************/

#define VKUI_BULLETBOX_SPACE 1.25f

static void
vkui_bulletbox_size(vkui_widget_t* widget,
                    float* w, float* h)
{
	ASSERT(widget);
	ASSERT(w);
	ASSERT(h);

	vkui_bulletbox_t* self = (vkui_bulletbox_t*) widget;
	vkui_widget_t*    icon = (vkui_widget_t*) self->icon;
	vkui_widget_t*    text = (vkui_widget_t*) self->text;

	float icon_w = *w;
	float icon_h = *h;
	vkui_widget_layoutSize(icon, &icon_w, &icon_h);

	float text_w = *w;
	float text_h = *h;
	vkui_widget_layoutSize(text, &text_w, &text_h);

	*w = VKUI_BULLETBOX_SPACE*icon_w + text_w;
	*h = (icon_h > text_h) ? icon_h : text_h;
}

static void
vkui_bulletbox_layout(vkui_widget_t* widget,
                      int dragx, int dragy)
{
	ASSERT(widget);

	vkui_bulletbox_t* self = (vkui_bulletbox_t*) widget;
	vkui_widget_t*    icon = (vkui_widget_t*) self->icon;
	vkui_widget_t*    text = (vkui_widget_t*) self->text;

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
	vkui_widget_layoutAnchor(icon, &rect_draw, &x, &y);
	cc_rect1f_intersect(&rect_draw,
	                    &widget->rect_clip,
	                    &rect_clip);
	vkui_widget_layoutXYClip(icon, x, y, &rect_clip,
	                         dragx, dragy);

	// layout text
	rect_draw.l = l + VKUI_BULLETBOX_SPACE*iw;
	rect_draw.w = tw;
	vkui_widget_layoutAnchor(text, &rect_draw, &x, &y);
	cc_rect1f_intersect(&rect_draw,
	                    &widget->rect_clip,
	                    &rect_clip);
	vkui_widget_layoutXYClip(text, x, y, &rect_clip,
	                         dragx, dragy);
}

static void
vkui_bulletbox_drag(vkui_widget_t* widget,
                    float x, float y,
                    float dx, float dy)
{
	ASSERT(widget);

	vkui_bulletbox_t* self = (vkui_bulletbox_t*) widget;
	vkui_widget_drag((vkui_widget_t*) self->icon,
	                 x, y, dx, dy);
	vkui_widget_drag((vkui_widget_t*) self->text,
	                 x, y, dx, dy);
}

static void vkui_bulletbox_draw(vkui_widget_t* widget)
{
	ASSERT(widget);

	vkui_bulletbox_t* self = (vkui_bulletbox_t*) widget;
	vkui_widget_draw((vkui_widget_t*) self->icon);
	vkui_widget_draw((vkui_widget_t*) self->text);
}

/***********************************************************
* public                                                   *
***********************************************************/

vkui_bulletbox_t*
vkui_bulletbox_new(vkui_screen_t* screen, size_t wsize,
                   int anchor,
                   vkui_widgetFn_t* fn,
                   vkui_bulletboxStyle_t* bulletbox_style,
                   const char** sprite_array)
{
	ASSERT(screen);
	ASSERT(fn);
	ASSERT(bulletbox_style);
	ASSERT(sprite_array);

	vkui_textStyle_t* text_style;
	text_style = &bulletbox_style->text_style;

	if(wsize == 0)
	{
		wsize = sizeof(vkui_bulletbox_t);
	}

	vkui_widgetLayout_t widget_layout =
	{
		.anchor = anchor
	};

	cc_vec4f_t clear =
	{
		.a = 0.0f
	};

	vkui_widgetScroll_t scroll =
	{
		.scroll_bar = 0
	};

	vkui_widgetPrivFn_t priv_fn =
	{
		.size_fn   = vkui_bulletbox_size,
		.layout_fn = vkui_bulletbox_layout,
		.drag_fn   = vkui_bulletbox_drag,
		.draw_fn   = vkui_bulletbox_draw,
	};

	vkui_bulletbox_t* self;
	self = (vkui_bulletbox_t*)
	       vkui_widget_new(screen, wsize, &clear,
	                       &widget_layout, &scroll, fn,
	                       &priv_fn);
	if(self == NULL)
	{
		return NULL;
	}

	int wrap = VKUI_WIDGET_WRAP_STRETCH_TEXT_VMEDIUM;
	if(text_style->size == VKUI_TEXT_SIZE_LARGE)
	{
		wrap = VKUI_WIDGET_WRAP_STRETCH_TEXT_VLARGE;
	}
	else if((text_style->size == VKUI_TEXT_SIZE_SMALL) ||
	        (text_style->size == VKUI_TEXT_SIZE_XSMALL))
	{
		wrap = VKUI_WIDGET_WRAP_STRETCH_TEXT_VSMALL;
	}

	// convert spacing to border
	int spacing = text_style->spacing;
	int border  = spacing | VKUI_WIDGET_BORDER_HMEDIUM;
	if(spacing < VKUI_TEXT_SPACING_MEDIUM)
	{
		border = spacing | VKUI_WIDGET_BORDER_HSMALL;
	}

	vkui_widgetLayout_t sprite_layout =
	{
		.border   = border,
		.wrapx    = wrap,
		.wrapy    = wrap,
		.stretchx = 1.0f,
		.stretchy = 1.0f
	};

	vkui_widgetFn_t sprite_fn;
	memset(&sprite_fn, 0, sizeof(vkui_widgetFn_t));

	self->icon = vkui_sprite_new(screen, 0, &sprite_layout,
	                             &sprite_fn,
	                             &bulletbox_style->color_icon,
	                             sprite_array);
	if(self->icon == NULL)
	{
		goto fail_icon;
	}

	vkui_textLayout_t text_layout =
	{
		.border = VKUI_WIDGET_BORDER_NONE
	};

	vkui_textFn_t text_fn;
	memset(&text_fn, 0, sizeof(vkui_textFn_t));

	self->text = vkui_text_new(screen, 0, &text_layout,
	                           text_style, &text_fn,
	                           &clear);
	if(self->text == NULL)
	{
		goto fail_text;
	}

	// success
	return self;

	// failure
	fail_text:
		vkui_sprite_delete(&self->icon);
	fail_icon:
		vkui_widget_delete((vkui_widget_t**) &self);
	return NULL;
}

vkui_bulletbox_t*
vkui_bulletbox_newPageItem(vkui_screen_t* screen,
                           vkui_widgetFn_t* fn,
                           const char** sprite_array)
{
	ASSERT(screen);
	ASSERT(fn);
	ASSERT(sprite_array);

	vkui_bulletboxStyle_t style =
	{
		.text_style =
		{
			.font_type = VKUI_TEXT_FONTTYPE_REGULAR,
			.size      = VKUI_TEXT_SIZE_MEDIUM,
			.spacing   = VKUI_TEXT_SPACING_MEDIUM
		}
	};
	vkui_screen_colorPageItem(screen, &style.color_icon);
	vkui_screen_colorPageItem(screen, &style.text_style.color);

	return vkui_bulletbox_new(screen, 0,
	                          VKUI_WIDGET_ANCHOR_TL, fn,
	                          &style, sprite_array);
}

vkui_bulletbox_t*
vkui_bulletbox_newFooterItem(vkui_screen_t* screen,
                             vkui_widgetFn_t* fn,
                             const char** sprite_array)
{
	ASSERT(screen);
	ASSERT(fn);
	ASSERT(sprite_array);

	vkui_bulletboxStyle_t style =
	{
		.text_style =
		{
			.font_type = VKUI_TEXT_FONTTYPE_BOLD,
			.size      = VKUI_TEXT_SIZE_MEDIUM,
			.spacing   = VKUI_TEXT_SPACING_LARGE
		}
	};
	vkui_screen_colorFooterItem(screen, &style.color_icon);
	vkui_screen_colorFooterItem(screen, &style.text_style.color);

	return vkui_bulletbox_new(screen, 0,
	                          VKUI_WIDGET_ANCHOR_TC, fn,
	                          &style, sprite_array);
}

void vkui_bulletbox_delete(vkui_bulletbox_t** _self)
{
	ASSERT(_self);

	vkui_bulletbox_t* self = *_self;
	if(self)
	{
		vkui_text_delete(&self->text);
		vkui_sprite_delete(&self->icon);
		vkui_widget_delete((vkui_widget_t**) _self);
	}
}

void vkui_bulletbox_select(vkui_bulletbox_t* self,
                           uint32_t index)
{
	ASSERT(self);

	vkui_sprite_select(self->icon, index);
}

void vkui_bulletbox_colorIcon(vkui_bulletbox_t* self,
                              cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	vkui_sprite_color(self->icon, color);
}

void vkui_bulletbox_colorText(vkui_bulletbox_t* self,
                              cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	vkui_text_color(self->text, color);
}

void vkui_bulletbox_label(vkui_bulletbox_t* self,
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

	vkui_text_label(self->text, "%s", string);
}
