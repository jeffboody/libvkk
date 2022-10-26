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

#define LOG_TAG "vkk"
#include "../../libcc/cc_log.h"
#include "../vkk_ui.h"

/***********************************************************
* private                                                  *
***********************************************************/

static void
vkk_uiLayer_size(vkk_uiWidget_t* widget, float* w, float* h)
{
	ASSERT(widget);
	ASSERT(w);
	ASSERT(h);

	vkk_uiLayer_t* self = (vkk_uiLayer_t*) widget;
	cc_listIter_t* iter = cc_list_head(self->list);

	float wmax  = 0.0f;
	float hmax  = 0.0f;
	float tmp_w = 0.0f;
	float tmp_h = 0.0f;
	while(iter)
	{
		tmp_w  = *w;
		tmp_h  = *h;
		widget = (vkk_uiWidget_t*) cc_list_peekIter(iter);
		vkk_uiWidget_layoutSize(widget, &tmp_w, &tmp_h);

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

static vkk_uiWidget_t*
vkk_uiLayer_action(vkk_uiWidget_t* widget,
                   vkk_uiWidgetActionInfo_t* info)
{
	ASSERT(widget);
	ASSERT(info);

	// send events front-to-back
	vkk_uiLayer_t* self = (vkk_uiLayer_t*) widget;
	cc_listIter_t* iter = cc_list_head(self->list);
	while(iter)
	{
		vkk_uiWidget_t* tmp;
		tmp = (vkk_uiWidget_t*) cc_list_peekIter(iter);

		tmp = vkk_uiWidget_action(tmp, info);
		if(tmp)
		{
			return tmp;
		}

		iter = cc_list_next(iter);
	}

	// only children receive actions
	return NULL;
}

static void
vkk_uiLayer_layout(vkk_uiWidget_t* widget,
                   int dragx, int dragy)
{
	ASSERT(widget);

	// the rect_clip is constant across all layers
	cc_rect1f_t rect_clip;
	cc_rect1f_intersect(&widget->rect_draw,
	                    &widget->rect_clip,
	                    &rect_clip);

	vkk_uiLayer_t* self = (vkk_uiLayer_t*) widget;
	cc_listIter_t* iter = cc_list_head(self->list);
	while(iter)
	{
		vkk_uiWidget_t* child;
		child = (vkk_uiWidget_t*) cc_list_peekIter(iter);

		// layout the layer
		float x = 0.0f;
		float y = 0.0f;
		vkk_uiWidget_layoutAnchor(child, &widget->rect_draw,
		                          &x, &y);
		vkk_uiWidget_layoutXYClip(child, x, y, &rect_clip,
		                          dragx, dragy);

		iter = cc_list_next(iter);
	}
}

static void
vkk_uiLayer_drag(vkk_uiWidget_t* widget,
                 float x, float y, float dx, float dy)
{
	ASSERT(widget);

	vkk_uiLayer_t* self = (vkk_uiLayer_t*) widget;
	cc_listIter_t* iter = cc_list_head(self->list);
	while(iter)
	{
		vkk_uiWidget_t* tmp;
		tmp = (vkk_uiWidget_t*) cc_list_peekIter(iter);
		vkk_uiWidget_drag(tmp, x, y, dx, dy);
		iter = cc_list_next(iter);
	}
}

static void vkk_uiLayer_draw(vkk_uiWidget_t* widget)
{
	ASSERT(widget);

	// draw back-to-front
	vkk_uiLayer_t* self = (vkk_uiLayer_t*) widget;
	cc_listIter_t* iter = cc_list_tail(self->list);
	while(iter)
	{
		vkk_uiWidget_t* tmp;
		tmp = (vkk_uiWidget_t*) cc_list_peekIter(iter);
		vkk_uiWidget_draw(tmp);
		iter = cc_list_prev(iter);
	}
}

static int
vkk_uiLayer_refresh(vkk_uiWidget_t* widget)
{
	ASSERT(widget);

	vkk_uiLayer_t* self = (vkk_uiLayer_t*) widget;

	vkk_uiWidgetRefresh_fn refresh_fn = self->refresh_fn;
	if(refresh_fn && ((*refresh_fn)(widget) == 0))
	{
		return 0;
	}

	cc_listIter_t* iter = cc_list_head(self->list);
	while(iter)
	{
		vkk_uiWidget_t* tmp;
		tmp = (vkk_uiWidget_t*) cc_list_peekIter(iter);
		vkk_uiWidget_refresh(tmp);
		iter = cc_list_next(iter);
	}

	return 1;
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_uiLayer_t*
vkk_uiLayer_new(vkk_uiScreen_t* screen, size_t wsize,
                vkk_uiLayerFn_t* lfn,
                vkk_uiWidgetLayout_t* layout,
                cc_vec4f_t* color)
{
	ASSERT(screen);
	ASSERT(lfn);
	ASSERT(layout);
	ASSERT(color);

	if(wsize == 0)
	{
		wsize = sizeof(vkk_uiLayer_t);
	}

	vkk_uiWidgetFn_t fn =
	{
		.priv       = lfn->priv,
		.action_fn  = vkk_uiLayer_action,
		.drag_fn    = vkk_uiLayer_drag,
		.draw_fn    = vkk_uiLayer_draw,
		.layout_fn  = vkk_uiLayer_layout,
		.refresh_fn = vkk_uiLayer_refresh,
		.size_fn    = vkk_uiLayer_size,
	};

	vkk_uiWidgetScroll_t scroll =
	{
		.scroll_bar = 0
	};

	vkk_uiLayer_t* self;
	self = (vkk_uiLayer_t*)
	       vkk_uiWidget_new(screen, wsize, color, layout,
	                        &scroll, &fn);
	if(self == NULL)
	{
		return NULL;
	}

	self->list = cc_list_new();
	if(self->list == NULL)
	{
		goto fail_list;
	}

	self->refresh_fn = lfn->refresh_fn;

	// success
	return self;

	// failure
	fail_list:
		vkk_uiWidget_delete((vkk_uiWidget_t**) &self);
	return NULL;
}

void vkk_uiLayer_delete(vkk_uiLayer_t** _self)
{
	ASSERT(_self);

	vkk_uiLayer_t* self = *_self;
	if(self)
	{
		cc_list_delete(&self->list);
		vkk_uiWidget_delete((vkk_uiWidget_t**) _self);
	}
}

void vkk_uiLayer_clear(vkk_uiLayer_t* self)
{
	ASSERT(self);

	vkk_uiWidget_t* widget = &self->base;

	cc_list_discard(self->list);
	vkk_uiWidget_scrollTop(widget);
	vkk_uiScreen_layoutDirty(widget->screen);
}

int vkk_uiLayer_add(vkk_uiLayer_t* self,
                    vkk_uiWidget_t* widget)
{
	ASSERT(self);
	ASSERT(widget);

	if(cc_list_append(self->list, NULL,
	                  (const void*) widget) == 0)
	{
		return 0;
	}

	vkk_uiWidget_scrollTop(widget);
	vkk_uiScreen_layoutDirty(widget->screen);

	return 1;
}

cc_listIter_t* vkk_uiLayer_head(vkk_uiLayer_t* self)
{
	ASSERT(self);

	return cc_list_head(self->list);
}

vkk_uiWidget_t* vkk_uiLayer_remove(vkk_uiLayer_t* self,
                                  cc_listIter_t** _iter)
{
	ASSERT(self);
	ASSERT(_iter);

	vkk_uiWidget_t* widget = &self->base;

	vkk_uiWidget_scrollTop(widget);
	vkk_uiScreen_layoutDirty(widget->screen);

	return (vkk_uiWidget_t*)
	       cc_list_remove(self->list, _iter);
}
