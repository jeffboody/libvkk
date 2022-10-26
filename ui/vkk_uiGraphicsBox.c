/*
 * Copyright (c) 2022 Jeff Boody
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
#include "../../libcc/cc_memory.h"
#include "../../libcc/cc_log.h"
#include "../vkk_ui.h"

/***********************************************************
* protected                                                *
***********************************************************/

static void
vkk_uiGraphicsBox_draw(vkk_uiWidget_t* widget)
{
	ASSERT(widget);

	vkk_uiGraphicsBox_t* self = (vkk_uiGraphicsBox_t*) widget;

	vkk_renderer_t* rend = widget->screen->renderer;

	cc_rect1f_t* rect_draw;
	cc_rect1f_t* rect_scissor;
	rect_draw    = vkk_uiWidget_rectDraw(widget);
	rect_scissor = vkk_uiWidget_rectScissor(widget);

	vkk_uiScreen_bind(widget->screen, VKK_UI_SCREEN_BIND_NONE);

	vkk_renderer_viewport(rend,
	                      rect_draw->l, rect_draw->t,
	                      rect_draw->w, rect_draw->h);

	if(self->clear_depth &&
	   vkk_uiScreen_depthDirty(widget->screen, widget))
	{
		vkk_renderer_clearDepth(rend,
		                        (int32_t)  rect_scissor->t,
		                        (int32_t)  rect_scissor->l,
		                        (uint32_t) rect_scissor->w,
		                        (uint32_t) rect_scissor->h);
	}

	vkk_uiWidgetDraw_fn draw_fn = self->draw_fn;
	(*draw_fn)(widget);

	if(self->clear_depth)
	{
		vkk_uiScreen_depthMark(widget->screen, widget);
	}

	uint32_t width;
	uint32_t height;
	vkk_renderer_surfaceSize(rend, &width, &height);
	vkk_renderer_viewport(rend, 0, 0,
	                      width, height);
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_uiGraphicsBox_t*
vkk_uiGraphicsBox_new(vkk_uiScreen_t* screen,
                      size_t wsize,
                      vkk_uiGraphicsBoxFn_t* gbfn,
                      vkk_uiWidgetLayout_t* layout,
                      int clear_depth,
                      cc_vec4f_t* color)
{
	ASSERT(screen);
	ASSERT(gbfn);
	ASSERT(gbfn->draw_fn);
	ASSERT(layout);
	ASSERT(color);

	vkk_uiWidgetScroll_t scroll =
	{
		.scroll_bar = 0,
	};

	vkk_uiWidgetFn_t fn =
	{
		.priv       = gbfn->priv,
		.action_fn  = gbfn->action_fn,
		.click_fn   = gbfn->click_fn,
		.draw_fn    = vkk_uiGraphicsBox_draw,
		.refresh_fn = gbfn->refresh_fn,
	};

	vkk_uiGraphicsBox_t* self;
	self = (vkk_uiGraphicsBox_t*)
	       vkk_uiWidget_new(screen,
	                        sizeof(vkk_uiGraphicsBox_t),
	                        color, layout, &scroll, &fn);
	if(self == NULL)
	{
		return NULL;
	}

	self->draw_fn     = gbfn->draw_fn;
	self->clear_depth = clear_depth;

	return self;
}

void vkk_uiGraphicsBox_delete(vkk_uiGraphicsBox_t** _self)
{
	ASSERT(_self);

	vkk_uiWidget_delete((vkk_uiWidget_t**) _self);
}
