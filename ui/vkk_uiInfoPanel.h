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

#ifndef vkk_uiInfoPanel_H
#define vkk_uiInfoPanel_H

typedef struct vkk_uiInfoPanelFn_s
{
	// priv and functions may be NULL
	void*                  priv;
	vkk_uiWidgetRefresh_fn refresh_fn;
} vkk_uiInfoPanelFn_t;

typedef struct vkk_uiInfoPanel_s
{
	vkk_uiListBox_t base;
} vkk_uiInfoPanel_t;

vkk_uiInfoPanel_t* vkk_uiInfoPanel_new(vkk_uiScreen_t* screen,
                                       size_t wsize,
                                       vkk_uiInfoPanelFn_t* ipfn);
void               vkk_uiInfoPanel_delete(vkk_uiInfoPanel_t** _self);
void               vkk_uiInfoPanel_add(vkk_uiInfoPanel_t* self,
                                       vkk_uiWidget_t* widget);
void               vkk_uiInfoPanel_clear(vkk_uiInfoPanel_t* self);

#endif
