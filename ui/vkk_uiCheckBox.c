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
vkk_uiCheckBox_click(vkk_uiWidget_t* widget,
                     float x, float y)
{
	ASSERT(widget);

	vkk_uiCheckBox_t* self = (vkk_uiCheckBox_t*) widget;

	self->value = 1 - self->value;

	vkk_uiWidgetValue_fn value_fn = widget->fn.value_fn;
	if(value_fn)
	{
		(*value_fn)(widget, self->value);
	}
}

static int
vkk_uiCheckBox_refresh(vkk_uiWidget_t* widget)
{
	ASSERT(widget);

	vkk_uiCheckBox_t*  self = (vkk_uiCheckBox_t*) widget;
	vkk_uiBulletBox_select(&self->base, self->value);

	return 1;
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_uiCheckBox_t*
vkk_uiCheckBox_new(vkk_uiScreen_t* screen, size_t wsize,
                   vkk_uiCheckBoxFn_t* cbfn,
                   vkk_uiBulletBoxStyle_t* bulletbox_style)
{
	ASSERT(screen);
	ASSERT(cbfn);
	ASSERT(bulletbox_style);

	if(wsize == 0)
	{
		wsize = sizeof(vkk_uiCheckBox_t);
	}

	vkk_uiBulletBoxFn_t bbfn =
	{
		.priv     = cbfn->priv,
		.click_fn = vkk_uiCheckBox_click,
	};

	const char* sprite_array[] =
	{
		"vkk/ui/icons/ic_check_box_outline_blank_white_24dp.png",
		"vkk/ui/icons/ic_check_box_white_24dp.png",
		NULL
	};

	vkk_uiCheckBox_t* self;
	self = (vkk_uiCheckBox_t*)
	       vkk_uiBulletBox_new(screen, wsize, &bbfn,
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
		.refresh_fn = vkk_uiCheckBox_refresh,
		.value_fn   = cbfn->value_fn,
	};
	vkk_uiWidgetFn_t fn_old =
	{
		.priv = NULL
	};
	vkk_uiWidget_override(widget, &fn_new, &fn_old);

	return self;
}

vkk_uiCheckBox_t*
vkk_uiCheckBox_newPageItem(vkk_uiScreen_t* screen,
                           vkk_uiCheckBoxFn_t* cbfn)
{
	ASSERT(screen);
	ASSERT(cbfn);

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

	return vkk_uiCheckBox_new(screen, 0, cbfn, &style);
}

void vkk_uiCheckBox_delete(vkk_uiCheckBox_t** _self)
{
	ASSERT(_self);

	vkk_uiCheckBox_t* self = *_self;
	if(self)
	{
		vkk_uiBulletBox_delete((vkk_uiBulletBox_t**) _self);
	}
}

void vkk_uiCheckBox_label(vkk_uiCheckBox_t* self,
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

int vkk_uiCheckBox_get(vkk_uiCheckBox_t* self)
{
	ASSERT(self);

	return self->value;
}

void vkk_uiCheckBox_set(vkk_uiCheckBox_t* self, int value)
{
	ASSERT(self);

	self->value = value ? 1 : 0;
}
