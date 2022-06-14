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

#ifndef vkui_bulletbox_H
#define vkui_bulletbox_H

#include "../../libcc/math/cc_vec4f.h"

typedef struct vkui_bulletboxStyle_s
{
	cc_vec4f_t       color_icon;
	vkui_textStyle_t text_style;
} vkui_bulletboxStyle_t;

typedef struct vkui_bulletbox_s
{
	vkui_widget_t  base;
	vkui_sprite_t* icon;
	vkui_text_t*   text;
} vkui_bulletbox_t;

vkui_bulletbox_t* vkui_bulletbox_new(vkui_screen_t* screen,
                                     size_t wsize,
                                     int anchor,
                                     vkui_widgetFn_t* fn,
                                     vkui_bulletboxStyle_t* bulletbox_style,
                                     const char** sprite_array);
vkui_bulletbox_t* vkui_bulletbox_newPageItem(vkui_screen_t* screen,
                                             vkui_widgetFn_t* fn,
                                             const char** sprite_array);
vkui_bulletbox_t* vkui_bulletbox_newFooterItem(vkui_screen_t* screen,
                                               vkui_widgetFn_t* fn,
                                               const char** sprite_array);
void              vkui_bulletbox_delete(vkui_bulletbox_t** _self);
void              vkui_bulletbox_select(vkui_bulletbox_t* self,
                                        uint32_t index);
void              vkui_bulletbox_colorIcon(vkui_bulletbox_t* self,
                                           cc_vec4f_t* color);
void              vkui_bulletbox_colorText(vkui_bulletbox_t* self,
                                           cc_vec4f_t* color);
void              vkui_bulletbox_label(vkui_bulletbox_t* self,
                                       const char* fmt, ...);

#endif
