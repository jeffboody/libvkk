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

#ifndef vkui_sprite_H
#define vkui_sprite_H

#include "../vkk.h"
#include "vkui_widget.h"
#include "vkui.h"

typedef struct vkui_spriteImage_s
{
	vkk_image_t*      image;
	vkk_uniformSet_t* us_image;
} vkui_spriteImage_t;

typedef struct vkui_sprite_s
{
	vkui_widget_t widget;

	// properties
	uint32_t count;
	uint32_t index;
	float    theta;

	// shader data
	vkui_spriteImage_t** image_array;
	vkk_buffer_t*        vb_color_xyuv;
	vkk_buffer_t*        ub_mvp;
	vkk_buffer_t*        ub_color;
	vkk_buffer_t*        ub_multiply;
	vkk_uniformSet_t*    us_mvp;
	vkk_uniformSet_t*    us_color;
	vkk_uniformSet_t*    us_multiply;
} vkui_sprite_t;

#endif
