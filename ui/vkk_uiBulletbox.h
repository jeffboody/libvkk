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

#ifndef vkk_uiBulletbox_H
#define vkk_uiBulletbox_H

#include "../../libcc/math/cc_vec4f.h"

typedef struct vkk_uiBulletboxStyle_s
{
	cc_vec4f_t        color_icon;
	vkk_uiTextStyle_t text_style;
} vkk_uiBulletboxStyle_t;

typedef struct vkk_uiBulletbox_s
{
	vkk_uiWidget_t  base;
	vkk_uiSprite_t* icon;
	vkk_uiText_t*   text;
} vkk_uiBulletbox_t;

vkk_uiBulletbox_t* vkk_uiBulletbox_new(vkk_uiScreen_t* screen,
                                       size_t wsize,
                                       int anchor,
                                       vkk_uiWidgetFn_t* fn,
                                       vkk_uiBulletboxStyle_t* bulletbox_style,
                                       const char** sprite_array);
vkk_uiBulletbox_t* vkk_uiBulletbox_newPageItem(vkk_uiScreen_t* screen,
                                               vkk_uiWidgetFn_t* fn,
                                               const char** sprite_array);
vkk_uiBulletbox_t* vkk_uiBulletbox_newFooterItem(vkk_uiScreen_t* screen,
                                                 vkk_uiWidgetFn_t* fn,
                                                 const char** sprite_array);
void               vkk_uiBulletbox_delete(vkk_uiBulletbox_t** _self);
void               vkk_uiBulletbox_select(vkk_uiBulletbox_t* self,
                                          uint32_t index);
void               vkk_uiBulletbox_colorIcon(vkk_uiBulletbox_t* self,
                                             cc_vec4f_t* color);
void               vkk_uiBulletbox_colorText(vkk_uiBulletbox_t* self,
                                             cc_vec4f_t* color);
void               vkk_uiBulletbox_label(vkk_uiBulletbox_t* self,
                                         const char* fmt, ...);

#endif
