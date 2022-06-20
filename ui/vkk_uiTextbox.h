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

#ifndef vkk_uiTextbox_H
#define vkk_uiTextbox_H

#include "../../libcc/cc_list.h"

typedef struct vkk_uiTextbox_s
{
	vkk_uiListbox_t base;

	// cached strings to reflow text
	cc_list_t* strings;
	int        dirty;
	float      last_w;
	float      last_h;

	// text properties
	vkk_uiTextStyle_t text_style;
} vkk_uiTextbox_t;

vkk_uiTextbox_t* vkk_uiTextbox_new(vkk_uiScreen_t* screen,
                                   size_t wsize,
                                   vkk_uiWidgetLayout_t* layout,
                                   vkk_uiWidgetScroll_t* scroll,
                                   vkk_uiWidgetFn_t* fn,
                                   vkk_uiTextStyle_t* text_style);
vkk_uiTextbox_t* vkk_uiTextbox_newPageButton(vkk_uiScreen_t* screen,
                                             vkk_uiWidgetFn_t* fn);
vkk_uiTextbox_t* vkk_uiTextbox_newPageParagraph(vkk_uiScreen_t* screen);
void             vkk_uiTextbox_delete(vkk_uiTextbox_t** _self);
void             vkk_uiTextbox_clear(vkk_uiTextbox_t* self);
void             vkk_uiTextbox_printf(vkk_uiTextbox_t* self,
                                      const char* fmt, ...);

#endif
