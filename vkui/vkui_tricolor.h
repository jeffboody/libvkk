/*
 * Copyright (c) 2019 Jeff Boody
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

#ifndef vkui_tricolor_H
#define vkui_tricolor_H

#include "../../libcc/math/cc_rect12f.h"
#include "../../libcc/math/cc_vec4f.h"
#include "../vkk.h"
#include "vkui.h"

typedef struct vkui_tricolor_s
{
	vkui_screen_t* screen;

	float a;
	float b;

	cc_rect1f_t rect;

	// shader data
	vkk_buffer_t*     vb_xyuv;
	vkk_buffer_t*     ub30_color0;
	vkk_buffer_t*     ub31_color1;
	vkk_buffer_t*     ub32_color2;
	vkk_buffer_t*     ub33_ab;
	vkk_uniformSet_t* us;
} vkui_tricolor_t;

vkui_tricolor_t* vkui_tricolor_new(vkui_screen_t* screen,
                                   cc_vec4f_t* color0,
                                   cc_vec4f_t* color1,
                                   cc_vec4f_t* color2);
void             vkui_tricolor_delete(vkui_tricolor_t** _self);
void             vkui_tricolor_ab(vkui_tricolor_t* self,
                                  float a, float b);
void             vkui_tricolor_rect(vkui_tricolor_t* self,
                                    cc_rect1f_t* rect);
void             vkui_tricolor_drawBuffer(vkui_tricolor_t* self,
                                          uint32_t vc,
                                          vkk_buffer_t* vb);
void             vkui_tricolor_drawRect(vkui_tricolor_t* self);

#endif
