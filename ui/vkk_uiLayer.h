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

#ifndef vkk_uiLayer_H
#define vkk_uiLayer_H

#include "../../libcc/cc_list.h"

typedef struct vkk_uiLayer_s
{
	vkk_uiWidget_t base;
	cc_list_t*    list;
} vkk_uiLayer_t;

vkk_uiLayer_t*  vkk_uiLayer_new(vkk_uiScreen_t* screen,
                                size_t wsize,
                                vkk_uiWidgetLayout_t* layout,
                                cc_vec4f_t* color);
void            vkk_uiLayer_delete(vkk_uiLayer_t** _self);
void            vkk_uiLayer_clear(vkk_uiLayer_t* self);
int             vkk_uiLayer_add(vkk_uiLayer_t* self,
                                vkk_uiWidget_t* widget);
cc_listIter_t*  vkk_uiLayer_head(vkk_uiLayer_t* self);
vkk_uiWidget_t* vkk_uiLayer_remove(vkk_uiLayer_t* self,
                                   cc_listIter_t** _iter);

#endif
