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
* public                                                   *
***********************************************************/

vkk_uiRadioList_t*
vkk_uiRadioList_new(vkk_uiScreen_t* screen, size_t wsize,
                    vkk_uiRadioListFn_t* rlfn,
                    vkk_uiWidgetLayout_t* layout,
                    vkk_uiWidgetScroll_t* scroll,
                    vkk_uiBulletBoxStyle_t* bulletbox_style)
{
	ASSERT(screen);
	ASSERT(rlfn);
	ASSERT(layout);
	ASSERT(scroll);
	ASSERT(bulletbox_style);

	if(wsize == 0)
	{
		wsize = sizeof(vkk_uiRadioList_t);
	}

	vkk_uiListBoxFn_t lbfn =
	{
		.priv = rlfn->priv,
	};

	cc_vec4f_t clear =
	{
		.a = 0.0f
	};

	vkk_uiRadioList_t* self;
	self = (vkk_uiRadioList_t*)
	       vkk_uiListBox_new(screen, wsize, &lbfn,
	                         layout, scroll,
	                         VKK_UI_LISTBOX_ORIENTATION_VERTICAL,
	                         &clear);
	if(self == NULL)
	{
		return NULL;
	}

	memcpy(&self->bulletbox_style, bulletbox_style,
	       sizeof(vkk_uiBulletBoxStyle_t));

	self->value_fn = rlfn->value_fn;

	return self;
}

vkk_uiRadioList_t*
vkk_uiRadioList_newPageItem(vkk_uiScreen_t* screen,
                            vkk_uiRadioListFn_t* rlfn)
{
	ASSERT(screen);
	ASSERT(rlfn);

	vkk_uiWidgetLayout_t list_layout =
	{
		.wrapx    = VKK_UI_WIDGET_WRAP_STRETCH_PARENT,
		.stretchx = 1.0f,
	};

	vkk_uiWidgetScroll_t scroll =
	{
		.scroll_bar = 0
	};

	vkk_uiBulletBoxStyle_t style =
	{
		.text_style =
		{
			.font_type = VKK_UI_TEXT_FONTTYPE_REGULAR,
			.size      = VKK_UI_TEXT_SIZE_MEDIUM,
			.spacing   = VKK_UI_TEXT_SPACING_LARGE,
		}
	};
	vkk_uiScreen_colorPageItem(screen, &style.color_icon);
	vkk_uiScreen_colorPageItem(screen, &style.text_style.color);

	return vkk_uiRadioList_new(screen, 0, rlfn,
	                           &list_layout, &scroll, &style);
}

void vkk_uiRadioList_delete(vkk_uiRadioList_t** _self)
{
	ASSERT(_self);

	vkk_uiRadioList_t* self = *_self;
	if(self)
	{
		vkk_uiRadioList_clear(self);
		vkk_uiListBox_delete((vkk_uiListBox_t**) _self);
		*_self = NULL;
	}
}

void vkk_uiRadioList_clear(vkk_uiRadioList_t* self)
{
	ASSERT(self);

	cc_listIter_t* iter;
	iter = vkk_uiListBox_head(&self->base);
	while(iter)
	{
		vkk_uiRadioBox_t* rb;
		rb = (vkk_uiRadioBox_t*)
		     vkk_uiListBox_remove(&self->base, &iter);
		vkk_uiRadioBox_delete(&rb);
	}
}

void vkk_uiRadioList_add(vkk_uiRadioList_t* self, int value,
                         const char* fmt, ...)
{
	ASSERT(self);
	ASSERT(fmt);

	vkk_uiWidget_t* widget = (vkk_uiWidget_t*) self;

	// decode string
	char string[256];
	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(string, 256, fmt, argptr);
	va_end(argptr);

	vkk_uiRadioBoxFn_t rbfn =
	{
		.priv     = widget->fn.priv,
		.value_fn = self->value_fn,
	};

	vkk_uiRadioBox_t* rb;
	rb = vkk_uiRadioBox_new(widget->screen, 0, &rbfn,
	                        &self->bulletbox_style, self,
	                        value);
	if(rb == NULL)
	{
		return;
	}

	if(vkk_uiListBox_add(&self->base,
	                     (vkk_uiWidget_t*) rb) == 0)
	{
		goto fail_add;
	}

	vkk_uiRadioBox_label(rb, "%s", string);

	// success
	return;

	// failure
	fail_add:
		vkk_uiRadioBox_delete(&rb);
}

int vkk_uiRadioList_get(vkk_uiRadioList_t* self)
{
	ASSERT(self);

	return self->value;
}

void vkk_uiRadioList_set(vkk_uiRadioList_t* self,
                         int value)
{
	ASSERT(self);

	self->value = value;
}
