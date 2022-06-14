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

#ifndef vkui_window_H
#define vkui_window_H

#include "vkui.h"

#define VKUI_WINDOW_FLAG_TITLE        0x01
#define VKUI_WINDOW_FLAG_PAGE_DEFAULT 0x02
#define VKUI_WINDOW_FLAG_PAGE_SIDEBAR 0x04
#define VKUI_WINDOW_FLAG_PAGE_POPUP   0x08
#define VKUI_WINDOW_FLAG_LAYER0       0x10
#define VKUI_WINDOW_FLAG_LAYER1       0x20
#define VKUI_WINDOW_FLAG_FOOTER       0x40
#define VKUI_WINDOW_FLAG_TRANSPARENT  0x80

typedef struct vkui_windowInfo_s
{
	uint32_t        flags;
	const char*     label;
	vkui_widgetFn_t fn;
} vkui_windowInfo_t;

typedef struct vkui_window_s
{
	vkui_widget_t     base;
	vkui_bulletbox_t* title;
	vkui_listbox_t*   page;
	vkui_layer_t*     layer0;
	vkui_layer_t*     layer1;
	vkui_listbox_t*   footer;
	vkui_widget_t*    focus;
	int               transparent;
} vkui_window_t;

vkui_window_t*  vkui_window_new(vkui_screen_t* screen,
                                size_t wsize,
                                vkui_windowInfo_t* info);
void            vkui_window_delete(vkui_window_t** _self);
void            vkui_window_focus(vkui_window_t* self,
                                  vkui_widget_t* focus);
void            vkui_window_select(vkui_window_t* self,
                                   uint32_t index);
void            vkui_window_label(vkui_window_t* self,
                                  const char* fmt, ...);
vkui_listbox_t* vkui_window_page(vkui_window_t* self);
vkui_layer_t*   vkui_window_layer0(vkui_window_t* self);
vkui_layer_t*   vkui_window_layer1(vkui_window_t* self);
vkui_listbox_t* vkui_window_footer(vkui_window_t* self);

#endif
