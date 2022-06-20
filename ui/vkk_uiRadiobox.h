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

#ifndef vkk_uiRadiobox_H
#define vkk_uiRadiobox_H

typedef struct vkk_uiRadiobox_s
{
	vkk_uiBulletbox_t base;

	int                value;
	vkk_uiRadiolist_t* parent;
} vkk_uiRadiobox_t;


vkk_uiRadiobox_t* vkk_uiRadiobox_new(vkk_uiScreen_t* screen,
                                     size_t wsize,
                                     vkk_uiBulletboxStyle_t* bulletbox_style,
                                     int value,
                                     vkk_uiRadiolist_t* parent);
void             vkk_uiRadiobox_delete(vkk_uiRadiobox_t** _self);
void             vkk_uiRadiobox_label(vkk_uiRadiobox_t* self,
                                      const char* fmt, ...);
void             vkk_uiRadiobox_refresh(vkk_uiRadiobox_t* self);

#endif
