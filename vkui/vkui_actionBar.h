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

#ifndef vkui_actionBar_H
#define vkui_actionBar_H

typedef struct vkui_actionBar_s
{
	vkui_listbox_t  base;
	vkui_listbox_t* actions;
	vkui_widget_t*  space;
	vkui_sprite_t*  selected;
	int             forward;
} vkui_actionBar_t;

vkui_actionBar_t* vkui_actionBar_new(vkui_screen_t* screen,
                                     size_t wsize,
                                     int anchor,
                                     int orientation,
                                     vkui_widget_refreshFn refresh_fn);
void              vkui_actionBar_delete(vkui_actionBar_t** _self);
vkui_listbox_t*   vkui_actionBar_actions(vkui_actionBar_t* self);
int               vkui_actionBar_popup(vkui_actionBar_t* self,
                                       vkui_sprite_t* action,
                                       vkui_window_t* popup);
int               vkui_actionBar_clickPopupFn(vkui_widget_t* widget,
                                              int state,
                                              float x, float y);

#endif
