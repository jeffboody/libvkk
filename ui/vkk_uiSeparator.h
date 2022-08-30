/*
 * Copyright (c) 2018 Jeff Boody
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

#ifndef vkk_uiSeparator_H
#define vkk_uiSeparator_H

#define VKK_UI_SEPARATOR_TYPE_HORIZONTAL 0
#define VKK_UI_SEPARATOR_TYPE_VERTICAL   1

typedef struct vkk_uiSeparator_s
{
	vkk_uiWidget_t  base;
	vkk_uiWidget_t* line;
} vkk_uiSeparator_t;

vkk_uiSeparator_t* vkk_uiSeparator_new(vkk_uiScreen_t* screen,
                                       size_t wsize,
                                       int type,
                                       cc_vec4f_t* color);
vkk_uiSeparator_t* vkk_uiSeparator_newPageItem(vkk_uiScreen_t* screen);
vkk_uiSeparator_t* vkk_uiSeparator_newInfoItem(vkk_uiScreen_t* screen);
void               vkk_uiSeparator_delete(vkk_uiSeparator_t** _self);

#endif
