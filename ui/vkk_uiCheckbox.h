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

#ifndef vkk_uiCheckbox_H
#define vkk_uiCheckbox_H

typedef struct vkk_uiCheckbox_s
{
	vkk_uiBulletbox_t base;

	int* pvalue;
} vkk_uiCheckbox_t;

vkk_uiCheckbox_t* vkk_uiCheckbox_new(vkk_uiScreen_t* screen,
                                    size_t wsize,
                                    vkk_uiBulletboxStyle_t* bulletbox_style,
                                    int* pvalue);
vkk_uiCheckbox_t* vkk_uiCheckbox_newPageItem(vkk_uiScreen_t* screen,
                                            int* pvalue);
void             vkk_uiCheckbox_delete(vkk_uiCheckbox_t** _self);
void             vkk_uiCheckbox_label(vkk_uiCheckbox_t* self,
                                      const char* fmt, ...);

#endif
