/*
 * Copyright (c) 2022 Jeff Boody
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

#ifndef vkk_uiStatusBar_H
#define vkk_uiStatusBar_H

typedef struct vkk_uiStatusBarFn_s
{
	// priv and functions may be NULL
	void*                  priv;
	vkk_uiWidgetRefresh_fn refresh_fn;
} vkk_uiStatusBarFn_t;

typedef struct vkk_uiStatusBar_s
{
	vkk_uiListBox_t base;
} vkk_uiStatusBar_t;

vkk_uiStatusBar_t* vkk_uiStatusBar_new(vkk_uiScreen_t* screen,
                                       size_t wsize,
                                       vkk_uiStatusBarFn_t* sbfn);
void               vkk_uiStatusBar_delete(vkk_uiStatusBar_t** _self);
void               vkk_uiStatusBar_add(vkk_uiStatusBar_t* self,
                                       vkk_uiWidget_t* widget);
void               vkk_uiStatusBar_clear(vkk_uiStatusBar_t* self);

#endif
