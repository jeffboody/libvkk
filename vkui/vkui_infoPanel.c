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

#define LOG_TAG "vkui"
#include "../../libcc/cc_log.h"
#include "vkui.h"

/***********************************************************
* public                                                   *
***********************************************************/

vkui_infoPanel_t*
vkui_infoPanel_new(vkui_screen_t* screen, size_t wsize,
                   vkui_widgetFn_t* widget_fn)
{
	ASSERT(screen);
	ASSERT(widget_fn);

	vkui_widgetLayout_t layout =
	{
		.border = VKUI_WIDGET_BORDER_MEDIUM,
	};

	vkui_widgetScroll_t scroll =
	{
		.scroll_bar = 0
	};

	cc_vec4f_t color;
	vkui_screen_colorBackground(screen, &color);

	return (vkui_infoPanel_t*)
	       vkui_listbox_new(screen, wsize, &layout,
	                        &scroll, widget_fn,
	                        VKUI_LISTBOX_ORIENTATION_VERTICAL,
	                        &color);
}

void vkui_infoPanel_delete(vkui_infoPanel_t** _self)
{
	ASSERT(_self);

	vkui_infoPanel_t* self = *_self;
	if(self)
	{
		vkui_listbox_delete((vkui_listbox_t**) _self);
	}
}

void vkui_infoPanel_add(vkui_infoPanel_t* self,
                        vkui_widget_t* widget)
{
	ASSERT(self);
	ASSERT(widget);

	vkui_listbox_t* base = &self->base;
	vkui_listbox_add(base, widget);
}

void vkui_infoPanel_clear(vkui_infoPanel_t* self)
{
	ASSERT(self);

	vkui_listbox_t* base = &self->base;
	vkui_listbox_clear(base);
}
