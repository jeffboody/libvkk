/*
 * Copyright (c) 2021 Jeff Boody
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LOG_TAG "vkk"
#include "../../libcc/cc_log.h"
#include "../vkk_platform.h"
#include "../vkk_ui.h"

/***********************************************************
* public - ANDROID                                         *
***********************************************************/

#ifdef ANDROID

#include "../../libcc/cc_memory.h"

vkk_uiFilePicker_t*
vkk_uiFilePicker_new(vkk_uiScreen_t* screen)
{
	ASSERT(screen);

	vkk_uiFilePicker_t* self;
	self = (vkk_uiFilePicker_t*)
	       CALLOC(1, sizeof(vkk_uiFilePicker_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->engine = screen->engine;

	return self;
}

void vkk_uiFilePicker_delete(vkk_uiFilePicker_t** _self)
{
	ASSERT(_self);

	vkk_uiFilePicker_t* self = *_self;
	if(self)
	{
		FREE(self);
		*_self = NULL;
	}
}

void
vkk_uiFilePicker_documentCreate(vkk_uiFilePicker_t* self,
                                void* priv,
                                vkk_platformCmd_documentFn document_fn,
                                const char* type,
                                const char* mode,
                                const char* path,
                                const char* name,
                                const char* ext)
{
	// priv may be NULL
	ASSERT(self);
	ASSERT(document_fn);
	ASSERT(type);
	ASSERT(mode);
	ASSERT(path);
	ASSERT(name);
	ASSERT(ext);

	vkk_platformCmdInfo_t info =
	{
		.cmd         = VKK_PLATFORM_CMD_DOCUMENT_CREATE,
		.priv        = priv,
		.document_fn = document_fn
	};

	snprintf(info.msg, 256,
	         "{\"title\":\"%s%s\",\"type\":\"%s\",\"mode\":\"%s\"}",
	         name, ext, type, mode);

	vkk_engine_platformCmd(self->engine, &info);
}

void
vkk_uiFilePicker_documentOpen(vkk_uiFilePicker_t* self,
                              void* priv,
                              vkk_platformCmd_documentFn document_fn,
                              const char* type,
                              const char* mode,
                              const char* path,
                              const char* ext)
{
	// priv may be NULL
	ASSERT(self);
	ASSERT(document_fn);
	ASSERT(type);
	ASSERT(mode);
	ASSERT(path);
	ASSERT(ext);

	vkk_platformCmdInfo_t info =
	{
		.cmd         = VKK_PLATFORM_CMD_DOCUMENT_OPEN,
		.priv        = priv,
		.document_fn = document_fn
	};

	snprintf(info.msg, 256,
	         "{\"type\":\"%s\",\"mode\":\"%s\"}",
	         type, mode);

	vkk_engine_platformCmd(self->engine, &info);
}

/***********************************************************
* private - LINUX                                          *
***********************************************************/

#else // LINUX

static int
vkk_uiFilePicker_clickSelect(vkk_uiWidget_t* widget,
                             int state, float x, float y)
{
	ASSERT(widget);

	vkk_uiFilePicker_t* self;
	self = (vkk_uiFilePicker_t*)
	       vkk_uiWidget_widgetFnPriv(widget);

	vkk_uiScreen_t* screen = widget->screen;
	vkk_engine_t*   engine = screen->engine;

	if(state == VKK_UI_WIDGET_POINTER_UP)
	{
		vkk_platformCmdInfo_t info =
		{
			.cmd         = VKK_PLATFORM_CMD_DOCUMENT_OPEN,
			.priv        = self->priv,
			.document_fn = self->document_fn,
		};

		if(self->create)
		{
			info.cmd = VKK_PLATFORM_CMD_DOCUMENT_CREATE;
		}

		vkk_uiFileList_filepath(self->file_list, info.msg);
		vkk_engine_platformCmd(engine, &info);

		vkk_uiScreen_windowPop(screen);
	}
	return 1;
}

static int
vkk_uiFilePicker_clickBack(vkk_uiWidget_t* widget,
                           int state, float x, float y)
{
	ASSERT(widget);

	if(state == VKK_UI_WIDGET_POINTER_UP)
	{
		vkk_uiScreen_windowPop(widget->screen);
	}
	return 1;
}

static int
vkk_uiFilePicker_clickFolder(vkk_uiWidget_t* widget,
                             int state, float x, float y)
{
	ASSERT(widget);

	vkk_uiFilePicker_t* self;
	self = (vkk_uiFilePicker_t*)
	       vkk_uiWidget_widgetFnPriv(widget);

	if(state == VKK_UI_WIDGET_POINTER_UP)
	{
		vkk_uiTextEntry_labelText(self->text_entry,
		                          "%s", "");
		vkk_uiScreen_windowPush(widget->screen,
		                        (vkk_uiWindow_t*)
		                        self->text_entry);
	}
	return 1;
}

/***********************************************************
* public - LINUX                                           *
***********************************************************/

vkk_uiFilePicker_t*
vkk_uiFilePicker_new(vkk_uiScreen_t* screen)
{
	ASSERT(screen);

	vkk_uiWindowInfo_t info =
	{
		.flags = VKK_UI_WINDOW_FLAG_TITLE  |
		         VKK_UI_WINDOW_FLAG_LAYER1 |
		         VKK_UI_WINDOW_FLAG_FOOTER,
		.label = "File Picker",
		.fn =
		{
			.click_fn = vkk_uiFilePicker_clickBack
		},
	};

	vkk_uiFilePicker_t* self;
	self = (vkk_uiFilePicker_t*)
	       vkk_uiWindow_new(screen,
	                        sizeof(vkk_uiFilePicker_t),
	                        &info);
	if(self == NULL)
	{
		return NULL;
	}

	self->file_list = vkk_uiFileList_new(screen);
	if(self->file_list == NULL)
	{
		goto fail_file_list;
	}

	vkk_uiWidgetFn_t fn_select =
	{
		.priv     = self,
		.click_fn = vkk_uiFilePicker_clickSelect
	};

	const char* sprite_select[] =
	{
		"vkk/ui/icons/ic_check_white_24dp.png",
		NULL
	};
	self->bulletbox_select = vkk_uiBulletbox_newFooterItem(screen,
	                                                       &fn_select,
	                                                       sprite_select);
	if(self->bulletbox_select == NULL)
	{
		goto fail_select;
	}

	vkk_uiWidgetFn_t fn_cancel =
	{
		.click_fn = vkk_uiFilePicker_clickBack
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

	vkk_uiWidgetFn_t fn_folder =
	{
		.priv     = self,
		.click_fn = vkk_uiFilePicker_clickFolder
	};

	const char* sprite_folder[] =
	{
		"vkk/ui/icons/ic_create_new_folder_white_24dp.png",
		NULL
	};
	self->bulletbox_folder = vkk_uiBulletbox_newFooterItem(screen,
	                                                       &fn_folder,
	                                                       sprite_folder);
	if(self->bulletbox_folder == NULL)
	{
		goto fail_folder;
	}

	self->text_entry = vkk_uiTextEntry_new(screen,
	                                       (void*) self->file_list,
	                                       "New Folder",
	                                       vkk_uiFileList_mkdir);
	if(self->text_entry == NULL)
	{
		goto fail_text_entry;
	}

	vkk_uiBulletbox_label(self->bulletbox_select,
	                      "%s", "Select");
	vkk_uiBulletbox_label(self->bulletbox_cancel,
	                      "%s", "Cancel");
	vkk_uiBulletbox_label(self->bulletbox_folder,
	                      "%s", "New Folder");
	vkk_uiTextEntry_labelAccept(self->text_entry, "%s",
	                            "Create");

	vkk_uiWindow_t*  window = (vkk_uiWindow_t*) self;
	vkk_uiListbox_t* footer = vkk_uiWindow_footer(window);
	vkk_uiListbox_add(footer, (vkk_uiWidget_t*)
	                  self->bulletbox_select);
	vkk_uiListbox_add(footer, (vkk_uiWidget_t*)
	                  self->bulletbox_cancel);
	vkk_uiListbox_add(footer, (vkk_uiWidget_t*)
	                  self->bulletbox_folder);

	vkk_uiLayer_t* layer1 = vkk_uiWindow_layer1(window);
	vkk_uiLayer_add(layer1, (vkk_uiWidget_t*) self->file_list);

	// success
	return self;

	// failure
	fail_text_entry:
		vkk_uiBulletbox_delete(&self->bulletbox_folder);
	fail_folder:
		vkk_uiBulletbox_delete(&self->bulletbox_cancel);
	fail_cancel:
		vkk_uiBulletbox_delete(&self->bulletbox_select);
	fail_select:
		vkk_uiFileList_delete(&self->file_list);
	fail_file_list:
		vkk_uiWindow_delete((vkk_uiWindow_t**) &self);
	return NULL;
}

void vkk_uiFilePicker_delete(vkk_uiFilePicker_t** _self)
{
	ASSERT(_self);

	vkk_uiFilePicker_t* self = *_self;
	if(self)
	{
		vkk_uiWindow_t*  window = (vkk_uiWindow_t*) self;
		vkk_uiListbox_t* footer = vkk_uiWindow_footer(window);
		vkk_uiLayer_t*   layer1 = vkk_uiWindow_layer1(window);
		vkk_uiListbox_clear(footer);
		vkk_uiLayer_clear(layer1);
		vkk_uiTextEntry_delete(&self->text_entry);
		vkk_uiBulletbox_delete(&self->bulletbox_folder);
		vkk_uiBulletbox_delete(&self->bulletbox_cancel);
		vkk_uiBulletbox_delete(&self->bulletbox_select);
		vkk_uiFileList_delete(&self->file_list);
		vkk_uiWindow_delete((vkk_uiWindow_t**) _self);
	}
}

void
vkk_uiFilePicker_documentCreate(vkk_uiFilePicker_t* self,
                                void* priv,
                                vkk_platformCmd_documentFn document_fn,
                                const char* type,
                                const char* mode,
                                const char* path,
                                const char* name,
                                const char* ext)
{
	// priv may be NULL
	ASSERT(self);
	ASSERT(document_fn);
	ASSERT(type);
	ASSERT(mode);
	ASSERT(path);
	ASSERT(name);
	ASSERT(ext);

	vkk_uiWidget_t*  base   = (vkk_uiWidget_t*) self;
	vkk_uiWindow_t*  window = (vkk_uiWindow_t*) self;
	vkk_uiListbox_t* footer = vkk_uiWindow_footer(window);

	self->priv        = priv;
	self->document_fn = document_fn;
	self->create      = 1;

	vkk_uiWindow_label(window, "%s", "Save As");

	vkk_uiListbox_clear(footer);
	vkk_uiListbox_add(footer, (vkk_uiWidget_t*)
	                  self->bulletbox_select);
	vkk_uiListbox_add(footer, (vkk_uiWidget_t*)
	                  self->bulletbox_cancel);
	vkk_uiListbox_add(footer, (vkk_uiWidget_t*)
	                  self->bulletbox_folder);

	vkk_uiFileList_reset(self->file_list, path, name, ext);
	vkk_uiScreen_windowPush(base->screen, window);
}

void
vkk_uiFilePicker_documentOpen(vkk_uiFilePicker_t* self,
                              void* priv,
                              vkk_platformCmd_documentFn document_fn,
                              const char* type,
                              const char* mode,
                              const char* path,
                              const char* ext)
{
	ASSERT(self);
	ASSERT(type);
	ASSERT(mode);
	ASSERT(path);
	ASSERT(ext);

	vkk_uiWidget_t*  base   = (vkk_uiWidget_t*) self;
	vkk_uiWindow_t*  window = (vkk_uiWindow_t*) self;
	vkk_uiListbox_t* footer = vkk_uiWindow_footer(window);

	self->priv        = priv;
	self->document_fn = document_fn;
	self->create      = 0;

	vkk_uiWindow_label(window, "%s", "Import");

	vkk_uiListbox_clear(footer);
	vkk_uiListbox_add(footer, (vkk_uiWidget_t*)
	                  self->bulletbox_select);
	vkk_uiListbox_add(footer, (vkk_uiWidget_t*)
	                  self->bulletbox_cancel);

	vkk_uiFileList_reset(self->file_list, path, "", ext);
	vkk_uiScreen_windowPush(base->screen, window);
}

#endif // LINUX
