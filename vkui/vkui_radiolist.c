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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "vkui"
#include "../../libcc/cc_log.h"
#include "vkui.h"

/***********************************************************
* private                                                  *
***********************************************************/

static void
vkui_radiolist_refresh(vkui_widget_t* widget)
{
	ASSERT(widget);

	vkui_radiolist_t* self = (vkui_radiolist_t*) widget;
	vkui_listbox_t*   base = &self->base;

	if(self->value != *(self->pvalue))
	{
		self->value = *(self->pvalue);

		cc_listIter_t* iter = cc_list_head(base->list);
		while(iter)
		{
			vkui_radiobox_t* rb;
			rb = (vkui_radiobox_t*) cc_list_peekIter(iter);
			vkui_radiobox_refresh(rb);
			iter = cc_list_next(iter);
		}
	}
}

/***********************************************************
* public                                                   *
***********************************************************/

vkui_radiolist_t*
vkui_radiolist_new(vkui_screen_t* screen, size_t wsize,
                   vkui_widgetLayout_t* layout,
                   vkui_widgetScroll_t* scroll,
                   vkui_bulletboxStyle_t* bulletbox_style,
                   int* pvalue)
{
	ASSERT(screen);
	ASSERT(layout);
	ASSERT(scroll);
	ASSERT(bulletbox_style);
	ASSERT(pvalue);

	if(wsize == 0)
	{
		wsize = sizeof(vkui_radiolist_t);
	}

	vkui_widgetFn_t fn =
	{
		.refresh_fn = vkui_radiolist_refresh,
	};

	cc_vec4f_t clear =
	{
		.a = 0.0f
	};

	vkui_radiolist_t* self;
	self = (vkui_radiolist_t*)
	       vkui_listbox_new(screen, wsize, layout, scroll, &fn,
	                        VKUI_LISTBOX_ORIENTATION_VERTICAL,
	                        &clear);
	if(self == NULL)
	{
		return NULL;
	}

	self->pvalue = pvalue;
	self->value  = *pvalue;

	memcpy(&self->bulletbox_style, bulletbox_style,
	       sizeof(vkui_bulletboxStyle_t));

	return self;
}

vkui_radiolist_t*
vkui_radiolist_newPageItem(vkui_screen_t* screen,
                           int* pvalue)
{
	ASSERT(screen);
	ASSERT(pvalue);

	vkui_widgetLayout_t list_layout =
	{
		.wrapx    = VKUI_WIDGET_WRAP_STRETCH_PARENT,
		.stretchx = 1.0f,
	};

	vkui_widgetScroll_t scroll =
	{
		.scroll_bar = 0
	};

	vkui_bulletboxStyle_t style =
	{
		.text_style =
		{
			.font_type = VKUI_TEXT_FONTTYPE_REGULAR,
			.size      = VKUI_TEXT_SIZE_MEDIUM,
			.spacing   = VKUI_TEXT_SPACING_MEDIUM
		}
	};
	vkui_screen_colorPageItem(screen, &style.color_icon);
	vkui_screen_colorPageItem(screen, &style.text_style.color);

	return vkui_radiolist_new(screen, 0, &list_layout,
	                          &scroll, &style, pvalue);
}

void vkui_radiolist_delete(vkui_radiolist_t** _self)
{
	ASSERT(_self);

	vkui_radiolist_t* self = *_self;
	if(self)
	{
		vkui_radiolist_clear(self);
		vkui_listbox_delete((vkui_listbox_t**) _self);
		*_self = NULL;
	}
}

void vkui_radiolist_clear(vkui_radiolist_t* self)
{
	ASSERT(self);

	vkui_listbox_t* base = &self->base;

	cc_listIter_t* iter;
	iter = vkui_listbox_head(base);
	while(iter)
	{
		vkui_radiobox_t* rb;
		rb = (vkui_radiobox_t*)
		     vkui_listbox_remove(base, &iter);
		vkui_radiobox_delete(&rb);
	}
}

void vkui_radiolist_add(vkui_radiolist_t* self, int value,
                        const char* fmt, ...)
{
	ASSERT(self);
	ASSERT(fmt);

	// decode string
	char string[256];
	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(string, 256, fmt, argptr);
	va_end(argptr);

	vkui_widget_t* widget = (vkui_widget_t*) self;
	vkui_radiobox_t* rb;
	rb = vkui_radiobox_new(widget->screen, 0,
	                       &self->bulletbox_style, value,
	                       self);
	if(rb == NULL)
	{
		return;
	}

	vkui_listbox_t* base = &self->base;
	if(vkui_listbox_add(base, (vkui_widget_t*) rb) == 0)
	{
		goto fail_add;
	}

	vkui_radiobox_label(rb, "%s", string);

	// success
	return;

	// failure
	fail_add:
		vkui_radiobox_delete(&rb);
}

void vkui_radiolist_value(vkui_radiolist_t* self,
                          int value)
{
	ASSERT(self);

	*(self->pvalue) = value;
}
