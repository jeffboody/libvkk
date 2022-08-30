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

#include <stdlib.h>

#define LOG_TAG "vkk"
#include "../../libcc/cc_log.h"
#include "../../libcc/cc_memory.h"
#include "../vkk_ui.h"

/***********************************************************
* public                                                   *
***********************************************************/

vkk_uiActionPopup_t*
vkk_uiActionPopup_new(vkk_uiScreen_t* screen,
                      size_t wsize,
                      vkk_uiActionPopupFn_t* apfn,
                      vkk_uiActionBar_t* parent)
{
	ASSERT(screen);
	ASSERT(apfn);
	ASSERT(parent);

	if(wsize == 0)
	{
		wsize = sizeof(vkk_uiActionPopup_t);
	}

	vkk_uiWindowFn_t wfn =
	{
		.priv       = apfn->priv,
		.refresh_fn = apfn->refresh_fn,
	};

	uint32_t flags = VKK_UI_WINDOW_FLAG_TITLE |
	                 VKK_UI_WINDOW_FLAG_PAGE_POPUP;

	vkk_uiActionPopup_t* self;
	self = (vkk_uiActionPopup_t*)
	       vkk_uiWindow_new(screen, wsize,
	                        &wfn, flags);
	if(self == NULL)
	{
		return NULL;
	}

	return self;
}

void vkk_uiActionPopup_delete(vkk_uiActionPopup_t** _self)
{
	ASSERT(_self);

	vkk_uiActionPopup_t* self = *_self;
	if(self)
	{
		vkk_uiWindow_delete((vkk_uiWindow_t**) _self);
	}
}
