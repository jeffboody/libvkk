/*
 * Copyright (c) 2020 Jeff Boody
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

#ifndef vkk_uiInputWindow_H
#define vkk_uiInputWindow_H

typedef struct vkk_uiInputWindowFn_s
{
	// priv and functions may be NULL
	void*                  priv;
	vkk_uiWidgetRefresh_fn refresh_fn;
	vkk_uiWidgetInput_fn   input_fn;
} vkk_uiInputWindowFn_t;

typedef struct vkk_uiInputWindow_s
{
	vkk_uiWindow_t       base;
	vkk_uiText_t*        text;
	vkk_uiBulletBox_t*   bulletbox_accept;
	vkk_uiBulletBox_t*   bulletbox_cancel;
	vkk_uiWidgetInput_fn input_fn;
} vkk_uiInputWindow_t;

vkk_uiInputWindow_t* vkk_uiInputWindow_new(vkk_uiScreen_t* screen,
                                           vkk_uiInputWindowFn_t* iwfn);
void                 vkk_uiInputWindow_delete(vkk_uiInputWindow_t** _self);
void                 vkk_uiInputWindow_label(vkk_uiInputWindow_t* self,
                                             const char* fmt, ...);
void                 vkk_uiInputWindow_labelAccept(vkk_uiInputWindow_t* self,
                                                   const char* fmt, ...);
void                 vkk_uiInputWindow_labelCancel(vkk_uiInputWindow_t* self,
                                                   const char* fmt, ...);
void                 vkk_uiInputWindow_labelText(vkk_uiInputWindow_t* self,
                                                 const char* fmt, ...);

#endif
