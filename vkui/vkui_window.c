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

#define LOG_TAG "vkui"
#include "../../libcc/cc_log.h"
#include "vkui_bulletbox.h"
#include "vkui_screen.h"
#include "vkui_window.h"
#include "vkui_widget.h"

/***********************************************************
* private                                                  *
***********************************************************/

static void
vkui_window_size(vkui_widget_t* widget, float* w, float* h)
{
	ASSERT(widget);
	ASSERT(w);
	ASSERT(h);

	vkui_window_t* self   = (vkui_window_t*) widget;
	vkui_widget_t* bullet = (vkui_widget_t*) self->bullet;
	vkui_widget_t* body   = self->body;
	vkui_widget_t* footer = self->footer;

	float wmax     = 0.0f;
	float hsum     = 0.0f;
	float bullet_w = *w;
	float bullet_h = *h;

	// layout bullet
	vkui_widget_layoutSize(bullet, &bullet_w, &bullet_h);
	wmax = bullet_w;
	hsum = bullet_h;

	// layout footer
	if(footer)
	{
		float footer_w = *w;
		float footer_h = *h - hsum;
		if(footer_h < 0.0f)
		{
			footer_h = 0.0f;
		}

		vkui_widget_layoutSize(footer, &footer_w, &footer_h);
		if(footer_w > wmax)
		{
			wmax = footer_w;
		}

		hsum += footer_h;
	}

	// layout body
	float body_w = *w;
	float body_h = *h - hsum;
	if(body_h < 0.0f)
	{
		body_h = 0.0f;
	}
	vkui_widget_layoutSize(body, &body_w, &body_h);

	if(body_w > wmax)
	{
		wmax = body_w;
	}
	hsum += body_h;

	*w = wmax;
	*h = hsum;
}

static int
vkui_window_click(vkui_widget_t* widget, void* priv,
                  int state, float x, float y)
{
	// priv may be NULL
	ASSERT(widget);

	vkui_window_t* self = (vkui_window_t*) widget;
	vkui_widget_t* bullet = (vkui_widget_t*) self->bullet;
	vkui_widget_t* body   = (vkui_widget_t*) self->body;
	vkui_widget_t* footer = (vkui_widget_t*) self->footer;
	if(vkui_widget_click(bullet, state, x, y) ||
	   vkui_widget_click(body, state, x, y)   ||
	   (footer && vkui_widget_click(footer, state, x, y)))
	{
		return 1;
	}

	// viewboxes are always clicked unless transparent
	if(self->transparent)
	{
		return 0;
	}
	return 1;
}

static void
vkui_window_layout(vkui_widget_t* widget,
                   int dragx, int dragy)
{
	ASSERT(widget);

	vkui_widgetLayout_t* layout = &widget->layout;
	vkui_window_t*       self   = (vkui_window_t*) widget;
	vkui_widget_t*       bullet = (vkui_widget_t*) self->bullet;
	vkui_widget_t*       body   = self->body;
	vkui_widget_t*       footer = self->footer;

	// note that the viewbox layout is a bit unique because
	// the top/bottom borders are inserted between header/body
	// and footer/body rather than at the absolute top/bottom

	// initialize the layout
	float x  = 0.0f;
	float y  = 0.0f;
	float t  = self->widget.rect_border.t;
	float l  = self->widget.rect_draw.l;
	float w  = self->widget.rect_draw.w;
	float h  = self->widget.rect_border.h;
	float bullet_h = bullet->rect_border.h;
	float footer_h = 0.0f;
	cc_rect1f_t rect_clip;
	cc_rect1f_t rect_draw;

	// layout bullet
	rect_draw.t = t;
	rect_draw.l = l;
	rect_draw.w = w;
	rect_draw.h = bullet_h;
	vkui_widget_layoutAnchor(bullet, &rect_draw, &x, &y);
	cc_rect1f_intersect(&rect_draw,
	                    &self->widget.rect_clip,
	                    &rect_clip);
	vkui_widget_layoutXYClip(bullet, x, y, &rect_clip,
	                         dragx, dragy);

	// get separator size
	float h_bo = 0.0f;
	float v_bo = 0.0f;
	vkui_screen_layoutBorder(widget->screen,
	                         layout->border,
	                         &h_bo, &v_bo);

	// layout body
	if(footer)
	{
		footer_h = footer->rect_border.h;
		rect_draw.t = t + bullet_h + v_bo;
		rect_draw.h = h - bullet_h - footer_h - 2.0f*v_bo;
	}
	else
	{
		rect_draw.t = t + bullet_h + v_bo;
		rect_draw.h = h - bullet_h - 2.0f*v_bo;
	}
	vkui_widget_layoutAnchor(body, &rect_draw, &x, &y);
	cc_rect1f_intersect(&rect_draw,
	                    &self->widget.rect_clip,
	                    &rect_clip);
	vkui_widget_layoutXYClip(body, x, y, &rect_clip,
	                         dragx, dragy);

	// tricolor values
	float a = bullet->rect_border.t + bullet->rect_border.h;
	float b = widget->rect_border.t + widget->rect_border.h;

	// layout footer
	if(footer)
	{
		rect_draw.t = t + h - footer_h;
		rect_draw.l = l;
		rect_draw.w = w;
		rect_draw.h = footer_h;
		vkui_widget_layoutAnchor(footer, &rect_draw, &x, &y);
		cc_rect1f_intersect(&rect_draw,
		                    &self->widget.rect_clip,
		                    &rect_clip);
		vkui_widget_layoutXYClip(footer, x, y, &rect_clip,
		                         dragx, dragy);

		b = footer->rect_border.t;
	}

	vkui_widget_tricolorAB(widget, a, b);
}

static void
vkui_window_drag(vkui_widget_t* widget, float x, float y,
                 float dx, float dy)
{
	ASSERT(widget);

	vkui_window_t* self = (vkui_window_t*) widget;
	vkui_widget_drag((vkui_widget_t*) self->bullet,
	                 x, y, dx, dy);
	vkui_widget_drag(self->body, x, y, dx, dy);
	if(self->footer)
	{
		vkui_widget_drag(self->footer, x, y, dx, dy);
	}
}

static void
vkui_window_scrollTop(vkui_widget_t* widget)
{
	ASSERT(widget);

	vkui_window_t* self = (vkui_window_t*) widget;

	vkui_widget_scrollTop(self->body);
}

static void
vkui_window_draw(vkui_widget_t* widget)
{
	ASSERT(widget);

	vkui_window_t* self = (vkui_window_t*) widget;

	vkui_widget_draw((vkui_widget_t*) self->bullet);
	vkui_widget_draw(self->body);
	if(self->footer)
	{
		vkui_widget_draw(self->footer);
	}
}

static void
vkui_window_refresh(vkui_widget_t* widget, void* priv)
{
	// priv may be NULL
	ASSERT(widget);

	vkui_window_t* self = (vkui_window_t*) widget;
	vkui_widget_refresh((vkui_widget_t*) self->bullet);
	vkui_widget_refresh(self->body);
	if(self->footer)
	{
		vkui_widget_refresh(self->footer);
	}
}

/***********************************************************
* public                                                   *
***********************************************************/

vkui_window_t*
vkui_window_new(vkui_screen_t* screen, size_t wsize,
                vkui_widgetLayout_t* layout,
                vkui_widgetFn_t* fn,
                vkui_windowStyle_t* style,
                const char** sprite_array,
                vkui_widget_t* body,
                vkui_widget_t* footer)
{
	// footer may be NULL
	ASSERT(screen);
	ASSERT(layout);
	ASSERT(fn);
	ASSERT(style);
	ASSERT(sprite_array);
	ASSERT(body);

	if(wsize == 0)
	{
		wsize = sizeof(vkui_window_t);
	}

	vkui_widgetScroll_t scroll =
	{
		.scroll_bar = 0
	};

	vkui_widgetFn_t viewbox_fn =
	{
		.click_fn   = vkui_window_click,
		.refresh_fn = vkui_window_refresh
	};

	vkui_widgetPrivFn_t priv_fn =
	{
		.size_fn      = vkui_window_size,
		.layout_fn    = vkui_window_layout,
		.drag_fn      = vkui_window_drag,
		.scrollTop_fn = vkui_window_scrollTop,
		.draw_fn      = vkui_window_draw,
	};

	cc_vec4f_t clear =
	{
		.a = 0.0f
	};

	vkui_window_t* self;
	self = (vkui_window_t*)
	       vkui_widget_new(screen, wsize, &clear,
	                       layout, &scroll, &viewbox_fn,
	                       &priv_fn);
	if(self == NULL)
	{
		return NULL;
	}
	vkui_widget_soundFx((vkui_widget_t*) self, 0);

	if(vkui_widget_tricolor((vkui_widget_t*) self,
	                        &style->color_header,
	                        &style->color_body,
	                        &style->color_footer) == 0)
	{
		goto fail_tricolor;
	}

	self->bullet = vkui_bulletbox_new(screen, 0,
	                                  VKUI_WIDGET_ANCHOR_TL,
	                                  fn,
	                                  &style->bulletbox_style,
	                                  sprite_array);
	if(self->bullet == NULL)
	{
		goto fail_bullet;
	}

	self->body   = body;
	self->footer = footer;

	if(style->color_body.a == 0.0f)
	{
		self->transparent = 1;
	}

	// success
	return self;

	// failure
	fail_bullet:
	fail_tricolor:
		vkui_widget_delete((vkui_widget_t**) &self);
	return NULL;
}

void vkui_window_delete(vkui_window_t** _self)
{
	ASSERT(_self);

	vkui_window_t* self = *_self;
	if(self)
	{
		vkui_bulletbox_delete(&self->bullet);
		vkui_widget_delete((vkui_widget_t**) _self);
	}
}

void vkui_window_select(vkui_window_t* self,
                        uint32_t index)
{
	ASSERT(self);

	vkui_bulletbox_select(self->bullet, index);
}

void vkui_window_label(vkui_window_t* self,
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

	vkui_bulletbox_label(self->bullet, "%s", string);
}
