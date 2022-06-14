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

#define VKUI_TEXT_FONTTYPE_REGULAR 0
#define VKUI_TEXT_FONTTYPE_BOLD    1
#define VKUI_TEXT_FONTTYPE_MEDIUM  2

#define VKUI_TEXT_SIZE_XSMALL -1
#define VKUI_TEXT_SIZE_SMALL  0
#define VKUI_TEXT_SIZE_MEDIUM 1
#define VKUI_TEXT_SIZE_LARGE  2

#define VKUI_TEXT_SPACING_NONE   0x00
#define VKUI_TEXT_SPACING_SMALL  0x10
#define VKUI_TEXT_SPACING_MEDIUM 0x20
#define VKUI_TEXT_SPACING_LARGE  0x40

typedef void (*vkui_text_enterFn)(vkui_widget_t* widget,
                                  const char* string);

typedef struct vkui_textLayout_s
{
	int   border;
	int   anchor;
	int   wrapx;
	float stretchx;
} vkui_textLayout_t;

typedef struct vkui_textFn_s
{
	// functions may be NULL

	vkui_widgetFn_t   fn;
	vkui_text_enterFn enter_fn;
} vkui_textFn_t;

typedef struct vkui_textStyle_s
{
	int        font_type;
	int        size;
	int        spacing;
	cc_vec4f_t color;
} vkui_textStyle_t;

typedef struct vkui_text_s
{
	vkui_widget_t base;

	// text properties
	vkui_text_enterFn enter_fn;
	vkui_textStyle_t  style;

	// string data
	size_t size;
	char*  string;
	float* xyuv;

	// shader data
	vkk_buffer_t*     vb_xyuv;
	vkk_buffer_t*     ub00_mvp;
	vkk_uniformSet_t* us0_mvp;
	vkk_buffer_t*     ub20_multiply;
	vkk_uniformSet_t* us2_multiplyImage;
	vkk_buffer_t*     ub10_color;
	vkk_uniformSet_t* us1_color;
} vkui_text_t;

vkui_text_t* vkui_text_new(vkui_screen_t* screen,
                           size_t wsize,
                           vkui_textLayout_t* text_layout,
                           vkui_textStyle_t* text_style,
                           vkui_textFn_t* text_fn,
                           cc_vec4f_t* color_fill);
vkui_text_t* vkui_text_newPageHeading(vkui_screen_t* screen);
vkui_text_t* vkui_text_newPageTextEntry(vkui_screen_t* screen,
                                        void* priv,
                                        vkui_text_enterFn enter_fn);
vkui_text_t* vkui_text_newInfoHeading(vkui_screen_t* screen);
vkui_text_t* vkui_text_newInfoItem(vkui_screen_t* screen);
void         vkui_text_delete(vkui_text_t** _self);
void         vkui_text_color(vkui_text_t* self,
                             cc_vec4f_t* color);
int          vkui_text_label(vkui_text_t* self,
                             const char* fmt, ...);
int          vkui_text_width(vkui_text_t* self, int cursor);
int          vkui_text_height(vkui_text_t* self);

#endif
