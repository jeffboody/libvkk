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

#ifndef vkk_uiTextBox_H
#define vkk_uiTextBox_H

#include "../../libcc/cc_list.h"

typedef struct vkk_uiTextBoxFn_s
{
	// priv and functions may be NULL
	void*                priv;
	vkk_uiWidgetClick_fn click_fn;
} vkk_uiTextBoxFn_t;

typedef struct vkk_uiTextBox_s
{
	vkk_uiListBox_t base;

	// cached strings to reflow text
	cc_list_t* strings;
	int        dirty;
	float      last_w;
	float      last_h;

	// text properties
	vkk_uiTextStyle_t text_style;
} vkk_uiTextBox_t;

vkk_uiTextBox_t* vkk_uiTextBox_new(vkk_uiScreen_t* screen,
                                   size_t wsize,
                                   vkk_uiTextBoxFn_t* tbfn,
                                   vkk_uiWidgetLayout_t* layout,
                                   vkk_uiWidgetScroll_t* scroll,
                                   vkk_uiTextStyle_t* text_style);
vkk_uiTextBox_t* vkk_uiTextBox_newPageButton(vkk_uiScreen_t* screen,
                                             vkk_uiTextBoxFn_t* tbfn);
vkk_uiTextBox_t* vkk_uiTextBox_newPageParagraph(vkk_uiScreen_t* screen);
void             vkk_uiTextBox_delete(vkk_uiTextBox_t** _self);
void             vkk_uiTextBox_clear(vkk_uiTextBox_t* self);
void             vkk_uiTextBox_printf(vkk_uiTextBox_t* self,
                                      const char* fmt, ...);

#endif
