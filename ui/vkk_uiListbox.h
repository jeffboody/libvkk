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

#ifndef vkk_uiListbox_H
#define vkk_uiListbox_H

#include "../../libcc/cc_list.h"

#define VKK_UI_LISTBOX_ORIENTATION_VERTICAL   0
#define VKK_UI_LISTBOX_ORIENTATION_HORIZONTAL 1

typedef struct vkk_uiListbox_s
{
	vkk_uiWidget_t base;
	cc_list_t*    list;

	int orientation;
} vkk_uiListbox_t;

vkk_uiListbox_t* vkk_uiListbox_new(vkk_uiScreen_t* screen,
                                   size_t wsize,
                                   vkk_uiWidgetLayout_t* layout,
                                   vkk_uiWidgetScroll_t* scroll,
                                   vkk_uiWidgetFn_t* fn,
                                   int orientation,
                                   cc_vec4f_t* color);
void             vkk_uiListbox_delete(vkk_uiListbox_t** _self);
void             vkk_uiListbox_clear(vkk_uiListbox_t* self);
int              vkk_uiListbox_add(vkk_uiListbox_t* self,
                                   vkk_uiWidget_t* widget);
int              vkk_uiListbox_addSorted(vkk_uiListbox_t* self,
                                         cc_listcmp_fn compare,
                                         vkk_uiWidget_t* widget);
cc_listIter_t*   vkk_uiListbox_head(vkk_uiListbox_t* self);
vkk_uiWidget_t*  vkk_uiListbox_remove(vkk_uiListbox_t* self,
                                      cc_listIter_t** _iter);

#endif
