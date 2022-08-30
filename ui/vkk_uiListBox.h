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

#ifndef vkk_uiListBox_H
#define vkk_uiListBox_H

#include "../../libcc/cc_list.h"

#define VKK_UI_LISTBOX_ORIENTATION_VERTICAL   0
#define VKK_UI_LISTBOX_ORIENTATION_HORIZONTAL 1

typedef struct vkk_uiListBoxFn_s
{
	// priv and functions may be NULL
	void*                  priv;
	vkk_uiWidgetRefresh_fn refresh_fn;
} vkk_uiListBoxFn_t;

typedef struct vkk_uiListBox_s
{
	vkk_uiWidget_t         base;
	cc_list_t*             list;
	int                    orientation;
	vkk_uiWidgetRefresh_fn refresh_fn;
} vkk_uiListBox_t;

vkk_uiListBox_t* vkk_uiListBox_new(vkk_uiScreen_t* screen,
                                   size_t wsize,
                                   vkk_uiListBoxFn_t* lbfn,
                                   vkk_uiWidgetLayout_t* layout,
                                   vkk_uiWidgetScroll_t* scroll,
                                   int orientation,
                                   cc_vec4f_t* color);
void             vkk_uiListBox_delete(vkk_uiListBox_t** _self);
void             vkk_uiListBox_clear(vkk_uiListBox_t* self);
int              vkk_uiListBox_add(vkk_uiListBox_t* self,
                                   vkk_uiWidget_t* widget);
int              vkk_uiListBox_addSorted(vkk_uiListBox_t* self,
                                         cc_listcmp_fn compare,
                                         vkk_uiWidget_t* widget);
cc_listIter_t*   vkk_uiListBox_head(vkk_uiListBox_t* self);
vkk_uiWidget_t*  vkk_uiListBox_remove(vkk_uiListBox_t* self,
                                      cc_listIter_t** _iter);

#endif
