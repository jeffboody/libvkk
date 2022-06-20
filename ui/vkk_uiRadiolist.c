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

#define LOG_TAG "vkk"
#include "../../libcc/cc_log.h"
#include "../vkk_ui.h"

/***********************************************************
* private                                                  *
***********************************************************/

static void
vkk_uiRadiolist_refresh(vkk_uiWidget_t* widget)
{
	ASSERT(widget);

	vkk_uiRadiolist_t* self = (vkk_uiRadiolist_t*) widget;
	vkk_uiListbox_t*   base = &self->base;

	if(self->value != *(self->pvalue))
	{
		self->value = *(self->pvalue);

		cc_listIter_t* iter = cc_list_head(base->list);
		while(iter)
		{
			vkk_uiRadiobox_t* rb;
			rb = (vkk_uiRadiobox_t*) cc_list_peekIter(iter);
			vkk_uiRadiobox_refresh(rb);
			iter = cc_list_next(iter);
		}
	}
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_uiRadiolist_t*
vkk_uiRadiolist_new(vkk_uiScreen_t* screen, size_t wsize,
                    vkk_uiWidgetLayout_t* layout,
                    vkk_uiWidgetScroll_t* scroll,
                    vkk_uiBulletboxStyle_t* bulletbox_style,
                    int* pvalue)
{
	ASSERT(screen);
	ASSERT(layout);
	ASSERT(scroll);
	ASSERT(bulletbox_style);
	ASSERT(pvalue);

	if(wsize == 0)
	{
		wsize = sizeof(vkk_uiRadiolist_t);
	}

	vkk_uiWidgetFn_t fn =
	{
		.refresh_fn = vkk_uiRadiolist_refresh,
	};

	cc_vec4f_t clear =
	{
		.a = 0.0f
	};

	vkk_uiRadiolist_t* self;
	self = (vkk_uiRadiolist_t*)
	       vkk_uiListbox_new(screen, wsize, layout, scroll, &fn,
	                         VKK_UI_LISTBOX_ORIENTATION_VERTICAL,
	                         &clear);
	if(self == NULL)
	{
		return NULL;
	}

	self->pvalue = pvalue;
	self->value  = *pvalue;

	memcpy(&self->bulletbox_style, bulletbox_style,
	       sizeof(vkk_uiBulletboxStyle_t));

	return self;
}

vkk_uiRadiolist_t*
vkk_uiRadiolist_newPageItem(vkk_uiScreen_t* screen,
                            int* pvalue)
{
	ASSERT(screen);
	ASSERT(pvalue);

	vkk_uiWidgetLayout_t list_layout =
	{
		.wrapx    = VKK_UI_WIDGET_WRAP_STRETCH_PARENT,
		.stretchx = 1.0f,
	};

	vkk_uiWidgetScroll_t scroll =
	{
		.scroll_bar = 0
	};

	vkk_uiBulletboxStyle_t style =
	{
		.text_style =
		{
			.font_type = VKK_UI_TEXT_FONTTYPE_REGULAR,
			.size      = VKK_UI_TEXT_SIZE_MEDIUM,
			.spacing   = VKK_UI_TEXT_SPACING_MEDIUM
		}
	};
	vkk_uiScreen_colorPageItem(screen, &style.color_icon);
	vkk_uiScreen_colorPageItem(screen, &style.text_style.color);

	return vkk_uiRadiolist_new(screen, 0, &list_layout,
	                           &scroll, &style, pvalue);
}

void vkk_uiRadiolist_delete(vkk_uiRadiolist_t** _self)
{
	ASSERT(_self);

	vkk_uiRadiolist_t* self = *_self;
	if(self)
	{
		vkk_uiRadiolist_clear(self);
		vkk_uiListbox_delete((vkk_uiListbox_t**) _self);
		*_self = NULL;
	}
}

void vkk_uiRadiolist_clear(vkk_uiRadiolist_t* self)
{
	ASSERT(self);

	vkk_uiListbox_t* base = &self->base;

	cc_listIter_t* iter;
	iter = vkk_uiListbox_head(base);
	while(iter)
	{
		vkk_uiRadiobox_t* rb;
		rb = (vkk_uiRadiobox_t*)
		     vkk_uiListbox_remove(base, &iter);
		vkk_uiRadiobox_delete(&rb);
	}
}

void vkk_uiRadiolist_add(vkk_uiRadiolist_t* self, int value,
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

	vkk_uiWidget_t* widget = (vkk_uiWidget_t*) self;
	vkk_uiRadiobox_t* rb;
	rb = vkk_uiRadiobox_new(widget->screen, 0,
	                        &self->bulletbox_style, value,
	                        self);
	if(rb == NULL)
	{
		return;
	}

	vkk_uiListbox_t* base = &self->base;
	if(vkk_uiListbox_add(base, (vkk_uiWidget_t*) rb) == 0)
	{
		goto fail_add;
	}

	vkk_uiRadiobox_label(rb, "%s", string);

	// success
	return;

	// failure
	fail_add:
		vkk_uiRadiobox_delete(&rb);
}

void vkk_uiRadiolist_value(vkk_uiRadiolist_t* self,
                           int value)
{
	ASSERT(self);

	*(self->pvalue) = value;
}
