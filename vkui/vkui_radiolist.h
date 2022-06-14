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

#ifndef vkui_radiolist_H
#define vkui_radiolist_H

typedef struct vkui_radiolist_s
{
	vkui_listbox_t base;

	// bulletbox properties
	vkui_bulletboxStyle_t bulletbox_style;

	// radiolist value
	int* pvalue;
	int  value;
} vkui_radiolist_t;

vkui_radiolist_t* vkui_radiolist_new(vkui_screen_t* screen,
                                     size_t wsize,
                                     vkui_widgetLayout_t* layout,
                                     vkui_widgetScroll_t* scroll,
                                     vkui_bulletboxStyle_t* bulletbox_style,
                                     int* pvalue);
vkui_radiolist_t* vkui_radiolist_newPageItem(vkui_screen_t* screen,
                                             int* pvalue);
void              vkui_radiolist_delete(vkui_radiolist_t** _self);
void              vkui_radiolist_clear(vkui_radiolist_t* self);
void              vkui_radiolist_add(vkui_radiolist_t* self,
                                     int value,
                                     const char* fmt, ...);
void              vkui_radiolist_value(vkui_radiolist_t* self,
                                       int value);

#endif
