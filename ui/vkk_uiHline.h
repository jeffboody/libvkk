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

#ifndef vkk_uiHline_H
#define vkk_uiHline_H

#define VKK_UI_HLINE_SIZE_SMALL  0
#define VKK_UI_HLINE_SIZE_MEDIUM 1
#define VKK_UI_HLINE_SIZE_LARGE  2

typedef struct vkk_uiHline_s
{
	vkk_uiWidget_t  base;
	vkk_uiWidget_t* line;

	int size;
} vkk_uiHline_t;

vkk_uiHline_t* vkk_uiHline_new(vkk_uiScreen_t* screen,
                               size_t wsize,
                               int size,
                               cc_vec4f_t* color);
vkk_uiHline_t* vkk_uiHline_newPageItem(vkk_uiScreen_t* screen);
vkk_uiHline_t* vkk_uiHline_newInfoItem(vkk_uiScreen_t* screen);
void           vkk_uiHline_delete(vkk_uiHline_t** _self);

#endif
