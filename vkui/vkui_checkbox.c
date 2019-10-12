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

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define LOG_TAG "vkui"
#include "../../libcc/cc_log.h"
#include "vkui_bulletbox.h"
#include "vkui_checkbox.h"
#include "vkui_screen.h"
#include "vkui_widget.h"

/***********************************************************
* private                                                  *
***********************************************************/

static int
vkui_checkbox_click(vkui_widget_t* widget, void* priv,
                    int state, float x, float y)
{
	// priv may be NULL
	assert(widget);

	vkui_checkbox_t* self = (vkui_checkbox_t*) widget;
	if(state == VKUI_WIDGET_POINTER_UP)
	{
		*(self->pvalue) = 1 - *(self->pvalue);
	}
	return 1;
}

static void
vkui_checkbox_refresh(vkui_widget_t* widget, void* priv)
{
	// priv may be NULL
	assert(widget);

	vkui_checkbox_t*  self   = (vkui_checkbox_t*) widget;
	vkui_bulletbox_t* bullet = &(self->bullet);
	vkui_bulletbox_select(bullet, *(self->pvalue));
}

/***********************************************************
* public                                                   *
***********************************************************/

vkui_checkbox_t*
vkui_checkbox_new(vkui_screen_t* screen, size_t wsize,
                  vkui_bulletboxStyle_t* bulletbox_style,
                  int* pvalue)
{
	assert(screen);
	assert(bulletbox_style);
	assert(pvalue);

	if(wsize == 0)
	{
		wsize = sizeof(vkui_checkbox_t);
	}

	vkui_widgetFn_t widget_fn =
	{
		.click_fn   = vkui_checkbox_click,
		.refresh_fn = vkui_checkbox_refresh,
	};

	const char* sprite_array[] =
	{
		"vkui/ic_check_box_outline_blank_white_24dp.texz",
		"vkui/ic_check_box_white_24dp.texz",
		NULL
	};

	vkui_checkbox_t* self;
	self = (vkui_checkbox_t*)
	       vkui_bulletbox_new(screen, wsize,
	                          VKUI_WIDGET_ANCHOR_TL,
	                          &widget_fn, bulletbox_style,
	                          sprite_array);
	if(self == NULL)
	{
		return NULL;
	}

	self->pvalue = pvalue;

	return self;
}

void vkui_checkbox_delete(vkui_checkbox_t** _self)
{
	assert(_self);

	vkui_checkbox_t* self = *_self;
	if(self)
	{
		vkui_bulletbox_delete((vkui_bulletbox_t**) _self);
		*_self = NULL;
	}
}

void vkui_checkbox_label(vkui_checkbox_t* self,
                         const char* fmt, ...)
{
	assert(self);
	assert(fmt);

	// decode string
	char    string[256];
	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(string, 256, fmt, argptr);
	va_end(argptr);

	vkui_bulletbox_label(&self->bullet, "%s", string);
}
