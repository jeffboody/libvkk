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

#ifndef vkk_uiBulletBox_H
#define vkk_uiBulletBox_H

#include "../../libcc/math/cc_vec4f.h"

typedef struct vkk_uiBulletBoxFn_s
{
	// priv and functions may be NULL
	void*                priv;
	vkk_uiWidgetClick_fn click_fn;
} vkk_uiBulletBoxFn_t;

typedef struct vkk_uiBulletBoxStyle_s
{
	cc_vec4f_t        color_icon;
	vkk_uiTextStyle_t text_style;
} vkk_uiBulletBoxStyle_t;

typedef struct vkk_uiBulletBox_s
{
	vkk_uiWidget_t  base;
	vkk_uiSprite_t* icon;
	vkk_uiText_t*   text;
} vkk_uiBulletBox_t;

vkk_uiBulletBox_t* vkk_uiBulletBox_new(vkk_uiScreen_t* screen,
                                       size_t wsize,
                                       vkk_uiBulletBoxFn_t* bbfn,
                                       int anchor,
                                       vkk_uiBulletBoxStyle_t* bulletbox_style,
                                       const char** sprite_array);
vkk_uiBulletBox_t* vkk_uiBulletBox_newPageItem(vkk_uiScreen_t* screen,
                                               vkk_uiBulletBoxFn_t* bbfn,
                                               const char** sprite_array);
vkk_uiBulletBox_t* vkk_uiBulletBox_newInfoItem(vkk_uiScreen_t* screen,
                                               const char** sprite_array);
vkk_uiBulletBox_t* vkk_uiBulletBox_newFooterItem(vkk_uiScreen_t* screen,
                                                 vkk_uiBulletBoxFn_t* bbfn,
                                                 const char** sprite_array);
void               vkk_uiBulletBox_delete(vkk_uiBulletBox_t** _self);
void               vkk_uiBulletBox_select(vkk_uiBulletBox_t* self,
                                          uint32_t index);
void               vkk_uiBulletBox_colorIcon(vkk_uiBulletBox_t* self,
                                             cc_vec4f_t* color);
void               vkk_uiBulletBox_colorText(vkk_uiBulletBox_t* self,
                                             cc_vec4f_t* color);
void               vkk_uiBulletBox_label(vkk_uiBulletBox_t* self,
                                         const char* fmt, ...);

#endif
