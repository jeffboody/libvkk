/*
 * Copyright (c) 2018 Jeff Boody
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
#include "vkui_hline.h"
#include "vkui_screen.h"
#include "vkui_widget.h"

/***********************************************************
* private                                                  *
***********************************************************/

static void
vkui_hline_size(vkui_widget_t* widget, float* w, float* h)
{
	ASSERT(widget);
	ASSERT(w);
	ASSERT(h);

	vkui_hline_t * self = (vkui_hline_t*) widget;

	// subtract border so that the hline doesn't intersect
	// with a scroll bar
	// note: this uses a border width of HLINE_SIZE - 1 to
	// minimize the asymetry on the hline which means that
	// a small HLINE will not have any spacing
	float h_bo = 0.0f;
	float v_bo = 0.0f;
	vkui_screen_layoutBorder(widget->screen,
	                         VKUI_WIDGET_BORDER_NONE + self->size,
	                         &h_bo, &v_bo);

	float hline_w = *w - h_bo;
	float hline_h = *h;
	vkui_widget_layoutSize(self->line, &hline_w, &hline_h);
}

static void
vkui_hline_layout(vkui_widget_t* widget,
                  int dragx, int dragy)
{
	ASSERT(widget);

	vkui_hline_t*  self = (vkui_hline_t*) widget;
	vkui_widget_t* line = self->line;

	// initialize the layout
	float x = 0.0f;
	float y = 0.0f;
	float t = widget->rect_draw.t;
	float l = widget->rect_draw.l;
	float h = widget->rect_draw.h;
	float w = widget->rect_draw.w;

	// layout line
	cc_rect1f_t rect_draw =
	{
		.t = t,
		.l = l,
		.w = w,
		.h = h
	};
	cc_rect1f_t rect_clip;
	vkui_widget_layoutAnchor(line, &rect_draw, &x, &y);
	cc_rect1f_intersect(&rect_draw,
	                    &widget->rect_clip,
	                    &rect_clip);
	vkui_widget_layoutXYClip(line, x, y, &rect_clip,
	                         dragx, dragy);
}

static void
vkui_hline_drag(vkui_widget_t* widget,
                float x, float y, float dx, float dy)
{
	ASSERT(widget);

	vkui_hline_t* self = (vkui_hline_t*) widget;
	vkui_widget_drag(self->line, x, y, dx, dy);
}

static void vkui_hline_draw(struct vkui_widget_s* widget)
{
	ASSERT(widget);

	vkui_hline_t* self = (vkui_hline_t*) widget;
	vkui_widget_draw(self->line);
}

/***********************************************************
* public                                                   *
***********************************************************/

vkui_hline_t*
vkui_hline_new(vkui_screen_t* screen, size_t wsize, int size,
               cc_vec4f_t* color)
{
	ASSERT(screen);
	ASSERT(color);

	if(wsize == 0)
	{
		wsize = sizeof(vkui_hline_t);
	}

	int wrapy = VKUI_WIDGET_WRAP_STRETCH_TEXT_VSMALL +
	            size - VKUI_HLINE_SIZE_SMALL;
	vkui_widgetLayout_t layout =
	{
		.anchor   = VKUI_WIDGET_ANCHOR_CL,
		.wrapx    = VKUI_WIDGET_WRAP_STRETCH_PARENT,
		.wrapy    = wrapy,
		.stretchx = 1.0f,
		.stretchy = 1.0f
	};

	vkui_widgetScroll_t scroll =
	{
		.scroll_bar = 0
	};

	vkui_widgetFn_t fn;
	memset(&fn, 0, sizeof(vkui_widgetFn_t));

	vkui_widgetPrivFn_t priv_fn =
	{
		.size_fn   = vkui_hline_size,
		.layout_fn = vkui_hline_layout,
		.drag_fn   = vkui_hline_drag,
		.draw_fn   = vkui_hline_draw,
	};

	cc_vec4f_t clear =
	{
		.a = 0.0f
	};

	vkui_hline_t* self;
	self = (vkui_hline_t*)
	       vkui_widget_new(screen, wsize, &clear, &layout,
	                       &scroll, &fn, &priv_fn);
	if(self == NULL)
	{
		return NULL;
	}

	self->size = size;

	// override the line properties
	memset(&priv_fn, 0, sizeof(vkui_widgetPrivFn_t));
	layout.stretchy = 0.10f;

	self->line = vkui_widget_new(screen, 0, color, &layout,
	                             &scroll, &fn, &priv_fn);
	if(self->line == NULL)
	{
		goto fail_line;
	}

	// success
	return self;

	// failure
	fail_line:
		vkui_widget_delete((vkui_widget_t**) &self);
	return NULL;
}

void vkui_hline_delete(vkui_hline_t** _self)
{
	ASSERT(_self);

	vkui_hline_t* self = *_self;
	if(self)
	{
		vkui_widget_delete(&self->line);
		vkui_widget_delete((vkui_widget_t**) _self);
	}
}
