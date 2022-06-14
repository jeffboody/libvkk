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

#ifndef vkui_infoPanel_H
#define vkui_infoPanel_H

typedef struct vkui_infoPanel_s
{
	vkui_listbox_t base;
} vkui_infoPanel_t;

vkui_infoPanel_t* vkui_infoPanel_new(vkui_screen_t* screen,
                                     size_t wsize,
                                     vkui_widgetFn_t* widget_fn);
void              vkui_infoPanel_delete(vkui_infoPanel_t** _self);
void              vkui_infoPanel_add(vkui_infoPanel_t* self,
                                     vkui_widget_t* widget);
void              vkui_infoPanel_clear(vkui_infoPanel_t* self);

#endif
