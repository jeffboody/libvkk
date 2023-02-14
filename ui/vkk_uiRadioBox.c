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

#define LOG_TAG "vkk"
#include "../../libcc/cc_log.h"
#include "../vkk_ui.h"

/***********************************************************
* private                                                  *
***********************************************************/

static void
vkk_uiRadioBox_click(vkk_uiWidget_t* widget,
                     float x, float y)
{
	ASSERT(widget);

	vkk_uiRadioBox_t* self = (vkk_uiRadioBox_t*) widget;

	vkk_uiRadioList_set(self->parent, self->value);

	vkk_uiWidgetValue_fn value_fn = widget->fn.value_fn;
	if(value_fn)
	{
		(*value_fn)(widget, self->value);
	}
}

static int
vkk_uiRadioBox_refresh(vkk_uiWidget_t* widget)
{
	ASSERT(widget);

	vkk_uiRadioBox_t* self = (vkk_uiRadioBox_t*) widget;

	if(self->value == vkk_uiRadioList_get(self->parent))
	{
		vkk_uiBulletBox_select(&self->base, 1);
	}
	else
	{
		vkk_uiBulletBox_select(&self->base, 0);
	}

	return 1;
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_uiRadioBox_t*
vkk_uiRadioBox_new(vkk_uiScreen_t* screen, size_t wsize,
                   vkk_uiRadioBoxFn_t* rbfn,
                   vkk_uiBulletBoxStyle_t* bulletbox_style,
                   vkk_uiRadioList_t* parent, int value)
{
	ASSERT(screen);
	ASSERT(bulletbox_style);
	ASSERT(parent);

	if(wsize == 0)
	{
		wsize = sizeof(vkk_uiRadioBox_t);
	}

	vkk_uiBulletBoxFn_t bulletbox_fn =
	{
		.priv     = rbfn->priv,
		.click_fn = vkk_uiRadioBox_click,
	};

	const char* sprite_array[] =
	{
		"vkk/ui/icons/ic_radio_button_unchecked_white_24dp.png",
		"vkk/ui/icons/ic_radio_button_checked_white_24dp.png",
		NULL
	};

	vkk_uiRadioBox_t* self;
	self = (vkk_uiRadioBox_t*)
	       vkk_uiBulletBox_new(screen, wsize,
	                           &bulletbox_fn,
	                           VKK_UI_WIDGET_ANCHOR_TL,
	                           bulletbox_style,
	                           sprite_array);
	if(self == NULL)
	{
		return NULL;
	}

	// override functions
	vkk_uiWidget_t*  widget = (vkk_uiWidget_t*) self;
	vkk_uiWidgetFn_t fn_new =
	{
		.refresh_fn = vkk_uiRadioBox_refresh,
		.value_fn   = rbfn->value_fn,
	};
	vkk_uiWidgetFn_t fn_old =
	{
		.priv = NULL
	};
	vkk_uiWidget_override(widget, &fn_new, &fn_old);

	self->parent = parent;
	self->value  = value;

	return self;
}

void vkk_uiRadioBox_delete(vkk_uiRadioBox_t** _self)
{
	ASSERT(_self);

	vkk_uiRadioBox_t* self = *_self;
	if(self)
	{
		vkk_uiBulletBox_delete((vkk_uiBulletBox_t**) _self);
	}
}

void vkk_uiRadioBox_label(vkk_uiRadioBox_t* self,
                          const char* fmt, ...)
{
	ASSERT(self);
	ASSERT(fmt);

	// decode string
	char    string[256];
	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(string, 256, fmt, argptr);
	va_end(argptr);

	vkk_uiBulletBox_label(&self->base, "%s", string);
}
