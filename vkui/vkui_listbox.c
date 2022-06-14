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
#include <string.h>

#define LOG_TAG "vkui"
#include "../../libcc/cc_log.h"
#include "../vkui.h"

/***********************************************************
* private                                                  *
***********************************************************/

static void
vkui_listbox_size(vkui_widget_t* widget, float* w, float* h)
{
	ASSERT(widget);
	ASSERT(w);
	ASSERT(h);

	vkui_listbox_t*      self   = (vkui_listbox_t*) widget;
	cc_listIter_t*       iter   = cc_list_head(self->list);
	vkui_widgetLayout_t* layout = &widget->layout;

	float cnt = (float) cc_list_size(self->list);
	float dw  = *w/cnt;
	float dh  = *h/cnt;
	if(self->orientation == VKUI_LISTBOX_ORIENTATION_VERTICAL)
	{
		dw = *w;
	}
	else
	{
		dh = *h;
	}

	// vertical lists that are shrink wrapped in x may have
	// widgets that are stretch parent (e.g. hlines) so
	// special handling is required to prevent the whole
	// list from being stretched to parent
	float                wmax  = 0.0f;
	float                hmax  = 0.0f;
	float                wsum  = 0.0f;
	float                hsum  = 0.0f;
	float                tmp_w = 0.0f;
	float                tmp_h = 0.0f;
	vkui_widget_t*       tmp_widget;
	vkui_widgetLayout_t* tmp_layout;
	if((layout->wrapx == VKUI_WIDGET_WRAP_SHRINK) &&
	   (self->orientation == VKUI_LISTBOX_ORIENTATION_VERTICAL))
	{
		// first pass computes size based on widgets
		// that are not stretch parent
		while(iter)
		{
			tmp_w  = dw;
			tmp_h  = dh;
			tmp_widget = (vkui_widget_t*) cc_list_peekIter(iter);
			tmp_layout = &tmp_widget->layout;
			if(tmp_layout->wrapx == VKUI_WIDGET_WRAP_STRETCH_PARENT)
			{
				iter = cc_list_next(iter);
				continue;
			}

			vkui_widget_layoutSize(tmp_widget, &tmp_w, &tmp_h);

			if(tmp_w > wmax)
			{
				wmax = tmp_w;
			}
			wsum += tmp_w;

			if(tmp_h > hmax)
			{
				hmax = tmp_h;
			}
			hsum += tmp_h;

			iter = cc_list_next(iter);
		}

		// second pass computes size of widgets that are
		// stretch parent
		iter  = cc_list_head(self->list);
		while(iter)
		{
			tmp_w  = wmax;
			tmp_h  = dh;
			tmp_widget = (vkui_widget_t*) cc_list_peekIter(iter);
			tmp_layout = &tmp_widget->layout;
			if(tmp_layout->wrapx != VKUI_WIDGET_WRAP_STRETCH_PARENT)
			{
				iter = cc_list_next(iter);
				continue;
			}

			vkui_widget_layoutSize(tmp_widget, &tmp_w, &tmp_h);

			if(tmp_w > wmax)
			{
				wmax = tmp_w;
			}
			wsum += tmp_w;

			if(tmp_h > hmax)
			{
				hmax = tmp_h;
			}
			hsum += tmp_h;

			iter = cc_list_next(iter);
		}
	}
	else
	{
		while(iter)
		{
			tmp_w  = dw;
			tmp_h  = dh;
			tmp_widget = (vkui_widget_t*) cc_list_peekIter(iter);
			vkui_widget_layoutSize(tmp_widget, &tmp_w, &tmp_h);

			if(tmp_w > wmax)
			{
				wmax = tmp_w;
			}
			wsum += tmp_w;

			if(tmp_h > hmax)
			{
				hmax = tmp_h;
			}
			hsum += tmp_h;

			iter = cc_list_next(iter);
		}
	}

	if(self->orientation == VKUI_LISTBOX_ORIENTATION_HORIZONTAL)
	{
		*w = wsum;
		*h = hmax;
	}
	else
	{
		*w = wmax;
		*h = hsum;
	}
}

static int
vkui_listbox_click(vkui_widget_t* widget,
                   int state, float x, float y)
{
	ASSERT(widget);

	vkui_listbox_t* self = (vkui_listbox_t*) widget;
	cc_listIter_t*  iter = cc_list_head(self->list);
	while(iter)
	{
		vkui_widget_t* tmp;
		tmp = (vkui_widget_t*) cc_list_peekIter(iter);
		if(vkui_widget_click(tmp, state, x, y))
		{
			return 1;
		}

		iter = cc_list_next(iter);
	}

	// listboxes are always clicked
	return 1;
}

static void
vkui_listbox_layoutVerticalShrink(vkui_listbox_t* self,
                                  int dragx, int dragy)
{
	ASSERT(self);

	vkui_widget_t* base = &self->base;

	// initialize the layout
	float x = 0.0f;
	float y = 0.0f;
	float t = base->rect_draw.t;
	float l = base->rect_draw.l;
	float w = base->rect_draw.w;
	cc_rect1f_t rect_clip;
	cc_rect1f_t rect_draw;

	cc_listIter_t* iter = cc_list_head(self->list);
	while(iter)
	{
		vkui_widget_t* child;
		child = (vkui_widget_t*) cc_list_peekIter(iter);

		float h = child->rect_border.h;

		rect_draw.t = t;
		rect_draw.l = l;
		rect_draw.w = w;
		rect_draw.h = h;
		t += h;

		vkui_widget_layoutAnchor(child, &rect_draw, &x, &y);
		cc_rect1f_intersect(&rect_draw,
		                    &base->rect_clip,
		                    &rect_clip);
		vkui_widget_layoutXYClip(child, x, y, &rect_clip,
		                         dragx, dragy);

		iter = cc_list_next(iter);
	}
}

static void
vkui_listbox_layoutVerticalStretch(vkui_listbox_t* self,
                                   int dragx, int dragy)
{
	ASSERT(self);

	vkui_widget_t* base = &self->base;

	// initialize the layout
	float x   = 0.0f;
	float y   = 0.0f;
	float t   = base->rect_draw.t;
	float l   = base->rect_draw.l;
	float w   = base->rect_draw.w;
	float h   = base->rect_draw.h;
	float cnt = (float) cc_list_size(self->list);
	float dh  = h/cnt;
	cc_rect1f_t rect_clip;
	cc_rect1f_t rect_draw;

	cc_listIter_t* iter = cc_list_head(self->list);
	while(iter)
	{
		vkui_widget_t* child;
		child = (vkui_widget_t*) cc_list_peekIter(iter);

		rect_draw.t = t;
		rect_draw.l = l;
		rect_draw.w = w;
		rect_draw.h = dh;
		t += dh;

		vkui_widget_layoutAnchor(child, &rect_draw, &x, &y);
		cc_rect1f_intersect(&rect_draw,
		                    &base->rect_clip,
		                    &rect_clip);
		vkui_widget_layoutXYClip(child, x, y, &rect_clip,
		                         dragx, dragy);

		iter = cc_list_next(iter);
	}
}

static void
vkui_listbox_layoutHorizontalShrink(vkui_listbox_t* self,
                                    int dragx, int dragy)
{
	ASSERT(self);

	vkui_widget_t* base = &self->base;

	// initialize the layout
	float x = 0.0f;
	float y = 0.0f;
	float t = base->rect_draw.t;
	float l = base->rect_draw.l;
	float h = base->rect_draw.h;
	cc_rect1f_t rect_clip;
	cc_rect1f_t rect_draw;

	cc_listIter_t* iter = cc_list_head(self->list);
	while(iter)
	{
		vkui_widget_t* child;
		child = (vkui_widget_t*) cc_list_peekIter(iter);

		float w = child->rect_border.w;

		rect_draw.t = t;
		rect_draw.l = l;
		rect_draw.w = w;
		rect_draw.h = h;
		l += w;

		vkui_widget_layoutAnchor(child, &rect_draw, &x, &y);
		cc_rect1f_intersect(&rect_draw,
		                    &base->rect_clip,
		                    &rect_clip);
		vkui_widget_layoutXYClip(child, x, y, &rect_clip,
		                         dragx, dragy);

		iter = cc_list_next(iter);
	}
}

static void
vkui_listbox_layoutHorizontalStretch(vkui_listbox_t* self,
                                     int dragx, int dragy)
{
	ASSERT(self);

	vkui_widget_t* base = &self->base;

	// initialize the layout
	float x   = 0.0f;
	float y   = 0.0f;
	float t   = base->rect_draw.t;
	float l   = base->rect_draw.l;
	float w   = base->rect_draw.w;
	float h   = base->rect_draw.h;
	float cnt = (float) cc_list_size(self->list);
	float dw  = w/cnt;
	cc_rect1f_t rect_clip;
	cc_rect1f_t rect_draw;

	cc_listIter_t* iter = cc_list_head(self->list);
	while(iter)
	{
		vkui_widget_t* child;
		child = (vkui_widget_t*) cc_list_peekIter(iter);

		rect_draw.t = t;
		rect_draw.l = l;
		rect_draw.w = dw;
		rect_draw.h = h;
		l += dw;

		vkui_widget_layoutAnchor(child, &rect_draw, &x, &y);
		cc_rect1f_intersect(&rect_draw,
		                    &base->rect_clip,
		                    &rect_clip);
		vkui_widget_layoutXYClip(child, x, y, &rect_clip,
		                         dragx, dragy);

		iter = cc_list_next(iter);
	}
}

static void
vkui_listbox_layout(vkui_widget_t* widget,
                    int dragx, int dragy)
{
	ASSERT(widget);

	vkui_widgetLayout_t* layout = &widget->layout;

	vkui_listbox_t* self = (vkui_listbox_t*) widget;
	if(cc_list_size(self->list) == 0)
	{
		return;
	}

	if(self->orientation == VKUI_LISTBOX_ORIENTATION_VERTICAL)
	{
		if(layout->wrapy == VKUI_WIDGET_WRAP_SHRINK)
		{
			vkui_listbox_layoutVerticalShrink(self, dragx, dragy);
		}
		else
		{
			vkui_listbox_layoutVerticalStretch(self, dragx, dragy);
		}
	}
	else
	{
		if(layout->wrapx == VKUI_WIDGET_WRAP_SHRINK)
		{
			vkui_listbox_layoutHorizontalShrink(self, dragx, dragy);
		}
		else
		{
			vkui_listbox_layoutHorizontalStretch(self, dragx, dragy);
		}
	}
}

static void
vkui_listbox_drag(vkui_widget_t* widget,
                  float x, float y, float dx, float dy)
{
	ASSERT(widget);

	vkui_listbox_t* self = (vkui_listbox_t*) widget;
	cc_listIter_t*  iter = cc_list_head(self->list);
	while(iter)
	{
		vkui_widget_t* tmp;
		tmp = (vkui_widget_t*) cc_list_peekIter(iter);
		vkui_widget_drag(tmp, x, y, dx, dy);
		iter = cc_list_next(iter);
	}
}

static void vkui_listbox_draw(vkui_widget_t* widget)
{
	ASSERT(widget);

	vkui_listbox_t* self = (vkui_listbox_t*) widget;
	cc_listIter_t*  iter = cc_list_head(self->list);
	while(iter)
	{
		vkui_widget_t* tmp;
		tmp = (vkui_widget_t*) cc_list_peekIter(iter);
		vkui_widget_draw(tmp);
		iter = cc_list_next(iter);
	}
}

static void
vkui_listbox_refresh(vkui_widget_t* widget)
{
	ASSERT(widget);

	vkui_listbox_t* self = (vkui_listbox_t*) widget;
	cc_listIter_t*  iter = cc_list_head(self->list);
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

vkui_listbox_t*
vkui_listbox_new(vkui_screen_t* screen, size_t wsize,
                 vkui_widgetLayout_t* layout,
                 vkui_widgetScroll_t* scroll,
                 vkui_widgetFn_t* fn, int orientation,
                 cc_vec4f_t* color)
{
	ASSERT(screen);
	ASSERT(layout);
	ASSERT(scroll);
	ASSERT(fn);
	ASSERT(color);

	if(wsize == 0)
	{
		wsize = sizeof(vkui_listbox_t);
	}

	// optionally set click/refresh functions
	vkui_widgetFn_t list_fn;
	memcpy(&list_fn, fn, sizeof(vkui_widgetFn_t));
	if(fn->click_fn == NULL)
	{
		list_fn.click_fn = vkui_listbox_click;
	}
	if(fn->refresh_fn == NULL)
	{
		list_fn.refresh_fn = vkui_listbox_refresh;
	}

	vkui_widgetPrivFn_t priv_fn =
	{
		.size_fn   = vkui_listbox_size,
		.layout_fn = vkui_listbox_layout,
		.drag_fn   = vkui_listbox_drag,
		.draw_fn   = vkui_listbox_draw,
	};

	vkui_listbox_t* self;
	self = (vkui_listbox_t*)
	       vkui_widget_new(screen, wsize, color, layout,
	                       scroll, &list_fn, &priv_fn);
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

	self->orientation = orientation;

	// success
	return self;

	// failure
	fail_list:
		vkui_widget_delete((vkui_widget_t**) &self);
	return NULL;
}

void vkui_listbox_delete(vkui_listbox_t** _self)
{
	ASSERT(_self);

	vkui_listbox_t* self = *_self;
	if(self)
	{
		cc_list_delete(&self->list);
		vkui_widget_delete((vkui_widget_t**) _self);
	}
}

void vkui_listbox_clear(vkui_listbox_t* self)
{
	ASSERT(self);

	vkui_widget_t* base = &self->base;

	cc_list_discard(self->list);
	vkui_widget_scrollTop(base);
	vkui_screen_dirty(base->screen);
}

int vkui_listbox_add(vkui_listbox_t* self,
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

int vkui_listbox_addSorted(vkui_listbox_t* self,
                           cc_listcmp_fn compare,
                           vkui_widget_t* widget)
{
	ASSERT(self);
	ASSERT(compare);
	ASSERT(widget);

	if(cc_list_insertSorted(self->list, compare,
	                        (const void*) widget) == 0)
	{
		return 0;
	}

	vkui_widget_scrollTop(widget);
	vkui_screen_dirty(widget->screen);

	return 1;
}

cc_listIter_t* vkui_listbox_head(vkui_listbox_t* self)
{
	ASSERT(self);

	return cc_list_head(self->list);
}

vkui_widget_t* vkui_listbox_remove(vkui_listbox_t* self,
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
