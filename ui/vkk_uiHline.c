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

#define LOG_TAG "vkk"
#include "../../libcc/cc_log.h"
#include "../vkk_ui.h"

/***********************************************************
* private                                                  *
***********************************************************/

static void
vkk_uiHline_size(vkk_uiWidget_t* widget, float* w, float* h)
{
	ASSERT(widget);
	ASSERT(w);
	ASSERT(h);

	vkk_uiHline_t * self = (vkk_uiHline_t*) widget;

	float hline_w = *w;
	float hline_h = *h;
	vkk_uiWidget_layoutSize(self->line, &hline_w, &hline_h);
}

static void
vkk_uiHline_layout(vkk_uiWidget_t* widget,
                   int dragx, int dragy)
{
	ASSERT(widget);

	vkk_uiHline_t*  self = (vkk_uiHline_t*) widget;
	vkk_uiWidget_t* line = self->line;

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
	vkk_uiWidget_layoutAnchor(line, &rect_draw, &x, &y);
	cc_rect1f_intersect(&rect_draw,
	                    &widget->rect_clip,
	                    &rect_clip);
	vkk_uiWidget_layoutXYClip(line, x, y, &rect_clip,
	                          dragx, dragy);
}

static void
vkk_uiHline_drag(vkk_uiWidget_t* widget,
                 float x, float y, float dx, float dy)
{
	ASSERT(widget);

	vkk_uiHline_t* self = (vkk_uiHline_t*) widget;
	vkk_uiWidget_drag(self->line, x, y, dx, dy);
}

static void vkk_uiHline_draw(struct vkk_uiWidget_s* widget)
{
	ASSERT(widget);

	vkk_uiHline_t* self = (vkk_uiHline_t*) widget;
	vkk_uiWidget_draw(self->line);
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_uiHline_t*
vkk_uiHline_new(vkk_uiScreen_t* screen, size_t wsize,
                int size, cc_vec4f_t* color)
{
	ASSERT(screen);
	ASSERT(color);

	if(wsize == 0)
	{
		wsize = sizeof(vkk_uiHline_t);
	}

	int wrapy = VKK_UI_WIDGET_WRAP_STRETCH_TEXT_VSMALL +
	            size - VKK_UI_HLINE_SIZE_SMALL;
	vkk_uiWidgetLayout_t layout =
	{
		.anchor   = VKK_UI_WIDGET_ANCHOR_CL,
		.wrapx    = VKK_UI_WIDGET_WRAP_STRETCH_PARENT,
		.wrapy    = wrapy,
		.stretchx = 1.0f,
		.stretchy = 1.0f
	};

	vkk_uiWidgetScroll_t scroll =
	{
		.scroll_bar = 0
	};

	vkk_uiWidgetFn_t fn;
	memset(&fn, 0, sizeof(vkk_uiWidgetFn_t));

	vkk_uiWidgetPrivFn_t priv_fn =
	{
		.size_fn   = vkk_uiHline_size,
		.layout_fn = vkk_uiHline_layout,
		.drag_fn   = vkk_uiHline_drag,
		.draw_fn   = vkk_uiHline_draw,
	};

	cc_vec4f_t clear =
	{
		.a = 0.0f
	};

	vkk_uiHline_t* self;
	self = (vkk_uiHline_t*)
	       vkk_uiWidget_new(screen, wsize, &clear, &layout,
	                        &scroll, &fn, &priv_fn);
	if(self == NULL)
	{
		return NULL;
	}

	self->size = size;

	// override the line properties
	memset(&priv_fn, 0, sizeof(vkk_uiWidgetPrivFn_t));
	layout.stretchy = 0.10f;

	self->line = vkk_uiWidget_new(screen, 0, color, &layout,
	                              &scroll, &fn, &priv_fn);
	if(self->line == NULL)
	{
		goto fail_line;
	}

	// success
	return self;

	// failure
	fail_line:
		vkk_uiWidget_delete((vkk_uiWidget_t**) &self);
	return NULL;
}

vkk_uiHline_t*
vkk_uiHline_newPageItem(vkk_uiScreen_t* screen)
{
	ASSERT(screen);

	cc_vec4f_t color = { .a = 0.0f };

	return vkk_uiHline_new(screen, 0,
	                       VKK_UI_HLINE_SIZE_SMALL, &color);
}

vkk_uiHline_t*
vkk_uiHline_newInfoItem(vkk_uiScreen_t* screen)
{
	ASSERT(screen);

	cc_vec4f_t color;
	vkk_uiScreen_colorPageItem(screen, &color);

	return vkk_uiHline_new(screen, 0,
	                       VKK_UI_HLINE_SIZE_SMALL, &color);
}

void vkk_uiHline_delete(vkk_uiHline_t** _self)
{
	ASSERT(_self);

	vkk_uiHline_t* self = *_self;
	if(self)
	{
		vkk_uiWidget_delete(&self->line);
		vkk_uiWidget_delete((vkk_uiWidget_t**) _self);
	}
}
