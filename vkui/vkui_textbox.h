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

#ifndef vkui_textbox_H
#define vkui_textbox_H

#include "../../libcc/cc_list.h"

typedef struct vkui_textbox_s
{
	vkui_listbox_t base;

	// cached strings to reflow text
	cc_list_t* strings;
	int        dirty;
	float      last_w;
	float      last_h;

	// text properties
	vkui_textStyle_t text_style;
} vkui_textbox_t;

vkui_textbox_t* vkui_textbox_new(vkui_screen_t* screen,
                                 size_t wsize,
                                 vkui_widgetLayout_t* layout,
                                 vkui_widgetScroll_t* scroll,
                                 vkui_widgetFn_t* fn,
                                 vkui_textStyle_t* text_style);
vkui_textbox_t* vkui_textbox_newPageButton(vkui_screen_t* screen,
                                           vkui_widgetFn_t* fn);
vkui_textbox_t* vkui_textbox_newPageParagraph(vkui_screen_t* screen);
void            vkui_textbox_delete(vkui_textbox_t** _self);
void            vkui_textbox_clear(vkui_textbox_t* self);
void            vkui_textbox_printf(vkui_textbox_t* self,
                                    const char* fmt, ...);

#endif
