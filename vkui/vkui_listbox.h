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

#ifndef vkui_listbox_H
#define vkui_listbox_H

#include "../../libcc/cc_list.h"

#define VKUI_LISTBOX_ORIENTATION_VERTICAL   0
#define VKUI_LISTBOX_ORIENTATION_HORIZONTAL 1

typedef struct vkui_listbox_s
{
	vkui_widget_t base;
	cc_list_t*    list;

	int orientation;
} vkui_listbox_t;

vkui_listbox_t* vkui_listbox_new(vkui_screen_t* screen,
                                 size_t wsize,
                                 vkui_widgetLayout_t* layout,
                                 vkui_widgetScroll_t* scroll,
                                 vkui_widgetFn_t* fn,
                                 int orientation,
                                 cc_vec4f_t* color);
void            vkui_listbox_delete(vkui_listbox_t** _self);
void            vkui_listbox_clear(vkui_listbox_t* self);
int             vkui_listbox_add(vkui_listbox_t* self,
                                 vkui_widget_t* widget);
int             vkui_listbox_addSorted(vkui_listbox_t* self,
                                       cc_listcmp_fn compare,
                                       vkui_widget_t* widget);
cc_listIter_t*  vkui_listbox_head(vkui_listbox_t* self);
vkui_widget_t*  vkui_listbox_remove(vkui_listbox_t* self,
                                    cc_listIter_t** _iter);

#endif
