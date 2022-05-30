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

#ifndef vkui_radiobox_H
#define vkui_radiobox_H

#include "vkui_bulletbox.h"
#include "vkui.h"

typedef struct vkui_radiobox_s
{
	vkui_bulletbox_t  bullet;
	int               value;
	vkui_radiolist_t* parent;
} vkui_radiobox_t;


vkui_radiobox_t* vkui_radiobox_new(vkui_screen_t* screen,
                                   size_t wsize,
                                   vkui_bulletboxStyle_t* bulletbox_style,
                                   int value,
                                   vkui_radiolist_t* parent);
void             vkui_radiobox_delete(vkui_radiobox_t** _self);
void             vkui_radiobox_label(vkui_radiobox_t* self,
                                     const char* fmt, ...);
void             vkui_radiobox_refresh(vkui_radiobox_t* self);

#endif
