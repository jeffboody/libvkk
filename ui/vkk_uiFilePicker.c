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
                                void* document_priv,
                                vkk_platformCmd_documentFn document_fn,
                                const char* type,
                                const char* mode,
                                const char* path,
                                const char* name,
                                const char* ext)
{
	// document_priv may be NULL
	ASSERT(self);
	ASSERT(document_fn);
	ASSERT(type);
	ASSERT(mode);
	ASSERT(path);
	ASSERT(name);
	ASSERT(ext);

	vkk_engine_platformCmdDocumentCreate(self->engine,
	                                     document_priv, document_fn,
	                                     type, mode, name, ext);
}

void
vkk_uiFilePicker_documentOpen(vkk_uiFilePicker_t* self,
                              void* document_priv,
                              vkk_platformCmd_documentFn document_fn,
                              const char* type,
                              const char* mode,
                              const char* path,
                              const char* ext)
{
	// document_priv may be NULL
	ASSERT(self);
	ASSERT(document_fn);
	ASSERT(type);
	ASSERT(mode);
	ASSERT(path);
	ASSERT(ext);

	vkk_engine_platformCmdDocumentOpen(self->engine,
	                                   document_priv, document_fn,
	                                   type, mode);
}

void vkk_uiFilePicker_documentName(vkk_uiFilePicker_t* self,
                                   void* document_priv,
                                   vkk_platformCmd_documentFn document_fn,
                                   const char* path,
                                   const char* ext)
{
	// unsupported on Android
	LOGW("unsupported");
	ASSERT(0);
}

/***********************************************************
* private - LINUX                                          *
***********************************************************/

#else // LINUX

#include <errno.h>

static int vkk_uiFilePicker_mkdir(const char* fname)
{
       ASSERT(fname);

       int  len = strnlen(fname, 255);
       char dir[256];
       int  i;
       for(i = 0; i < len; ++i)
       {
               dir[i]     = fname[i];
               dir[i + 1] = '\0';

               if(dir[i] == '/')
               {
                       if(access(dir, R_OK) == 0)
                       {
                               // dir already exists
                               continue;
                       }

                       // try to mkdir
                       if(mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
                       {
                               if(errno == EEXIST)
                               {
                                       // already exists
                               }
                               else
                               {
                                       LOGE("mkdir %s failed", dir);
                                       return 0;
                               }
                       }
               }
       }

       return 1;
}

static void
vkk_uiFilePicker_clickSelect(vkk_uiWidget_t* widget,
                             float x, float y)
{
	ASSERT(widget);

	vkk_uiFilePicker_t* self;
	self = (vkk_uiFilePicker_t*) vkk_uiWidget_priv(widget);

	// input string is determined in input function
	vkk_uiFileList_input(&self->file_list->base, "");
}

static void
vkk_uiFilePicker_clickFolder(vkk_uiWidget_t* widget,
                             float x, float y)
{
	ASSERT(widget);

	vkk_uiFilePicker_t* self;
	self = (vkk_uiFilePicker_t*) vkk_uiWidget_priv(widget);

	vkk_uiInputWindow_labelText(self->input_window,
	                            "%s", "");
	vkk_uiScreen_windowPush(widget->screen,
	                        (vkk_uiWindow_t*)
	                        self->input_window);
}

/***********************************************************
* public - LINUX                                           *
***********************************************************/

vkk_uiFilePicker_t*
vkk_uiFilePicker_new(vkk_uiScreen_t* screen)
{
	ASSERT(screen);

	vkk_uiWindowFn_t wfn =
	{
		.priv = NULL,
	};

	uint32_t flags = VKK_UI_WINDOW_FLAG_TITLE  |
		             VKK_UI_WINDOW_FLAG_LAYER1 |
		             VKK_UI_WINDOW_FLAG_FOOTER;

	vkk_uiFilePicker_t* self;
	self = (vkk_uiFilePicker_t*)
	       vkk_uiWindow_new(screen,
	                        sizeof(vkk_uiFilePicker_t),
	                        &wfn, flags);
	if(self == NULL)
	{
		return NULL;
	}

	self->file_list = vkk_uiFileList_new(screen, self);
	if(self->file_list == NULL)
	{
		goto fail_file_list;
	}

	vkk_uiBulletBoxFn_t bbfn_select =
	{
		.priv     = self,
		.click_fn = vkk_uiFilePicker_clickSelect
	};

	const char* sprite_array_select[] =
	{
		"vkk/ui/icons/ic_check_white_24dp.png",
		NULL
	};
	self->bulletbox_select = vkk_uiBulletBox_newFooterItem(screen,
	                                                       &bbfn_select,
	                                                       sprite_array_select);
	if(self->bulletbox_select == NULL)
	{
		goto fail_select;
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

	vkk_uiBulletBoxFn_t bbfn_folder =
	{
		.priv     = self,
		.click_fn = vkk_uiFilePicker_clickFolder
	};

	const char* sprite_array_folder[] =
	{
		"vkk/ui/icons/ic_create_new_folder_white_24dp.png",
		NULL
	};

	self->bulletbox_folder = vkk_uiBulletBox_newFooterItem(screen,
	                                                       &bbfn_folder,
	                                                       sprite_array_folder);
	if(self->bulletbox_folder == NULL)
	{
		goto fail_folder;
	}

	vkk_uiInputWindowFn_t iwfn =
	{
		.priv     = self->file_list,
		.input_fn = vkk_uiFileList_mkdir,
	};

	self->input_window = vkk_uiInputWindow_new(screen,
	                                           &iwfn);
	if(self->input_window == NULL)
	{
		goto fail_input_window;
	}

	vkk_uiInputWindow_label(self->input_window,
	                        "%s", "New Folder");
	vkk_uiInputWindow_labelAccept(self->input_window, "%s",
	                              "Create");
	vkk_uiBulletBox_label(self->bulletbox_select,
	                      "%s", "Select");
	vkk_uiBulletBox_label(self->bulletbox_cancel,
	                      "%s", "Cancel");
	vkk_uiBulletBox_label(self->bulletbox_folder,
	                      "%s", "New Folder");

	vkk_uiWindow_t*  window = (vkk_uiWindow_t*) self;
	vkk_uiListBox_t* footer = vkk_uiWindow_footer(window);
	vkk_uiListBox_add(footer, (vkk_uiWidget_t*)
	                  self->bulletbox_select);
	vkk_uiListBox_add(footer, (vkk_uiWidget_t*)
	                  self->bulletbox_cancel);
	vkk_uiListBox_add(footer, (vkk_uiWidget_t*)
	                  self->bulletbox_folder);

	vkk_uiLayer_t* layer1 = vkk_uiWindow_layer1(window);
	vkk_uiLayer_add(layer1, (vkk_uiWidget_t*) self->file_list);

	// success
	return self;

	// failure
	fail_input_window:
		vkk_uiBulletBox_delete(&self->bulletbox_folder);
	fail_folder:
		vkk_uiBulletBox_delete(&self->bulletbox_cancel);
	fail_cancel:
		vkk_uiBulletBox_delete(&self->bulletbox_select);
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
		vkk_uiWindow_t*  window = &self->base;
		vkk_uiListBox_t* footer = vkk_uiWindow_footer(window);
		vkk_uiLayer_t*   layer1 = vkk_uiWindow_layer1(window);
		vkk_uiListBox_clear(footer);
		vkk_uiLayer_clear(layer1);
		vkk_uiInputWindow_delete(&self->input_window);
		vkk_uiBulletBox_delete(&self->bulletbox_folder);
		vkk_uiBulletBox_delete(&self->bulletbox_cancel);
		vkk_uiBulletBox_delete(&self->bulletbox_select);
		vkk_uiFileList_delete(&self->file_list);
		vkk_uiWindow_delete((vkk_uiWindow_t**) _self);
	}
}

void
vkk_uiFilePicker_documentCreate(vkk_uiFilePicker_t* self,
                                void* document_priv,
                                vkk_platformCmd_documentFn document_fn,
                                const char* type,
                                const char* mode,
                                const char* path,
                                const char* name,
                                const char* ext)
{
	// document_priv may be NULL
	ASSERT(self);
	ASSERT(document_fn);
	ASSERT(type);
	ASSERT(mode);
	ASSERT(path);
	ASSERT(name);
	ASSERT(ext);

	vkk_uiWindow_t*  window = &self->base;
	vkk_uiWidget_t*  widget = &window->base;
	vkk_uiListBox_t* footer = vkk_uiWindow_footer(window);

	char dname[256];
	snprintf(dname, 256, "%s/", path);
	if(vkk_uiFilePicker_mkdir(dname))
	{
		self->document_priv = document_priv;
		self->document_fn   = document_fn;
		self->mode          = VKK_UI_FILEPICKER_MODE_CREATE;

		vkk_uiWindow_label(window, "%s", "Save As");

		vkk_uiListBox_clear(footer);
		vkk_uiListBox_add(footer, (vkk_uiWidget_t*)
		                  self->bulletbox_select);
		vkk_uiListBox_add(footer, (vkk_uiWidget_t*)
		                  self->bulletbox_cancel);
		vkk_uiListBox_add(footer, (vkk_uiWidget_t*)
		                  self->bulletbox_folder);

		vkk_uiFileList_reset(self->file_list, path, name, ext);
		vkk_uiScreen_windowPush(widget->screen, window);
	}
}

void
vkk_uiFilePicker_documentOpen(vkk_uiFilePicker_t* self,
                              void* document_priv,
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

	vkk_uiWindow_t*  window = &self->base;
	vkk_uiWidget_t*  widget = &window->base;
	vkk_uiListBox_t* footer = vkk_uiWindow_footer(window);

	char dname[256];
	snprintf(dname, 256, "%s/", path);
	if(vkk_uiFilePicker_mkdir(dname))
	{
		self->document_priv = document_priv;
		self->document_fn   = document_fn;
		self->mode          = VKK_UI_FILEPICKER_MODE_OPEN;

		vkk_uiWindow_label(window, "%s", "Import");

		vkk_uiListBox_clear(footer);
		vkk_uiListBox_add(footer, (vkk_uiWidget_t*)
		                  self->bulletbox_select);
		vkk_uiListBox_add(footer, (vkk_uiWidget_t*)
		                  self->bulletbox_cancel);

		vkk_uiFileList_reset(self->file_list, path, "", ext);
		vkk_uiScreen_windowPush(widget->screen, window);
	}
}

void vkk_uiFilePicker_documentName(vkk_uiFilePicker_t* self,
                                   void* document_priv,
                                   vkk_platformCmd_documentFn document_fn,
                                   const char* path,
                                   const char* ext)
{
	ASSERT(self);
	ASSERT(path);
	ASSERT(ext);

	vkk_uiWindow_t*  window = &self->base;
	vkk_uiWidget_t*  widget = &window->base;
	vkk_uiListBox_t* footer = vkk_uiWindow_footer(window);

	char dname[256];
	snprintf(dname, 256, "%s/", path);
	if(vkk_uiFilePicker_mkdir(dname))
	{
		self->document_priv = document_priv;
		self->document_fn   = document_fn;
		self->mode          = VKK_UI_FILEPICKER_MODE_NAME;

		vkk_uiWindow_label(window, "%s", "File Name");

		vkk_uiListBox_clear(footer);
		vkk_uiListBox_add(footer, (vkk_uiWidget_t*)
		                  self->bulletbox_select);
		vkk_uiListBox_add(footer, (vkk_uiWidget_t*)
		                  self->bulletbox_cancel);

		vkk_uiFileList_reset(self->file_list, path, "", ext);
		vkk_uiScreen_windowPush(widget->screen, window);
	}
}

#endif // LINUX
