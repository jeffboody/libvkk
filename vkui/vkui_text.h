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

#ifndef vkui_text_H
#define vkui_text_H

#include "vkui_widget.h"
#include "vkui.h"

typedef struct vkui_text_s
{
	vkui_widget_t widget;

	// text properties
	vkui_text_enterFn enter_fn;
	vkui_textStyle_t  style;

	// string data
	size_t size;
	char*  string;
	float* xyuv;

	// shader data
	vkk_buffer_t*     vb_xyuv;
	vkk_buffer_t*     ub_mvp;
	vkk_uniformSet_t* us_mvp;
	vkk_buffer_t*     ub_multiply;
	vkk_uniformSet_t* us_multiplyImage;
	vkk_buffer_t*     ub_color;
	vkk_uniformSet_t* us_color;
} vkui_text_t;

int vkui_text_width(vkui_text_t* self, int cursor);
int vkui_text_height(vkui_text_t* self);

#endif
