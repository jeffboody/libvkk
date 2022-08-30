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

#ifndef vkk_uiCheckBox_H
#define vkk_uiCheckBox_H

typedef struct vkk_uiCheckBoxFn_s
{
	// priv and functions may be NULL
	void*                priv;
	vkk_uiWidgetValue_fn value_fn;
} vkk_uiCheckBoxFn_t;

typedef struct vkk_uiCheckBox_s
{
	vkk_uiBulletBox_t base;
	int               value;
} vkk_uiCheckBox_t;

vkk_uiCheckBox_t* vkk_uiCheckBox_new(vkk_uiScreen_t* screen,
                                     size_t wsize,
                                     vkk_uiCheckBoxFn_t* cbfn,
                                     vkk_uiBulletBoxStyle_t* bulletbox_style);
vkk_uiCheckBox_t* vkk_uiCheckBox_newPageItem(vkk_uiScreen_t* screen,
                                             vkk_uiCheckBoxFn_t* cbfn);
void             vkk_uiCheckBox_delete(vkk_uiCheckBox_t** _self);
void             vkk_uiCheckBox_label(vkk_uiCheckBox_t* self,
                                      const char* fmt, ...);
int              vkk_uiCheckBox_get(vkk_uiCheckBox_t* self);
void             vkk_uiCheckBox_set(vkk_uiCheckBox_t* self,
                                    int value);

#endif
