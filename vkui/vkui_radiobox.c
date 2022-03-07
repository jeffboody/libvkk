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

#define LOG_TAG "vkui"
#include "../../libcc/cc_log.h"
#include "vkui_radiobox.h"
#include "vkui_radiolist.h"
#include "vkui_widget.h"

/***********************************************************
* private                                                  *
***********************************************************/

static int
vkui_radiobox_click(vkui_widget_t* widget, void* priv,
                   int state, float x, float y)
{
	// priv may be NULL
	ASSERT(widget);

	vkui_radiobox_t* self = (vkui_radiobox_t*) widget;
	if(state == VKUI_WIDGET_POINTER_UP)
	{
		vkui_radiolist_value(self->parent, self->value);
	}

	return 1;
}

/***********************************************************
* public                                                   *
***********************************************************/

vkui_radiobox_t*
vkui_radiobox_new(vkui_screen_t* screen, size_t wsize,
                  vkui_bulletboxStyle_t* bulletbox_style,
                  int value, vkui_radiolist_t* parent)
{
	ASSERT(screen);
	ASSERT(bulletbox_style);
	ASSERT(parent);

	if(wsize == 0)
	{
		wsize = sizeof(vkui_radiobox_t);
	}

	vkui_widgetFn_t widget_fn =
	{
		.click_fn = vkui_radiobox_click
	};

	const char* sprite_array[] =
	{
		"vkui/icons/ic_radio_button_unchecked_white_24dp.png",
		"vkui/icons/ic_radio_button_checked_white_24dp.png",
		NULL
	};

	vkui_radiobox_t* self;
	self = (vkui_radiobox_t*)
	       vkui_bulletbox_new(screen, wsize,
	                          VKUI_WIDGET_ANCHOR_TL,
	                          &widget_fn, bulletbox_style,
	                          sprite_array);
	if(self == NULL)
	{
		return NULL;
	}

	self->value  = value;
	self->parent = parent;

	vkui_radiobox_refresh(self);

	return self;
}

void vkui_radiobox_delete(vkui_radiobox_t** _self)
{
	ASSERT(_self);

	vkui_radiobox_t* self = *_self;
	if(self)
	{
		vkui_bulletbox_delete((vkui_bulletbox_t**) _self);
		*_self = NULL;
	}
}

void vkui_radiobox_label(vkui_radiobox_t* self,
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

	vkui_bulletbox_label(&self->bullet, "%s", string);
}

void vkui_radiobox_refresh(vkui_radiobox_t* self)
{
	ASSERT(self);

	vkui_radiolist_t* parent = self->parent;
	vkui_bulletbox_t* bullet = &(self->bullet);
	if(self->value == parent->value)
	{
		vkui_bulletbox_select(bullet, 1);
	}
	else
	{
		vkui_bulletbox_select(bullet, 0);
	}
}
