/*
 * Copyright (c) 2022 Jeff Boody
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

#ifndef vkk_vgBuffer_H
#define vkk_vgBuffer_H

#include <stdint.h>

typedef struct vkk_vgBuffer_s
{
	uint32_t elem;
	uint32_t count;
	float*   data;
} vkk_vgBuffer_t;

vkk_vgBuffer_t* vkk_vgBuffer_new(uint32_t elem);
void            vkk_vgBuffer_delete(vkk_vgBuffer_t** _self);
void            vkk_vgBuffer_reset(vkk_vgBuffer_t* self);
int             vkk_vgBuffer_resize(vkk_vgBuffer_t* self,
                                    uint32_t count);
float*          vkk_vgBuffer_add2(vkk_vgBuffer_t* self,
                                  float x, float y);
float*          vkk_vgBuffer_add3(vkk_vgBuffer_t* self,
                                  float x, float y, float z);
float*          vkk_vgBuffer_add4(vkk_vgBuffer_t* self,
                                  float x, float y,
                                  float z, float w);
size_t          vkk_vgBuffer_size(vkk_vgBuffer_t* self);
uint32_t        vkk_vgBuffer_count(vkk_vgBuffer_t* self);
float*          vkk_vgBuffer_get(vkk_vgBuffer_t* self,
                                 uint32_t idx);

#endif
