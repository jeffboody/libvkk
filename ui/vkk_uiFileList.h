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

#ifndef vkk_uiFileList_H
#define vkk_uiFileList_H

typedef struct vkk_uiFileList_s
{
	vkk_uiWidget_t     base;
	vkk_uiText_t*      heading_name;
	vkk_uiText_t*      text_name;
	vkk_uiText_t*      text_ext;
	vkk_uiText_t*      heading_path;
	vkk_uiBulletBox_t* bulletbox_path;
	vkk_uiListBox_t*   listbox_files;

	int dirty;
} vkk_uiFileList_t;

vkk_uiFileList_t* vkk_uiFileList_new(vkk_uiScreen_t* screen,
                                     vkk_uiFilePicker_t* parent);
void              vkk_uiFileList_delete(vkk_uiFileList_t** _self);
void              vkk_uiFileList_reset(vkk_uiFileList_t* self,
                                       const char* path,
                                       const char* name,
                                       const char* ext);
void              vkk_uiFileList_filepath(vkk_uiFileList_t* self,
                                          char* filepath);
void              vkk_uiFileList_mkdir(vkk_uiWidget_t* widget,
                                       const char* text);
void              vkk_uiFileList_input(vkk_uiWidget_t* widget,
                                       const char* string);

#endif
