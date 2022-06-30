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
vkk_uiTextEntry_clickEnter(vkk_uiWidget_t* widget,
                           const char* string)
{
	ASSERT(widget);
	ASSERT(string);

	vkk_uiTextEntry_t* self;
	self = (vkk_uiTextEntry_t*)
	       vkk_uiWidget_widgetFnPriv(widget);

	vkk_uiTextEntry_acceptFn accept_fn = self->accept_fn;
	(*accept_fn)(self->priv, string);

	vkk_uiScreen_windowPop(widget->screen);
}

static int
vkk_uiTextEntry_clickAccept(vkk_uiWidget_t* widget,
                            int state,
                            float x, float y)
{
	ASSERT(widget);

	if(state == VKK_UI_WIDGET_POINTER_UP)
	{
		vkk_uiTextEntry_t* ui_enter_text;
		vkk_uiText_t*      text;
		ui_enter_text = (vkk_uiTextEntry_t*)
		                vkk_uiWidget_widgetFnPriv(widget);
		text          = ui_enter_text->text;

		vkk_uiTextEntry_clickEnter((vkk_uiWidget_t*) text,
		                           text->string);
	}
	return 1;
}

static int
vkk_uiTextEntry_clickCancel(vkk_uiWidget_t* widget,
                            int state,
                            float x, float y)
{
	ASSERT(widget);

	if(state == VKK_UI_WIDGET_POINTER_UP)
	{
		vkk_uiScreen_windowPop(widget->screen);
	}
	return 1;
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_uiTextEntry_t*
vkk_uiTextEntry_new(vkk_uiScreen_t* screen,
                    void* priv,
                    const char* label,
                    vkk_uiTextEntry_acceptFn accept_fn)
{
	ASSERT(screen);
	ASSERT(accept_fn);

	vkk_uiWindowInfo_t info =
	{
		.flags = VKK_UI_WINDOW_FLAG_TITLE        |
		         VKK_UI_WINDOW_FLAG_PAGE_DEFAULT |
		         VKK_UI_WINDOW_FLAG_FOOTER,
		.label = label,
		.fn =
		{
			.click_fn = vkk_uiTextEntry_clickCancel
		},
	};

	vkk_uiTextEntry_t* self;
	self = (vkk_uiTextEntry_t*)
	       vkk_uiWindow_new(screen,
	                        sizeof(vkk_uiTextEntry_t),
	                        &info);
	if(self == NULL)
	{
		return NULL;
	}

	self->priv      = priv;
	self->accept_fn = accept_fn;

	self->text = vkk_uiText_newPageTextEntry(screen, self,
	                                         vkk_uiTextEntry_clickEnter);
	if(self->text == NULL)
	{
		goto fail_text;
	}
	vkk_uiWindow_focus((vkk_uiWindow_t*) self,
	                   (vkk_uiWidget_t*) self->text);

	vkk_uiWidgetFn_t fn_accept =
	{
		.priv     = (void*) self,
		.click_fn = vkk_uiTextEntry_clickAccept
	};

	const char* sprite_accept[] =
	{
		"vkk/ui/icons/ic_check_white_24dp.png",
		NULL
	};
	self->bulletbox_accept = vkk_uiBulletbox_newFooterItem(screen,
	                                                       &fn_accept,
	                                                       sprite_accept);
	if(self->bulletbox_accept == NULL)
	{
		goto fail_accept;
	}

	vkk_uiWidgetFn_t fn_cancel =
	{
		.click_fn = vkk_uiTextEntry_clickCancel
	};

	const char* sprite_cancel[] =
	{
		"vkk/ui/icons/ic_close_white_24dp.png",
		NULL
	};
	self->bulletbox_cancel = vkk_uiBulletbox_newFooterItem(screen,
	                                                       &fn_cancel,
	                                                       sprite_cancel);
	if(self->bulletbox_cancel == NULL)
	{
		goto fail_cancel;
	}

	vkk_uiText_label(self->text, "%s", "");
	vkk_uiBulletbox_label(self->bulletbox_accept,
	                      "%s", "Accept");
	vkk_uiBulletbox_label(self->bulletbox_cancel,
	                      "%s", "Cancel");

	vkk_uiWindow_t*  window = (vkk_uiWindow_t*) self;
	vkk_uiListbox_t* page   = vkk_uiWindow_page(window);
	vkk_uiListbox_t* footer = vkk_uiWindow_footer(window);
	vkk_uiListbox_add(page,   (vkk_uiWidget_t*) self->text);
	vkk_uiListbox_add(footer, (vkk_uiWidget_t*) self->bulletbox_accept);
	vkk_uiListbox_add(footer, (vkk_uiWidget_t*) self->bulletbox_cancel);

	// success
	return self;

	// failure
	fail_cancel:
		vkk_uiBulletbox_delete(&self->bulletbox_accept);
	fail_accept:
		vkk_uiText_delete(&self->text);
	fail_text:
		vkk_uiWindow_delete((vkk_uiWindow_t**) &self);
	return NULL;
}

void vkk_uiTextEntry_delete(vkk_uiTextEntry_t** _self)
{
	ASSERT(_self);

	vkk_uiTextEntry_t* self = *_self;
	if(self)
	{
		vkk_uiWindow_t*  window = (vkk_uiWindow_t*) self;
		vkk_uiListbox_t* page   = vkk_uiWindow_page(window);
		vkk_uiListbox_t* footer = vkk_uiWindow_footer(window);
		vkk_uiListbox_clear(page);
		vkk_uiListbox_clear(footer);
		vkk_uiBulletbox_delete(&self->bulletbox_cancel);
		vkk_uiBulletbox_delete(&self->bulletbox_accept);
		vkk_uiText_delete(&self->text);
		vkk_uiWindow_delete((vkk_uiWindow_t**) _self);
	}
}

void vkk_uiTextEntry_label(vkk_uiTextEntry_t* self,
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

void vkk_uiTextEntry_labelAccept(vkk_uiTextEntry_t* self,
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

	vkk_uiBulletbox_label(self->bulletbox_accept,
	                      "%s", tmp_string);
}

void vkk_uiTextEntry_labelCancel(vkk_uiTextEntry_t* self,
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

	vkk_uiBulletbox_label(self->bulletbox_cancel,
	                      "%s", tmp_string);
}

void vkk_uiTextEntry_labelText(vkk_uiTextEntry_t* self,
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
