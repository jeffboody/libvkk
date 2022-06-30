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

#ifndef vkk_uiTextEntry_H
#define vkk_uiTextEntry_H

typedef void (*vkk_uiTextEntry_acceptFn)(void* priv,
                                         const char* text);

typedef struct vkk_uiTextEntry_s
{
	vkk_uiWindow_t     window;
	vkk_uiText_t*      text;
	vkk_uiBulletbox_t* bulletbox_accept;
	vkk_uiBulletbox_t* bulletbox_cancel;

	void* priv;
	vkk_uiTextEntry_acceptFn accept_fn;
} vkk_uiTextEntry_t;

vkk_uiTextEntry_t* vkk_uiTextEntry_new(vkk_uiScreen_t* screen,
                                       void* priv,
                                       const char* label,
                                       vkk_uiTextEntry_acceptFn accept_fn);
void               vkk_uiTextEntry_delete(vkk_uiTextEntry_t** _self);
void               vkk_uiTextEntry_label(vkk_uiTextEntry_t* self,
                                         const char* fmt, ...);
void               vkk_uiTextEntry_labelAccept(vkk_uiTextEntry_t* self,
                                               const char* fmt, ...);
void               vkk_uiTextEntry_labelCancel(vkk_uiTextEntry_t* self,
                                               const char* fmt, ...);
void               vkk_uiTextEntry_labelText(vkk_uiTextEntry_t* self,
                                             const char* fmt, ...);

#endif
