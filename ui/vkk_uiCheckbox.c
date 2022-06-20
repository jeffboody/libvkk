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

static int
vkk_uiCheckbox_click(vkk_uiWidget_t* widget,
                     int state, float x, float y)
{
	ASSERT(widget);

	vkk_uiCheckbox_t* self = (vkk_uiCheckbox_t*) widget;
	if(state == VKK_UI_WIDGET_POINTER_UP)
	{
		*(self->pvalue) = 1 - *(self->pvalue);
	}
	return 1;
}

static void
vkk_uiCheckbox_refresh(vkk_uiWidget_t* widget)
{
	ASSERT(widget);

	vkk_uiCheckbox_t*  self = (vkk_uiCheckbox_t*) widget;
	vkk_uiBulletbox_t* base = &self->base;
	vkk_uiBulletbox_select(base, *(self->pvalue));
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_uiCheckbox_t*
vkk_uiCheckbox_new(vkk_uiScreen_t* screen, size_t wsize,
                   vkk_uiBulletboxStyle_t* bulletbox_style,
                   int* pvalue)
{
	ASSERT(screen);
	ASSERT(bulletbox_style);
	ASSERT(pvalue);

	if(wsize == 0)
	{
		wsize = sizeof(vkk_uiCheckbox_t);
	}

	vkk_uiWidgetFn_t widget_fn =
	{
		.click_fn   = vkk_uiCheckbox_click,
		.refresh_fn = vkk_uiCheckbox_refresh,
	};

	const char* sprite_array[] =
	{
		"vkk/ui/icons/ic_check_box_outline_blank_white_24dp.png",
		"vkk/ui/icons/ic_check_box_white_24dp.png",
		NULL
	};

	vkk_uiCheckbox_t* self;
	self = (vkk_uiCheckbox_t*)
	       vkk_uiBulletbox_new(screen, wsize,
	                          VKK_UI_WIDGET_ANCHOR_TL,
	                          &widget_fn, bulletbox_style,
	                          sprite_array);
	if(self == NULL)
	{
		return NULL;
	}

	self->pvalue = pvalue;

	return self;
}

vkk_uiCheckbox_t*
vkk_uiCheckbox_newPageItem(vkk_uiScreen_t* screen,
                           int* pvalue)
{
	ASSERT(screen);
	ASSERT(pvalue);

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

	return vkk_uiCheckbox_new(screen, 0, &style, pvalue);
}

void vkk_uiCheckbox_delete(vkk_uiCheckbox_t** _self)
{
	ASSERT(_self);

	vkk_uiCheckbox_t* self = *_self;
	if(self)
	{
		vkk_uiBulletbox_delete((vkk_uiBulletbox_t**) _self);
		*_self = NULL;
	}
}

void vkk_uiCheckbox_label(vkk_uiCheckbox_t* self,
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

	vkk_uiBulletbox_t* base = &self->base;
	vkk_uiBulletbox_label(base, "%s", string);
}
