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

typedef struct vkk_uiActionBar_s
{
	vkk_uiListbox_t  base;
	vkk_uiListbox_t* actions;
	vkk_uiWidget_t*  space;
	vkk_uiSprite_t*  selected;
	int              forward;
} vkk_uiActionBar_t;

vkk_uiActionBar_t* vkk_uiActionBar_new(vkk_uiScreen_t* screen,
                                       size_t wsize,
                                       int anchor,
                                       int orientation,
                                       vkk_uiWidget_refreshFn refresh_fn);
void               vkk_uiActionBar_delete(vkk_uiActionBar_t** _self);
vkk_uiListbox_t*   vkk_uiActionBar_actions(vkk_uiActionBar_t* self);
int                vkk_uiActionBar_popup(vkk_uiActionBar_t* self,
                                         vkk_uiSprite_t* action,
                                         vkk_uiWindow_t* popup);
int                vkk_uiActionBar_clickPopupFn(vkk_uiWidget_t* widget,
                                               int state,
                                               float x, float y);

#endif
