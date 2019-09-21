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

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define LOG_TAG "vkui"
#include "../../libcc/cc_log.h"
#include "vkui_bulletbox.h"
#include "vkui_screen.h"
#include "vkui_viewbox.h"
#include "vkui_widget.h"

/***********************************************************
* private                                                  *
***********************************************************/

static void
vkui_viewbox_size(vkui_widget_t* widget, float* w, float* h)
{
	assert(widget);
	assert(w);
	assert(h);

	vkui_widgetLayout_t* layout = &widget->layout;
	vkui_viewbox_t*      self   = (vkui_viewbox_t*) widget;
	vkui_widget_t*       bullet = (vkui_widget_t*) self->bullet;
	vkui_widget_t*       body   = self->body;
	vkui_widget_t*       footer = self->footer;

	float wmax     = 0.0f;
	float hsum     = 0.0f;
	float bullet_w = *w;
	float bullet_h = *h;

	// layout bullet
	vkui_widget_layoutSize(bullet, &bullet_w, &bullet_h);
	wmax = bullet_w;
	hsum = bullet_h;

	// layout separator(s)
	float h_bo = 0.0f;
	float v_bo = 0.0f;
	vkui_screen_layoutBorder(widget->screen, layout->border,
	                         &h_bo, &v_bo);
	if(footer)
	{
		hsum += 4.0f*v_bo;
	}
	else
	{
		hsum += 2.0f*v_bo;
	}

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
	float body_h = *h - hsum ;
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
vkui_viewbox_click(vkui_widget_t* widget, void* priv,
                   int state, float x, float y)
{
	// priv may be NULL
	assert(widget);

	vkui_viewbox_t* self = (vkui_viewbox_t*) widget;
	vkui_widget_t*  bullet = (vkui_widget_t*) self->bullet;
	vkui_widget_t*  body   = (vkui_widget_t*) self->body;
	vkui_widget_t*  footer = (vkui_widget_t*) self->footer;
	if(vkui_widget_click(bullet, state, x, y) ||
	   vkui_widget_click(body, state, x, y)   ||
	   (footer && vkui_widget_click(footer, state, x, y)))
	{
		return 1;
	}

	// viewboxes are always clicked
	return 1;
}

static void
vkui_viewbox_layout(vkui_widget_t* widget,
                    int dragx, int dragy)
{
	assert(widget);

	vkui_widgetLayout_t* layout = &widget->layout;
	vkui_viewbox_t*      self   = (vkui_viewbox_t*) widget;
	vkui_widget_t*       bullet = (vkui_widget_t*) self->bullet;
	vkui_widget_t*       body   = self->body;
	vkui_widget_t*       footer = self->footer;

	// initialize the layout
	float x  = 0.0f;
	float y  = 0.0f;
	float t  = self->widget.rect_draw.t;
	float l  = self->widget.rect_draw.l;
	float w  = self->widget.rect_draw.w;
	float h  = self->widget.rect_draw.h;
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
		rect_draw.t = t + bullet_h + 2.0f*v_bo;
		rect_draw.h = h - bullet_h - 4.0f*v_bo - footer_h;
	}
	else
	{
		rect_draw.t = t + bullet_h + 2.0f*v_bo;
		rect_draw.h = h - bullet_h - 2.0f*v_bo;
	}
	vkui_widget_layoutAnchor(body, &rect_draw, &x, &y);
	cc_rect1f_intersect(&rect_draw,
	                    &self->widget.rect_clip,
	                    &rect_clip);
	vkui_widget_layoutXYClip(body, x, y, &rect_clip,
	                         dragx, dragy);

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
	}
}

static void
vkui_viewbox_drag(vkui_widget_t* widget, float x, float y,
                  float dx, float dy)
{
	assert(widget);

	vkui_viewbox_t* self = (vkui_viewbox_t*) widget;
	vkui_widget_drag((vkui_widget_t*) self->bullet,
	                 x, y, dx, dy);
	vkui_widget_drag(self->body, x, y, dx, dy);
	if(self->footer)
	{
		vkui_widget_drag(self->footer, x, y, dx, dy);
	}
}

static void
vkui_viewbox_scrollTop(vkui_widget_t* widget)
{
	assert(widget);

	vkui_viewbox_t* self = (vkui_viewbox_t*) widget;

	vkui_widget_scrollTop(self->body);
}

static void
vkui_viewbox_draw(vkui_widget_t* widget)
{
	assert(widget);

	vkui_widgetLayout_t* layout = &widget->layout;

	// bullet separator y
	float h_bo = 0.0f;
	float v_bo = 0.0f;
	vkui_viewbox_t* self = (vkui_viewbox_t*) widget;
	vkui_screen_layoutBorder(widget->screen,
	                         layout->border,
	                         &h_bo, &v_bo);
	vkui_widget_t* w = &(self->bullet->widget);
	float          y = w->rect_border.t +
	                   w->rect_border.h + v_bo;

	vkui_widget_draw((vkui_widget_t*) self->bullet);
	vkui_widget_draw(self->body);
	if(self->footer)
	{
		vkui_widget_draw(self->footer);
	}
	vkui_widget_headerY(widget, y);
}

static void
vkui_viewbox_refresh(vkui_widget_t* widget, void* priv)
{
	// priv may be NULL
	assert(widget);

	vkui_viewbox_t* self = (vkui_viewbox_t*) widget;
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

vkui_viewbox_t*
vkui_viewbox_new(vkui_screen_t* screen, size_t wsize,
                 vkui_widgetLayout_t* layout,
                 vkui_widgetFn_t* fn,
                 vkui_viewboxStyle_t* style,
                 const char** sprite_array,
                 vkui_widget_t* body,
                 vkui_widget_t* footer)
{
	// footer may be NULL
	assert(screen);
	assert(layout);
	assert(fn);
	assert(style);
	assert(sprite_array);
	assert(body);

	if(wsize == 0)
	{
		wsize = sizeof(vkui_viewbox_t);
	}

	vkui_widgetScroll_t scroll =
	{
		.scroll_bar = 0
	};

	vkui_widgetFn_t viewbox_fn =
	{
		.click_fn   = vkui_viewbox_click,
		.refresh_fn = vkui_viewbox_refresh
	};

	vkui_widgetPrivFn_t priv_fn =
	{
		.size_fn      = vkui_viewbox_size,
		.layout_fn    = vkui_viewbox_layout,
		.drag_fn      = vkui_viewbox_drag,
		.scrollTop_fn = vkui_viewbox_scrollTop,
		.draw_fn      = vkui_viewbox_draw,
	};

	// TODO - use tricolor widget
	vkui_viewbox_t* self;
	self = (vkui_viewbox_t*)
	       vkui_widget_new(screen, wsize, &style->color_body,
	                       layout, &scroll, &viewbox_fn,
	                       &priv_fn);
	if(self == NULL)
	{
		return NULL;
	}
	vkui_widget_soundFx((vkui_widget_t*) self, 0);

	self->bullet = vkui_bulletbox_new(screen, 0, fn,
	                                  &style->text_style,
	                                  sprite_array);
	if(self->bullet == NULL)
	{
		goto fail_bullet;
	}

	self->body   = body;
	self->footer = footer;

	// success
	return self;

	// failure
	fail_bullet:
		vkui_widget_delete((vkui_widget_t**) &self);
	return NULL;
}

void vkui_viewbox_delete(vkui_viewbox_t** _self)
{
	assert(_self);

	vkui_viewbox_t* self = *_self;
	if(self)
	{
		vkui_bulletbox_delete(&self->bullet);
		vkui_widget_delete((vkui_widget_t**) _self);
	}
}

void vkui_viewbox_select(vkui_viewbox_t* self,
                         uint32_t index)
{
	assert(self);

	vkui_bulletbox_select(self->bullet, index);
}

void vkui_viewbox_label(vkui_viewbox_t* self,
                        const char* fmt, ...)
{
	assert(self);
	assert(fmt);

	// decode string
	char    string[256];
	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(string, 256, fmt, argptr);
	va_end(argptr);

	vkui_bulletbox_label(self->bullet, "%s", string);
}