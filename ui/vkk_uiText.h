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

#ifndef vkk_uiText_H
#define vkk_uiText_H

#define VKK_UI_TEXT_FONTTYPE_REGULAR 0
#define VKK_UI_TEXT_FONTTYPE_BOLD    1
#define VKK_UI_TEXT_FONTTYPE_MEDIUM  2

#define VKK_UI_TEXT_SIZE_XSMALL -1
#define VKK_UI_TEXT_SIZE_SMALL  0
#define VKK_UI_TEXT_SIZE_MEDIUM 1
#define VKK_UI_TEXT_SIZE_LARGE  2

#define VKK_UI_TEXT_SPACING_NONE   0x00
#define VKK_UI_TEXT_SPACING_SMALL  0x10
#define VKK_UI_TEXT_SPACING_MEDIUM 0x20
#define VKK_UI_TEXT_SPACING_LARGE  0x40
#define VKK_UI_TEXT_SPACING_XLARGE 0x80

typedef struct vkk_uiTextFn_s
{
	// priv and functions may be NULL
	void*                priv;
	vkk_uiWidgetClick_fn click_fn;
	vkk_uiWidgetInput_fn input_fn;
} vkk_uiTextFn_t;

typedef struct vkk_uiTextLayout_s
{
	int   border;
	int   anchor;
	int   wrapx;
	float stretchx;
} vkk_uiTextLayout_t;

typedef struct vkk_uiTextStyle_s
{
	int        font_type;
	int        size;
	int        spacing;
	cc_vec4f_t color;
} vkk_uiTextStyle_t;

typedef struct vkk_uiText_s
{
	vkk_uiWidget_t       base;
	vkk_uiTextStyle_t    style;
	vkk_uiWidgetClick_fn click_fn;

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
} vkk_uiText_t;

vkk_uiText_t* vkk_uiText_new(vkk_uiScreen_t* screen,
                             size_t wsize,
                             vkk_uiTextFn_t* tfn,
                             vkk_uiTextLayout_t* text_layout,
                             vkk_uiTextStyle_t* text_style,
                             cc_vec4f_t* color_fill);
vkk_uiText_t* vkk_uiText_newPageHeading(vkk_uiScreen_t* screen);
vkk_uiText_t* vkk_uiText_newPageItem(vkk_uiScreen_t* screen);
vkk_uiText_t* vkk_uiText_newPageSubheading(vkk_uiScreen_t* screen);
vkk_uiText_t* vkk_uiText_newPageTextInput(vkk_uiScreen_t* screen,
                                          vkk_uiTextFn_t* tfn);
vkk_uiText_t* vkk_uiText_newInfoHeading(vkk_uiScreen_t* screen);
vkk_uiText_t* vkk_uiText_newInfoItem(vkk_uiScreen_t* screen);
void          vkk_uiText_delete(vkk_uiText_t** _self);
void          vkk_uiText_color(vkk_uiText_t* self,
                               cc_vec4f_t* color);
int           vkk_uiText_label(vkk_uiText_t* self,
                               const char* fmt, ...);
int           vkk_uiText_width(vkk_uiText_t* self, int cursor);
int           vkk_uiText_height(vkk_uiText_t* self);
const char*   vkk_uiText_string(vkk_uiText_t* self);

#endif
