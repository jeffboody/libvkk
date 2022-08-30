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

#ifndef vkk_uiRadioBox_H
#define vkk_uiRadioBox_H

typedef struct vkk_uiRadioBoxFn_s
{
	// priv and functions may be NULL
	void*                priv;
	vkk_uiWidgetValue_fn value_fn;
} vkk_uiRadioBoxFn_t;

typedef struct vkk_uiRadioBox_s
{
	vkk_uiBulletBox_t  base;
	vkk_uiRadioList_t* parent;
	int                value;
} vkk_uiRadioBox_t;


vkk_uiRadioBox_t* vkk_uiRadioBox_new(vkk_uiScreen_t* screen,
                                     size_t wsize,
                                     vkk_uiRadioBoxFn_t* rbfn,
                                     vkk_uiBulletBoxStyle_t* bulletbox_style,
                                     vkk_uiRadioList_t* parent,
                                     int value);
void             vkk_uiRadioBox_delete(vkk_uiRadioBox_t** _self);
void             vkk_uiRadioBox_label(vkk_uiRadioBox_t* self,
                                      const char* fmt, ...);

#endif
