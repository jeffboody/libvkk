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

#ifndef vkk_uiActionBar_H
#define vkk_uiActionBar_H

typedef struct vkk_uiActionBarFn_s
{
	// priv and functions may be NULL
	void*                  priv;
	vkk_uiWidgetRefresh_fn refresh_fn;
} vkk_uiActionBarFn_t;

typedef struct vkk_uiActionBar_s
{
	vkk_uiListBox_t  base;
	vkk_uiListBox_t* actions;
	vkk_uiWidget_t*  space;
	int              forward;

	vkk_uiWidgetRefresh_fn refresh_fn;

	vkk_uiActionPopup_t* last_popup;
} vkk_uiActionBar_t;

vkk_uiActionBar_t*   vkk_uiActionBar_new(vkk_uiScreen_t* screen,
                                         size_t wsize,
                                         vkk_uiActionBarFn_t* abfn,
                                         int anchor,
                                         int orientation);
void                 vkk_uiActionBar_delete(vkk_uiActionBar_t** _self);
vkk_uiListBox_t*     vkk_uiActionBar_actions(vkk_uiActionBar_t* self);

#endif
