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
#include "../vkk_ui.h"

/***********************************************************
* private                                                  *
***********************************************************/

static void
vkk_uiActionButton_click(vkk_uiWidget_t* widget,
                         float x, float y)
{
	ASSERT(widget);

	vkk_uiActionButton_t* self = (vkk_uiActionButton_t*) widget;

	vkk_uiActionPopup_t** _popup = self->_popup;
	if(_popup)
	{
		vkk_uiScreen_popupSet(widget->screen,
		                      self->parent,
		                      *_popup);
	}
	else
	{
		vkk_uiScreen_popupSet(widget->screen, NULL, NULL);
	}

	vkk_uiWindow_t** _window = self->_window;
	if(_window)
	{
		vkk_uiScreen_windowPush(widget->screen, *_window);
	}

	if(self->click_fn)
	{
		(*self->click_fn)(widget, x, y);
	}
}

static int
vkk_uiActionButton_refresh(vkk_uiWidget_t* widget)
{
	ASSERT(widget);

	vkk_uiActionButton_t* self;
	self = (vkk_uiActionButton_t*) widget;

	vkk_uiActionPopup_t** _popup = self->_popup;

	// get the active popup
	vkk_uiActionBar_t*   bar;
	vkk_uiActionPopup_t* popup;
	vkk_uiScreen_popupGet(widget->screen, &bar, &popup);
	if(self->parent != bar)
	{
		popup = NULL;
	}

	cc_vec4f_t color;
	if(*_popup == popup)
	{
		vkk_uiScreen_colorActionIcon1(widget->screen, &color);
	}
	else
	{
		vkk_uiScreen_colorActionIcon0(widget->screen, &color);
	}
	vkk_uiSprite_color(&self->base, &color);

	return 1;
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_uiActionButton_t*
vkk_uiActionButton_new(vkk_uiScreen_t* screen,
                       size_t wsize,
                       vkk_uiActionButtonFn_t* abfn,
                       vkk_uiActionBar_t* parent,
                       const char** sprite_array)
{
	ASSERT(screen);
	ASSERT(abfn);
	ASSERT(parent);
	ASSERT(sprite_array);

	if(wsize == 0)
	{
		wsize = sizeof(vkk_uiActionButton_t);
	}

	vkk_uiSpriteFn_t sfn =
	{
		.priv     = abfn->priv,
		.click_fn = vkk_uiActionButton_click,
	};

	vkk_uiWidgetLayout_t layout =
	{
		.border   = VKK_UI_WIDGET_BORDER_LARGE,
		.wrapx    = VKK_UI_WIDGET_WRAP_STRETCH_TEXT_VMEDIUM,
		.wrapy    = VKK_UI_WIDGET_WRAP_STRETCH_TEXT_VMEDIUM,
		.stretchx = 1.0f,
		.stretchy = 1.0f
	};

	cc_vec4f_t color;
	vkk_uiScreen_colorActionIcon0(screen, &color);

	vkk_uiActionButton_t* self;
	self = (vkk_uiActionButton_t*)
	       vkk_uiSprite_new(screen, wsize, &sfn, &layout,
	                        &color, sprite_array);
	if(self == NULL)
	{
		return NULL;
	}

	self->parent   = parent;
	self->click_fn = abfn->click_fn;

	return self;
}

vkk_uiActionButton_t*
vkk_uiActionButton_newPopup(vkk_uiScreen_t* screen,
                            size_t wsize,
                            vkk_uiActionBar_t* parent,
                            vkk_uiActionPopup_t** _popup,
                            const char** sprite_array)
{
	ASSERT(screen);
	ASSERT(_popup);
	ASSERT(sprite_array);

	if(wsize == 0)
	{
		wsize = sizeof(vkk_uiActionButton_t);
	}

	vkk_uiActionButtonFn_t abfn =
	{
		.priv = NULL,
	};

	vkk_uiActionButton_t* self;
	self = vkk_uiActionButton_new(screen, wsize, &abfn,
	                              parent, sprite_array);
	if(self == NULL)
	{
		return NULL;
	}

	// override functions
	vkk_uiWidget_t*  widget = (vkk_uiWidget_t*) self;
	vkk_uiWidgetFn_t fn_new =
	{
		.refresh_fn = vkk_uiActionButton_refresh,
	};
	vkk_uiWidgetFn_t fn_old =
	{
		.priv = NULL
	};
	vkk_uiWidget_override(widget, &fn_new, &fn_old);

	self->_popup = _popup;

	return self;
}

vkk_uiActionButton_t*
vkk_uiActionButton_newTransition(vkk_uiScreen_t* screen,
                                 size_t wsize,
                                 vkk_uiActionBar_t* parent,
                                 vkk_uiWindow_t** _window,
                                 const char** sprite_array)
{
	ASSERT(screen);
	ASSERT(_window);
	ASSERT(sprite_array);

	if(wsize == 0)
	{
		wsize = sizeof(vkk_uiActionButton_t);
	}

	vkk_uiActionButtonFn_t abfn =
	{
		.priv = NULL,
	};

	vkk_uiActionButton_t* self;
	self = vkk_uiActionButton_new(screen, wsize, &abfn,
	                              parent, sprite_array);
	if(self == NULL)
	{
		return NULL;
	}

	self->_window = _window;

	return self;
}

void vkk_uiActionButton_delete(vkk_uiActionButton_t** _self)
{
	ASSERT(_self);

	vkk_uiActionButton_t* self = *_self;
	if(self)
	{
		vkk_uiSprite_delete((vkk_uiSprite_t**) _self);
	}
}
