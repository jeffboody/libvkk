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

#include <stdlib.h>

#define LOG_TAG "vkui"
#include "../../libcc/cc_log.h"
#include "vkui_layer.h"
#include "vkui_screen.h"
#include "vkui_widget.h"

/***********************************************************
* private                                                  *
***********************************************************/

static void
vkui_layer_size(vkui_widget_t* widget, float* w, float* h)
{
	ASSERT(widget);
	ASSERT(w);
	ASSERT(h);

	vkui_layer_t*  self = (vkui_layer_t*) widget;
	cc_listIter_t* iter = cc_list_head(self->list);

	float wmax  = 0.0f;
	float hmax  = 0.0f;
	float tmp_w = 0.0f;
	float tmp_h = 0.0f;
	while(iter)
	{
		tmp_w  = *w;
		tmp_h  = *h;
		widget = (vkui_widget_t*) cc_list_peekIter(iter);
		vkui_widget_layoutSize(widget, &tmp_w, &tmp_h);

		if(tmp_w > wmax)
		{
			wmax = tmp_w;
		}

		if(tmp_h > hmax)
		{
			hmax = tmp_h;
		}

		iter = cc_list_next(iter);
	}

	*w = wmax;
	*h = hmax;
}

static int
vkui_layer_click(vkui_widget_t* widget, void* priv,
                 int state, float x, float y)
{
	ASSERT(widget);
	ASSERT(priv == NULL);

	// send events front-to-back
	vkui_layer_t*  self = (vkui_layer_t*) widget;
	cc_listIter_t* iter = cc_list_head(self->list);
	while(iter)
	{
		widget = (vkui_widget_t*) cc_list_peekIter(iter);
		if(vkui_widget_click(widget, state, x, y))
		{
			return 1;
		}

		iter = cc_list_next(iter);
	}

	// layers are only clicked if a child is clicked
	return 0;
}

static void
vkui_layer_layout(vkui_widget_t* widget,
                  int dragx, int dragy)
{
	ASSERT(widget);

	// the rect_clip is constant across all layers
	cc_rect1f_t rect_clip;
	cc_rect1f_intersect(&widget->rect_draw,
	                    &widget->rect_clip,
	                    &rect_clip);

	vkui_layer_t*  self = (vkui_layer_t*) widget;
	cc_listIter_t* iter = cc_list_head(self->list);
	while(iter)
	{
		vkui_widget_t* child;
		child = (vkui_widget_t*) cc_list_peekIter(iter);

		// layout the layer
		float x = 0.0f;
		float y = 0.0f;
		vkui_widget_layoutAnchor(child, &widget->rect_draw,
		                         &x, &y);
		vkui_widget_layoutXYClip(child, x, y, &rect_clip,
		                         dragx, dragy);

		iter = cc_list_next(iter);
	}
}

static void
vkui_layer_drag(vkui_widget_t* widget,
                float x, float y, float dx, float dy)
{
	ASSERT(widget);

	vkui_layer_t*  self = (vkui_layer_t*) widget;
	cc_listIter_t* iter = cc_list_head(self->list);
	while(iter)
	{
		vkui_widget_t* tmp;
		tmp = (vkui_widget_t*) cc_list_peekIter(iter);
		vkui_widget_drag(tmp, x, y, dx, dy);
		iter = cc_list_next(iter);
	}
}

static void vkui_layer_draw(vkui_widget_t* widget)
{
	ASSERT(widget);

	// draw back-to-front
	vkui_layer_t*  self = (vkui_layer_t*) widget;
	cc_listIter_t* iter = cc_list_tail(self->list);
	while(iter)
	{
		vkui_widget_t* tmp;
		tmp = (vkui_widget_t*) cc_list_peekIter(iter);
		vkui_widget_draw(tmp);
		iter = cc_list_prev(iter);
	}
}

static void
vkui_layer_refresh(vkui_widget_t* widget, void* priv)
{
	ASSERT(widget);
	ASSERT(priv == NULL);

	vkui_layer_t*  self = (vkui_layer_t*) widget;
	cc_listIter_t* iter = cc_list_head(self->list);
	while(iter)
	{
		vkui_widget_t* tmp;
		tmp = (vkui_widget_t*) cc_list_peekIter(iter);
		vkui_widget_refresh(tmp);
		iter = cc_list_next(iter);
	}
}

/***********************************************************
* public                                                   *
***********************************************************/

vkui_layer_t*
vkui_layer_new(vkui_screen_t* screen, size_t wsize,
               vkui_widgetLayout_t* layout,
               cc_vec4f_t* color)
{
	ASSERT(screen);
	ASSERT(layout);
	ASSERT(color);

	if(wsize == 0)
	{
		wsize = sizeof(vkui_layer_t);
	}

	vkui_widgetFn_t layer_fn =
	{
		.click_fn   = vkui_layer_click,
		.refresh_fn = vkui_layer_refresh
	};

	vkui_widgetScroll_t scroll =
	{
		.scroll_bar = 0
	};

	vkui_widgetPrivFn_t priv_fn =
	{
		.size_fn    = vkui_layer_size,
		.layout_fn  = vkui_layer_layout,
		.drag_fn    = vkui_layer_drag,
		.draw_fn    = vkui_layer_draw,
	};

	vkui_layer_t* self;
	self = (vkui_layer_t*)
	       vkui_widget_new(screen, wsize, color, layout,
	                      &scroll, &layer_fn, &priv_fn);
	if(self == NULL)
	{
		return NULL;
	}

	vkui_widget_t* base = &self->base;
	vkui_widget_soundFx(base, 0);

	self->list = cc_list_new();
	if(self->list == NULL)
	{
		goto fail_list;
	}

	// success
	return self;

	// failure
	fail_list:
		vkui_widget_delete((vkui_widget_t**) &self);
	return NULL;
}

void vkui_layer_delete(vkui_layer_t** _self)
{
	ASSERT(_self);

	vkui_layer_t* self = *_self;
	if(self)
	{
		cc_list_delete(&self->list);
		vkui_widget_delete((vkui_widget_t**) _self);
	}
}

void vkui_layer_clear(vkui_layer_t* self)
{
	ASSERT(self);

	vkui_widget_t* base = &self->base;

	cc_list_discard(self->list);
	vkui_widget_scrollTop(base);
	vkui_screen_dirty(base->screen);
}

int vkui_layer_add(vkui_layer_t* self,
                   vkui_widget_t* widget)
{
	ASSERT(self);
	ASSERT(widget);

	if(cc_list_append(self->list, NULL,
	                  (const void*) widget) == 0)
	{
		return 0;
	}

	vkui_widget_scrollTop(widget);
	vkui_screen_dirty(widget->screen);

	return 1;
}

cc_listIter_t* vkui_layer_head(vkui_layer_t* self)
{
	ASSERT(self);

	return cc_list_head(self->list);
}

vkui_widget_t* vkui_layer_remove(vkui_layer_t* self,
                                 cc_listIter_t** _iter)
{
	ASSERT(self);
	ASSERT(_iter);

	vkui_widget_t* base = &self->base;

	vkui_widget_scrollTop(base);
	vkui_screen_dirty(base->screen);

	return (vkui_widget_t*)
	       cc_list_remove(self->list, _iter);
}
