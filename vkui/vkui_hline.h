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

#ifndef vkui_hline_H
#define vkui_hline_H

#define VKUI_HLINE_SIZE_SMALL  0
#define VKUI_HLINE_SIZE_MEDIUM 1
#define VKUI_HLINE_SIZE_LARGE  2

typedef struct vkui_hline_s
{
	vkui_widget_t  base;
	vkui_widget_t* line;

	int size;
} vkui_hline_t;

vkui_hline_t* vkui_hline_new(vkui_screen_t* screen,
                             size_t wsize,
                             int size,
                             cc_vec4f_t* color);
vkui_hline_t* vkui_hline_newPageItem(vkui_screen_t* screen);
vkui_hline_t* vkui_hline_newInfoItem(vkui_screen_t* screen);
void          vkui_hline_delete(vkui_hline_t** _self);

#endif
