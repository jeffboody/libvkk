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

#ifndef vkk_uiRadiolist_H
#define vkk_uiRadiolist_H

typedef struct vkk_uiRadiolist_s
{
	vkk_uiListbox_t base;

	// bulletbox properties
	vkk_uiBulletboxStyle_t bulletbox_style;

	// radiolist value
	int* pvalue;
	int  value;
} vkk_uiRadiolist_t;

vkk_uiRadiolist_t* vkk_uiRadiolist_new(vkk_uiScreen_t* screen,
                                       size_t wsize,
                                       vkk_uiWidgetLayout_t* layout,
                                       vkk_uiWidgetScroll_t* scroll,
                                       vkk_uiBulletboxStyle_t* bulletbox_style,
                                       int* pvalue);
vkk_uiRadiolist_t* vkk_uiRadiolist_newPageItem(vkk_uiScreen_t* screen,
                                               int* pvalue);
void               vkk_uiRadiolist_delete(vkk_uiRadiolist_t** _self);
void               vkk_uiRadiolist_clear(vkk_uiRadiolist_t* self);
void               vkk_uiRadiolist_add(vkk_uiRadiolist_t* self,
                                       int value,
                                       const char* fmt, ...);
void               vkk_uiRadiolist_value(vkk_uiRadiolist_t* self,
                                         int value);

#endif
