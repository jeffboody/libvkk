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

#ifndef vkui_font_H
#define vkui_font_H

#include "../../libcc/math/cc_rect12f.h"
#include "../../texgz/texgz_tex.h"
#include "../vkk.h"
#include "vkui.h"

// define cursor as ASCII unit separator
#define VKUI_FONT_CURSOR 31

typedef struct vkui_fontcoords_s
{
	int x;
	int y;
	int w;
} vkui_fontcoords_t;

typedef struct vkui_font_s
{
	texgz_tex_t* tex;

	// font attributes
	int   size;
	int   h;
	float aspect_ratio_avg;
	vkui_fontcoords_t coords[128];

	// shader data
	vkk_image_t* img21;
} vkui_font_t;

vkui_font_t* vkui_font_new(vkui_screen_t* screen,
                           const char* resource,
                           const char* texname,
                           const char* xmlname);
void         vkui_font_delete(vkui_font_t** _self);
void         vkui_font_request(vkui_font_t* self,
                               char c,
                               cc_rect2f_t* pc,
                               cc_rect2f_t* tc,
                               cc_rect2f_t* vc);
float        vkui_font_aspectRatioAvg(vkui_font_t* self);
int          vkui_font_width(vkui_font_t* self, char c);
int          vkui_font_height(vkui_font_t* self);
int          vkui_font_measure(vkui_font_t* self,
                               const char* s);
texgz_tex_t* vkui_font_render(vkui_font_t* self,
                              const char* s);

#endif
