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

#define LOG_TAG "vkk"
#include "../../libcc/cc_log.h"
#include "../vkk_ui.h"

/***********************************************************
* private                                                  *
***********************************************************/

static void
vkk_uiListbox_size(vkk_uiWidget_t* widget, float* w, float* h)
{
	ASSERT(widget);
	ASSERT(w);
	ASSERT(h);

	vkk_uiListbox_t*      self   = (vkk_uiListbox_t*) widget;
	cc_listIter_t*        iter   = cc_list_head(self->list);
	vkk_uiWidgetLayout_t* layout = &widget->layout;

	float cnt = (float) cc_list_size(self->list);
	float dw  = *w/cnt;
	float dh  = *h/cnt;
	if(self->orientation == VKK_UI_LISTBOX_ORIENTATION_VERTICAL)
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
	float wmax  = 0.0f;
	float hmax  = 0.0f;
	float wsum  = 0.0f;
	float hsum  = 0.0f;
	float tmp_w = 0.0f;
	float tmp_h = 0.0f;
	vkk_uiWidget_t*       tmp_widget;
	vkk_uiWidgetLayout_t* tmp_layout;
	if((layout->wrapx == VKK_UI_WIDGET_WRAP_SHRINK) &&
	   (self->orientation == VKK_UI_LISTBOX_ORIENTATION_VERTICAL))
	{
		// first pass computes size based on widgets
		// that are not stretch parent
		while(iter)
		{
			tmp_w  = dw;
			tmp_h  = dh;
			tmp_widget = (vkk_uiWidget_t*) cc_list_peekIter(iter);
			tmp_layout = &tmp_widget->layout;
			if(tmp_layout->wrapx == VKK_UI_WIDGET_WRAP_STRETCH_PARENT)
			{
				iter = cc_list_next(iter);
				continue;
			}

			vkk_uiWidget_layoutSize(tmp_widget, &tmp_w, &tmp_h);

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
			tmp_widget = (vkk_uiWidget_t*) cc_list_peekIter(iter);
			tmp_layout = &tmp_widget->layout;
			if(tmp_layout->wrapx != VKK_UI_WIDGET_WRAP_STRETCH_PARENT)
			{
				iter = cc_list_next(iter);
				continue;
			}

			vkk_uiWidget_layoutSize(tmp_widget, &tmp_w, &tmp_h);

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
			tmp_widget = (vkk_uiWidget_t*) cc_list_peekIter(iter);
			vkk_uiWidget_layoutSize(tmp_widget, &tmp_w, &tmp_h);

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

	if(self->orientation == VKK_UI_LISTBOX_ORIENTATION_HORIZONTAL)
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
vkk_uiListbox_click(vkk_uiWidget_t* widget,
                    int state, float x, float y)
{
	ASSERT(widget);

	vkk_uiListbox_t* self = (vkk_uiListbox_t*) widget;
	cc_listIter_t*   iter = cc_list_head(self->list);
	while(iter)
	{
		vkk_uiWidget_t* tmp;
		tmp = (vkk_uiWidget_t*) cc_list_peekIter(iter);
		if(vkk_uiWidget_click(tmp, state, x, y))
		{
			return 1;
		}

		iter = cc_list_next(iter);
	}

	// listboxes are always clicked
	return 1;
}

static void
vkk_uiListbox_layoutVerticalShrink(vkk_uiListbox_t* self,
                                   int dragx, int dragy)
{
	ASSERT(self);

	vkk_uiWidget_t* base = &self->base;

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
		vkk_uiWidget_t* child;
		child = (vkk_uiWidget_t*) cc_list_peekIter(iter);

		float h = child->rect_border.h;

		rect_draw.t = t;
		rect_draw.l = l;
		rect_draw.w = w;
		rect_draw.h = h;
		t += h;

		vkk_uiWidget_layoutAnchor(child, &rect_draw, &x, &y);
		cc_rect1f_intersect(&rect_draw,
		                    &base->rect_clip,
		                    &rect_clip);
		vkk_uiWidget_layoutXYClip(child, x, y, &rect_clip,
		                          dragx, dragy);

		iter = cc_list_next(iter);
	}
}

static void
vkk_uiListbox_layoutVerticalStretch(vkk_uiListbox_t* self,
                                    int dragx, int dragy)
{
	ASSERT(self);

	vkk_uiWidget_t* base = &self->base;

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
		vkk_uiWidget_t* child;
		child = (vkk_uiWidget_t*) cc_list_peekIter(iter);

		rect_draw.t = t;
		rect_draw.l = l;
		rect_draw.w = w;
		rect_draw.h = dh;
		t += dh;

		vkk_uiWidget_layoutAnchor(child, &rect_draw, &x, &y);
		cc_rect1f_intersect(&rect_draw,
		                    &base->rect_clip,
		                    &rect_clip);
		vkk_uiWidget_layoutXYClip(child, x, y, &rect_clip,
		                          dragx, dragy);

		iter = cc_list_next(iter);
	}
}

static void
vkk_uiListbox_layoutHorizontalShrink(vkk_uiListbox_t* self,
                                     int dragx, int dragy)
{
	ASSERT(self);

	vkk_uiWidget_t* base = &self->base;

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
		vkk_uiWidget_t* child;
		child = (vkk_uiWidget_t*) cc_list_peekIter(iter);

		float w = child->rect_border.w;

		rect_draw.t = t;
		rect_draw.l = l;
		rect_draw.w = w;
		rect_draw.h = h;
		l += w;

		vkk_uiWidget_layoutAnchor(child, &rect_draw, &x, &y);
		cc_rect1f_intersect(&rect_draw,
		                    &base->rect_clip,
		                    &rect_clip);
		vkk_uiWidget_layoutXYClip(child, x, y, &rect_clip,
		                          dragx, dragy);

		iter = cc_list_next(iter);
	}
}

static void
vkk_uiListbox_layoutHorizontalStretch(vkk_uiListbox_t* self,
                                      int dragx, int dragy)
{
	ASSERT(self);

	vkk_uiWidget_t* base = &self->base;

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
		vkk_uiWidget_t* child;
		child = (vkk_uiWidget_t*) cc_list_peekIter(iter);

		rect_draw.t = t;
		rect_draw.l = l;
		rect_draw.w = dw;
		rect_draw.h = h;
		l += dw;

		vkk_uiWidget_layoutAnchor(child, &rect_draw, &x, &y);
		cc_rect1f_intersect(&rect_draw,
		                    &base->rect_clip,
		                    &rect_clip);
		vkk_uiWidget_layoutXYClip(child, x, y, &rect_clip,
		                          dragx, dragy);

		iter = cc_list_next(iter);
	}
}

static void
vkk_uiListbox_layout(vkk_uiWidget_t* widget,
                     int dragx, int dragy)
{
	ASSERT(widget);

	vkk_uiWidgetLayout_t* layout = &widget->layout;

	vkk_uiListbox_t* self = (vkk_uiListbox_t*) widget;
	if(cc_list_size(self->list) == 0)
	{
		return;
	}

	if(self->orientation == VKK_UI_LISTBOX_ORIENTATION_VERTICAL)
	{
		if(layout->wrapy == VKK_UI_WIDGET_WRAP_SHRINK)
		{
			vkk_uiListbox_layoutVerticalShrink(self, dragx, dragy);
		}
		else
		{
			vkk_uiListbox_layoutVerticalStretch(self, dragx, dragy);
		}
	}
	else
	{
		if(layout->wrapx == VKK_UI_WIDGET_WRAP_SHRINK)
		{
			vkk_uiListbox_layoutHorizontalShrink(self, dragx, dragy);
		}
		else
		{
			vkk_uiListbox_layoutHorizontalStretch(self, dragx, dragy);
		}
	}
}

static void
vkk_uiListbox_drag(vkk_uiWidget_t* widget,
                   float x, float y, float dx, float dy)
{
	ASSERT(widget);

	vkk_uiListbox_t* self = (vkk_uiListbox_t*) widget;
	cc_listIter_t*   iter = cc_list_head(self->list);
	while(iter)
	{
		vkk_uiWidget_t* tmp;
		tmp = (vkk_uiWidget_t*) cc_list_peekIter(iter);
		vkk_uiWidget_drag(tmp, x, y, dx, dy);
		iter = cc_list_next(iter);
	}
}

static void vkk_uiListbox_draw(vkk_uiWidget_t* widget)
{
	ASSERT(widget);

	vkk_uiListbox_t* self = (vkk_uiListbox_t*) widget;
	cc_listIter_t*  iter = cc_list_head(self->list);
	while(iter)
	{
		vkk_uiWidget_t* tmp;
		tmp = (vkk_uiWidget_t*) cc_list_peekIter(iter);
		vkk_uiWidget_draw(tmp);
		iter = cc_list_next(iter);
	}
}

static void
vkk_uiListbox_refresh(vkk_uiWidget_t* widget)
{
	ASSERT(widget);

	vkk_uiListbox_t* self = (vkk_uiListbox_t*) widget;
	cc_listIter_t*   iter = cc_list_head(self->list);
	while(iter)
	{
		vkk_uiWidget_t* tmp;
		tmp = (vkk_uiWidget_t*) cc_list_peekIter(iter);
		vkk_uiWidget_refresh(tmp);
		iter = cc_list_next(iter);
	}
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_uiListbox_t*
vkk_uiListbox_new(vkk_uiScreen_t* screen, size_t wsize,
                  vkk_uiWidgetLayout_t* layout,
                  vkk_uiWidgetScroll_t* scroll,
                  vkk_uiWidgetFn_t* fn, int orientation,
                  cc_vec4f_t* color)
{
	ASSERT(screen);
	ASSERT(layout);
	ASSERT(scroll);
	ASSERT(fn);
	ASSERT(color);

	if(wsize == 0)
	{
		wsize = sizeof(vkk_uiListbox_t);
	}

	// optionally set click/refresh functions
	vkk_uiWidgetFn_t list_fn;
	memcpy(&list_fn, fn, sizeof(vkk_uiWidgetFn_t));
	if(fn->click_fn == NULL)
	{
		list_fn.click_fn = vkk_uiListbox_click;
	}
	if(fn->refresh_fn == NULL)
	{
		list_fn.refresh_fn = vkk_uiListbox_refresh;
	}

	vkk_uiWidgetPrivFn_t priv_fn =
	{
		.size_fn   = vkk_uiListbox_size,
		.layout_fn = vkk_uiListbox_layout,
		.drag_fn   = vkk_uiListbox_drag,
		.draw_fn   = vkk_uiListbox_draw,
	};

	vkk_uiListbox_t* self;
	self = (vkk_uiListbox_t*)
	       vkk_uiWidget_new(screen, wsize, color, layout,
	                        scroll, &list_fn, &priv_fn);
	if(self == NULL)
	{
		return NULL;
	}

	vkk_uiWidget_t* base = &self->base;
	vkk_uiWidget_soundFx(base, 0);

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
		vkk_uiWidget_delete((vkk_uiWidget_t**) &self);
	return NULL;
}

void vkk_uiListbox_delete(vkk_uiListbox_t** _self)
{
	ASSERT(_self);

	vkk_uiListbox_t* self = *_self;
	if(self)
	{
		cc_list_delete(&self->list);
		vkk_uiWidget_delete((vkk_uiWidget_t**) _self);
	}
}

void vkk_uiListbox_clear(vkk_uiListbox_t* self)
{
	ASSERT(self);

	vkk_uiWidget_t* base = &self->base;

	cc_list_discard(self->list);
	vkk_uiWidget_scrollTop(base);
	vkk_uiScreen_dirty(base->screen);
}

int vkk_uiListbox_add(vkk_uiListbox_t* self,
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
	vkk_uiScreen_dirty(widget->screen);

	return 1;
}

int vkk_uiListbox_addSorted(vkk_uiListbox_t* self,
                            cc_listcmp_fn compare,
                            vkk_uiWidget_t* widget)
{
	ASSERT(self);
	ASSERT(compare);
	ASSERT(widget);

	if(cc_list_insertSorted(self->list, compare,
	                        (const void*) widget) == 0)
	{
		return 0;
	}

	vkk_uiWidget_scrollTop(widget);
	vkk_uiScreen_dirty(widget->screen);

	return 1;
}

cc_listIter_t* vkk_uiListbox_head(vkk_uiListbox_t* self)
{
	ASSERT(self);

	return cc_list_head(self->list);
}

vkk_uiWidget_t* vkk_uiListbox_remove(vkk_uiListbox_t* self,
                                     cc_listIter_t** _iter)
{
	ASSERT(self);
	ASSERT(_iter);

	vkk_uiWidget_t* base = &self->base;

	vkk_uiWidget_scrollTop(base);
	vkk_uiScreen_dirty(base->screen);

	return (vkk_uiWidget_t*)
	       cc_list_remove(self->list, _iter);
}