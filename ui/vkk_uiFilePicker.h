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

#ifndef vkk_uiFilePicker_H
#define vkk_uiFilePicker_H

#include "../vkk_platform.h"

typedef struct vkk_uiFilePicker_s
{
#ifdef ANDROID
	vkk_engine_t* engine;
#else
	vkk_uiWindow_t       base;
	vkk_uiFileList_t*    file_list;
	vkk_uiBulletBox_t*   bulletbox_select;
	vkk_uiBulletBox_t*   bulletbox_cancel;
	vkk_uiBulletBox_t*   bulletbox_folder;
	vkk_uiInputWindow_t* input_window;

	void* document_priv;
	vkk_platformCmd_documentFn document_fn;

	int create;
#endif
} vkk_uiFilePicker_t;

vkk_uiFilePicker_t* vkk_uiFilePicker_new(vkk_uiScreen_t* screen);
void                vkk_uiFilePicker_delete(vkk_uiFilePicker_t** _self);
void                vkk_uiFilePicker_documentCreate(vkk_uiFilePicker_t* self,
                                                    void* document_priv,
                                                    vkk_platformCmd_documentFn document_fn,
                                                    const char* type,
                                                    const char* mode,
                                                    const char* path,
                                                    const char* name,
                                                    const char* ext);
void                vkk_uiFilePicker_documentOpen(vkk_uiFilePicker_t* self,
                                                  void* document_priv,
                                                  vkk_platformCmd_documentFn document_fn,
                                                  const char* type,
                                                  const char* mode,
                                                  const char* path,
                                                  const char* ext);

#endif
