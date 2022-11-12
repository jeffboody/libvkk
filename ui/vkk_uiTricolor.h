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

#ifndef vkk_uiTricolor_H
#define vkk_uiTricolor_H

#include "../../libcc/math/cc_rect12f.h"
#include "../../libcc/math/cc_vec4f.h"

typedef struct vkk_uiTricolor_s
{
	vkk_uiScreen_t* screen;

	float alpha0;
	float alpha1;
	float alpha2;

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
} vkk_uiTricolor_t;

vkk_uiTricolor_t* vkk_uiTricolor_new(vkk_uiScreen_t* screen,
                                     cc_vec4f_t* color0,
                                     cc_vec4f_t* color1,
                                     cc_vec4f_t* color2);
void              vkk_uiTricolor_delete(vkk_uiTricolor_t** _self);
void              vkk_uiTricolor_ab(vkk_uiTricolor_t* self,
                                    float a, float b);
void              vkk_uiTricolor_rect(vkk_uiTricolor_t* self,
                                      cc_rect1f_t* rect);
void              vkk_uiTricolor_drawBuffer0(vkk_uiTricolor_t* self,
                                             uint32_t vc,
                                             vkk_buffer_t* vb);
void              vkk_uiTricolor_drawBuffer1(vkk_uiTricolor_t* self,
                                             uint32_t vc,
                                             vkk_buffer_t* vb);
void              vkk_uiTricolor_drawRect(vkk_uiTricolor_t* self);

#endif
