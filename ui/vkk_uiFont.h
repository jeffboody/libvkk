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

#ifndef vkk_uiFont_H
#define vkk_uiFont_H

#include "../../libcc/math/cc_rect12f.h"
#include "../../texgz/texgz_tex.h"
#include "../vkk.h"

// define cursor as ASCII unit separator
#define VKK_UI_FONT_CURSOR 31

typedef struct vkk_uiFontcoords_s
{
	int x;
	int y;
	int w;
} vkk_uiFontcoords_t;

typedef struct vkk_uiFont_s
{
	texgz_tex_t* tex;

	// font attributes
	int   size;
	int   h;
	float aspect_ratio_avg;
	vkk_uiFontcoords_t coords[128];

	// shader data
	vkk_image_t* img21;
} vkk_uiFont_t;

vkk_uiFont_t* vkk_uiFont_new(vkk_uiScreen_t* screen,
                             const char* resource,
                             const char* texname,
                             const char* xmlname);
void          vkk_uiFont_delete(vkk_uiFont_t** _self);
void          vkk_uiFont_request(vkk_uiFont_t* self,
                                 char c,
                                 cc_rect2f_t* pc,
                                 cc_rect2f_t* tc,
                                 cc_rect2f_t* vc);
float         vkk_uiFont_aspectRatioAvg(vkk_uiFont_t* self);
int           vkk_uiFont_width(vkk_uiFont_t* self, char c);
int           vkk_uiFont_height(vkk_uiFont_t* self);
int           vkk_uiFont_measure(vkk_uiFont_t* self,
                                 const char* s);
texgz_tex_t*  vkk_uiFont_render(vkk_uiFont_t* self,
                                const char* s);

#endif
