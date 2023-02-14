/*
 * Copyright (c) 2020 Jeff Boody
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
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "vkk"
#include "../../libcc/cc_log.h"
#include "../../libcc/cc_memory.h"
#include "../vkk_ui.h"

/***********************************************************
* private widget                                           *
***********************************************************/

static void
vkk_uiInputWindow_input(vkk_uiWidget_t* widget,
                        const char* string)
{
	ASSERT(widget);
	ASSERT(string);

	// widget is the text input widget

	vkk_uiInputWindow_t* self;
	self = (vkk_uiInputWindow_t*) vkk_uiWidget_priv(widget);

	// call input_fn with the InputWindow widget
	// so the correct priv is available
	vkk_uiWidgetInput_fn input_fn = self->input_fn;
	if(input_fn)
	{
		(*input_fn)((vkk_uiWidget_t*) self, string);
	}
	vkk_uiScreen_windowPop(widget->screen);
}

static void
vkk_uiInputWindow_clickAccept(vkk_uiWidget_t* widget,
                              float x, float y)
{
	ASSERT(widget);

	// widget is the accept button

	vkk_uiInputWindow_t* self;
	self = (vkk_uiInputWindow_t*) vkk_uiWidget_priv(widget);

	const char* string = vkk_uiText_string(self->text);

	// call input_fn with the InputWindow widget
	// so the correct priv is available
	vkk_uiWidgetInput_fn input_fn = self->input_fn;
	if(input_fn)
	{
		(*input_fn)((vkk_uiWidget_t*) self, string);
	}
	vkk_uiScreen_windowPop(widget->screen);
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_uiInputWindow_t*
vkk_uiInputWindow_new(vkk_uiScreen_t* screen,
                      vkk_uiInputWindowFn_t* iwfn)
{
	ASSERT(screen);
	ASSERT(iwfn);

	vkk_uiWindowFn_t wfn =
	{
		.priv       = iwfn->priv,
		.refresh_fn = iwfn->refresh_fn,
	};

	uint32_t flags = VKK_UI_WINDOW_FLAG_TITLE        |
	                 VKK_UI_WINDOW_FLAG_PAGE_DEFAULT |
	                 VKK_UI_WINDOW_FLAG_FOOTER;

	vkk_uiInputWindow_t* self;
	self = (vkk_uiInputWindow_t*)
	       vkk_uiWindow_new(screen,
	                        sizeof(vkk_uiInputWindow_t),
	                        &wfn, flags);
	if(self == NULL)
	{
		return NULL;
	}

	vkk_uiTextFn_t tfn =
	{
		.priv     = self,
		.input_fn = vkk_uiInputWindow_input,
	};

	self->text = vkk_uiText_newPageTextInput(screen, &tfn);
	if(self->text == NULL)
	{
		goto fail_text;
	}
	vkk_uiWindow_focus((vkk_uiWindow_t*) self,
	                   (vkk_uiWidget_t*) self->text);

	vkk_uiBulletBoxFn_t bbfn_accept =
	{
		.priv     = self,
		.click_fn = vkk_uiInputWindow_clickAccept
	};

	const char* sprite_array_accept[] =
	{
		"vkk/ui/icons/ic_check_white_24dp.png",
		NULL
	};
	self->bulletbox_accept = vkk_uiBulletBox_newFooterItem(screen,
	                                                       &bbfn_accept,
	                                                       sprite_array_accept);
	if(self->bulletbox_accept == NULL)
	{
		goto fail_accept;
	}

	vkk_uiBulletBoxFn_t bbfn_cancel =
	{
		.click_fn = vkk_uiWidget_clickBack
	};

	const char* sprite_array_cancel[] =
	{
		"vkk/ui/icons/ic_close_white_24dp.png",
		NULL
	};
	self->bulletbox_cancel = vkk_uiBulletBox_newFooterItem(screen,
	                                                       &bbfn_cancel,
	                                                       sprite_array_cancel);
	if(self->bulletbox_cancel == NULL)
	{
		goto fail_cancel;
	}

	self->input_fn = iwfn->input_fn;

	vkk_uiText_label(self->text, "%s", "");
	vkk_uiBulletBox_label(self->bulletbox_accept,
	                      "%s", "Accept");
	vkk_uiBulletBox_label(self->bulletbox_cancel,
	                      "%s", "Cancel");

	vkk_uiWindow_t*  window = &self->base;
	vkk_uiListBox_t* page   = vkk_uiWindow_page(window);
	vkk_uiListBox_t* footer = vkk_uiWindow_footer(window);
	vkk_uiListBox_add(page,   (vkk_uiWidget_t*) self->text);
	vkk_uiListBox_add(footer, (vkk_uiWidget_t*) self->bulletbox_accept);
	vkk_uiListBox_add(footer, (vkk_uiWidget_t*) self->bulletbox_cancel);

	// success
	return self;

	// failure
	fail_cancel:
		vkk_uiBulletBox_delete(&self->bulletbox_accept);
	fail_accept:
		vkk_uiText_delete(&self->text);
	fail_text:
		vkk_uiWindow_delete((vkk_uiWindow_t**) &self);
	return NULL;
}

void vkk_uiInputWindow_delete(vkk_uiInputWindow_t** _self)
{
	ASSERT(_self);

	vkk_uiInputWindow_t* self = *_self;
	if(self)
	{
		vkk_uiWindow_t*  window = &self->base;
		vkk_uiListBox_t* page   = vkk_uiWindow_page(window);
		vkk_uiListBox_t* footer = vkk_uiWindow_footer(window);
		vkk_uiListBox_clear(page);
		vkk_uiListBox_clear(footer);
		vkk_uiBulletBox_delete(&self->bulletbox_cancel);
		vkk_uiBulletBox_delete(&self->bulletbox_accept);
		vkk_uiText_delete(&self->text);
		vkk_uiWindow_delete((vkk_uiWindow_t**) _self);
	}
}

void vkk_uiInputWindow_label(vkk_uiInputWindow_t* self,
                           const char* fmt, ...)
{
	ASSERT(self);
	ASSERT(fmt);

	// decode string
	char tmp_string[256];
	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(tmp_string, 256, fmt, argptr);
	va_end(argptr);

	vkk_uiWindow_t* window;
	window = (vkk_uiWindow_t*) self;
	vkk_uiWindow_label(window, "%s", tmp_string);
}

void vkk_uiInputWindow_labelAccept(vkk_uiInputWindow_t* self,
                                 const char* fmt, ...)
{
	ASSERT(self);
	ASSERT(fmt);

	// decode string
	char tmp_string[256];
	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(tmp_string, 256, fmt, argptr);
	va_end(argptr);

	vkk_uiBulletBox_label(self->bulletbox_accept,
	                      "%s", tmp_string);
}

void vkk_uiInputWindow_labelCancel(vkk_uiInputWindow_t* self,
                                   const char* fmt, ...)
{
	ASSERT(self);
	ASSERT(fmt);

	// decode string
	char tmp_string[256];
	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(tmp_string, 256, fmt, argptr);
	va_end(argptr);

	vkk_uiBulletBox_label(self->bulletbox_cancel,
	                      "%s", tmp_string);
}

void vkk_uiInputWindow_labelText(vkk_uiInputWindow_t* self,
                                 const char* fmt, ...)
{
	ASSERT(self);
	ASSERT(fmt);

	// decode string
	char tmp_string[256];
	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(tmp_string, 256, fmt, argptr);
	va_end(argptr);

	vkk_uiText_label(self->text, "%s", tmp_string);
}
