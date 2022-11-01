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

#ifndef vkk_uiWindow_H
#define vkk_uiWindow_H

#define VKK_UI_WINDOW_FLAG_TITLE        0x0001
#define VKK_UI_WINDOW_FLAG_PAGE_DEFAULT 0x0002
#define VKK_UI_WINDOW_FLAG_PAGE_POPUP   0x0008
#define VKK_UI_WINDOW_FLAG_LAYER0       0x0010
#define VKK_UI_WINDOW_FLAG_LAYER1       0x0020
#define VKK_UI_WINDOW_FLAG_FOOTER       0x0040
#define VKK_UI_WINDOW_FLAG_TRANSPARENT  0x0080
#define VKK_UI_WINDOW_FLAG_FULLSCREEN   0x0100

typedef struct vkk_uiWindowFn_s
{
	// priv and functions may be NULL
	void*                  priv;
	vkk_uiWidgetRefresh_fn refresh_fn;
} vkk_uiWindowFn_t;

typedef struct vkk_uiWindow_s
{
	vkk_uiWidget_t         base;
	vkk_uiBulletBox_t*     title;
	vkk_uiListBox_t*       page;
	vkk_uiLayer_t*         layer0;
	vkk_uiLayer_t*         layer1;
	vkk_uiListBox_t*       footer;
	vkk_uiWidget_t*        focus;
	int                    transparent;
	vkk_uiWidgetRefresh_fn refresh_fn;

	uint32_t flags;
} vkk_uiWindow_t;

vkk_uiWindow_t*  vkk_uiWindow_new(vkk_uiScreen_t* screen,
                                  size_t wsize,
                                  vkk_uiWindowFn_t* wfn,
                                  uint32_t flags);
void             vkk_uiWindow_delete(vkk_uiWindow_t** _self);
void             vkk_uiWindow_fullscreen(vkk_uiWindow_t* self,
                                         int fullscreen);
void             vkk_uiWindow_focus(vkk_uiWindow_t* self,
                                    vkk_uiWidget_t* focus);
void             vkk_uiWindow_select(vkk_uiWindow_t* self,
                                     uint32_t index);
void             vkk_uiWindow_label(vkk_uiWindow_t* self,
                                    const char* fmt, ...);
vkk_uiListBox_t* vkk_uiWindow_page(vkk_uiWindow_t* self);
vkk_uiLayer_t*   vkk_uiWindow_layer0(vkk_uiWindow_t* self);
vkk_uiLayer_t*   vkk_uiWindow_layer1(vkk_uiWindow_t* self);
vkk_uiListBox_t* vkk_uiWindow_footer(vkk_uiWindow_t* self);

#endif
