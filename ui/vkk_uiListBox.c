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
vkk_uiListBox_size(vkk_uiWidget_t* widget, float* w, float* h)
{
	ASSERT(widget);
	ASSERT(w);
	ASSERT(h);

	vkk_uiListBox_t*      self   = (vkk_uiListBox_t*) widget;
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

static vkk_uiWidget_t*
vkk_uiListBox_action(vkk_uiWidget_t* widget,
                     vkk_uiWidgetActionInfo_t* info)
{
	ASSERT(widget);
	ASSERT(info);

	vkk_uiListBox_t* self = (vkk_uiListBox_t*) widget;
	cc_listIter_t*   iter = cc_list_head(self->list);
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

	// receive DOWN action for scrolling
	if(info->action == VKK_UI_WIDGET_ACTION_DOWN)
	{
		return widget;
	}

	return NULL;
}

static void
vkk_uiListBox_layoutVerticalShrink(vkk_uiListBox_t* self,
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
vkk_uiListBox_layoutVerticalStretch(vkk_uiListBox_t* self,
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
vkk_uiListBox_layoutHorizontalShrink(vkk_uiListBox_t* self,
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
vkk_uiListBox_layoutHorizontalStretch(vkk_uiListBox_t* self,
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
vkk_uiListBox_layout(vkk_uiWidget_t* widget,
                     int dragx, int dragy)
{
	ASSERT(widget);

	vkk_uiWidgetLayout_t* layout = &widget->layout;

	vkk_uiListBox_t* self = (vkk_uiListBox_t*) widget;
	if(cc_list_size(self->list) == 0)
	{
		return;
	}

	if(self->orientation == VKK_UI_LISTBOX_ORIENTATION_VERTICAL)
	{
		if(layout->wrapy == VKK_UI_WIDGET_WRAP_SHRINK)
		{
			vkk_uiListBox_layoutVerticalShrink(self, dragx, dragy);
		}
		else
		{
			vkk_uiListBox_layoutVerticalStretch(self, dragx, dragy);
		}
	}
	else
	{
		if(layout->wrapx == VKK_UI_WIDGET_WRAP_SHRINK)
		{
			vkk_uiListBox_layoutHorizontalShrink(self, dragx, dragy);
		}
		else
		{
			vkk_uiListBox_layoutHorizontalStretch(self, dragx, dragy);
		}
	}
}

static void
vkk_uiListBox_drag(vkk_uiWidget_t* widget,
                   float x, float y, float dx, float dy)
{
	ASSERT(widget);

	vkk_uiListBox_t* self = (vkk_uiListBox_t*) widget;
	cc_listIter_t*   iter = cc_list_head(self->list);
	while(iter)
	{
		vkk_uiWidget_t* tmp;
		tmp = (vkk_uiWidget_t*) cc_list_peekIter(iter);
		vkk_uiWidget_drag(tmp, x, y, dx, dy);
		iter = cc_list_next(iter);
	}
}

static void vkk_uiListBox_draw(vkk_uiWidget_t* widget)
{
	ASSERT(widget);

	vkk_uiListBox_t* self = (vkk_uiListBox_t*) widget;
	cc_listIter_t*  iter = cc_list_head(self->list);
	while(iter)
	{
		vkk_uiWidget_t* tmp;
		tmp = (vkk_uiWidget_t*) cc_list_peekIter(iter);
		vkk_uiWidget_draw(tmp);
		iter = cc_list_next(iter);
	}
}

static int
vkk_uiListBox_refresh(vkk_uiWidget_t* widget)
{
	ASSERT(widget);

	vkk_uiListBox_t* self = (vkk_uiListBox_t*) widget;

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

vkk_uiListBox_t*
vkk_uiListBox_new(vkk_uiScreen_t* screen, size_t wsize,
                  vkk_uiListBoxFn_t* lbfn,
                  vkk_uiWidgetLayout_t* layout,
                  vkk_uiWidgetScroll_t* scroll,
                  int orientation,
                  cc_vec4f_t* color)
{
	ASSERT(screen);
	ASSERT(lbfn);
	ASSERT(layout);
	ASSERT(scroll);
	ASSERT(color);

	if(wsize == 0)
	{
		wsize = sizeof(vkk_uiListBox_t);
	}

	vkk_uiWidgetFn_t fn =
	{
		.priv       = lbfn->priv,
		.action_fn  = vkk_uiListBox_action,
		.drag_fn    = vkk_uiListBox_drag,
		.draw_fn    = vkk_uiListBox_draw,
		.layout_fn  = vkk_uiListBox_layout,
		.refresh_fn = vkk_uiListBox_refresh,
		.size_fn    = vkk_uiListBox_size,
	};

	vkk_uiListBox_t* self;
	self = (vkk_uiListBox_t*)
	       vkk_uiWidget_new(screen, wsize, color, layout,
	                        scroll, &fn);
	if(self == NULL)
	{
		return NULL;
	}

	self->list = cc_list_new();
	if(self->list == NULL)
	{
		goto fail_list;
	}

	self->orientation = orientation;
	self->refresh_fn  = lbfn->refresh_fn;

	// success
	return self;

	// failure
	fail_list:
		vkk_uiWidget_delete((vkk_uiWidget_t**) &self);
	return NULL;
}

void vkk_uiListBox_delete(vkk_uiListBox_t** _self)
{
	ASSERT(_self);

	vkk_uiListBox_t* self = *_self;
	if(self)
	{
		cc_list_delete(&self->list);
		vkk_uiWidget_delete((vkk_uiWidget_t**) _self);
	}
}

void vkk_uiListBox_clear(vkk_uiListBox_t* self)
{
	ASSERT(self);

	vkk_uiWidget_t* widget = &self->base;

	cc_list_discard(self->list);
	vkk_uiWidget_scrollTop(widget);
	vkk_uiScreen_layoutDirty(widget->screen);
}

int vkk_uiListBox_add(vkk_uiListBox_t* self,
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

int vkk_uiListBox_addSorted(vkk_uiListBox_t* self,
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
	vkk_uiScreen_layoutDirty(widget->screen);

	return 1;
}

cc_listIter_t* vkk_uiListBox_head(vkk_uiListBox_t* self)
{
	ASSERT(self);

	return cc_list_head(self->list);
}

vkk_uiWidget_t* vkk_uiListBox_remove(vkk_uiListBox_t* self,
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
