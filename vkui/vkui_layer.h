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

#ifndef vkui_layer_H
#define vkui_layer_H

#include "../../libcc/cc_list.h"
#include "vkui.h"

typedef struct vkui_layer_s
{
	vkui_widget_t base;
	cc_list_t*    list;
} vkui_layer_t;

vkui_layer_t*  vkui_layer_new(vkui_screen_t* screen,
                              size_t wsize,
                              vkui_widgetLayout_t* layout,
                              cc_vec4f_t* color);
void           vkui_layer_delete(vkui_layer_t** _self);
void           vkui_layer_clear(vkui_layer_t* self);
int            vkui_layer_add(vkui_layer_t* self,
                              vkui_widget_t* widget);
cc_listIter_t* vkui_layer_head(vkui_layer_t* self);
vkui_widget_t* vkui_layer_remove(vkui_layer_t* self,
                                 cc_listIter_t** _iter);

#endif
