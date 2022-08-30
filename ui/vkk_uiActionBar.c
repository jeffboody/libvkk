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

static vkk_uiListBox_t*
vkk_uiActionBar_newActions(vkk_uiScreen_t* screen,
                           int anchor,
                           int orientation)
{
	ASSERT(screen);

	vkk_uiWidgetLayout_t layout =
	{
		.border = VKK_UI_WIDGET_BORDER_MEDIUM,
		.anchor = anchor
	};

	vkk_uiWidgetScroll_t scroll =
	{
		.scroll_bar = 0
	};

	vkk_uiListBoxFn_t lbfn =
	{
		.priv = NULL
	};

	cc_vec4f_t color;
	vkk_uiScreen_colorBackground(screen, &color);

	return vkk_uiListBox_new(screen, 0, &lbfn, &layout,
	                         &scroll, orientation, &color);
}

static int vkk_uiActionBar_refresh(vkk_uiWidget_t* widget)
{
	ASSERT(widget);

	vkk_uiActionBar_t* self = (vkk_uiActionBar_t*) widget;
	vkk_uiListBox_t*   base = &self->base;

	vkk_uiWidgetRefresh_fn refresh_fn = self->refresh_fn;
	if(refresh_fn && ((*refresh_fn)(widget) == 0))
	{
		return 0;
	}

	// get the active popup
	vkk_uiActionBar_t*   bar;
	vkk_uiActionPopup_t* popup;
	vkk_uiScreen_popupGet(widget->screen, &bar, &popup);
	if(self != bar)
	{
		popup = NULL;
	}

	// check if the popup changed
	if(popup == self->last_popup)
	{
		return 1;
	}

	// toggle the popup selected
	vkk_uiListBox_clear(base);
	if(popup == NULL)
	{
		vkk_uiListBox_add(base, &self->actions->base);
	}
	else
	{
		if(self->forward)
		{
			vkk_uiListBox_add(base, (vkk_uiWidget_t*) self->actions);
			vkk_uiListBox_add(base, (vkk_uiWidget_t*) self->space);
			vkk_uiListBox_add(base, (vkk_uiWidget_t*) popup);
		}
		else
		{
			vkk_uiListBox_add(base, (vkk_uiWidget_t*) popup);
			vkk_uiListBox_add(base, (vkk_uiWidget_t*) self->space);
			vkk_uiListBox_add(base, (vkk_uiWidget_t*) self->actions);
		}
	}

	self->last_popup = popup;

	return 1;
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_uiActionBar_t*
vkk_uiActionBar_new(vkk_uiScreen_t* screen,
                    size_t wsize,
                    vkk_uiActionBarFn_t* abfn,
                    int anchor,
                    int orientation)
{
	ASSERT(screen);
	ASSERT(abfn);

	if(wsize == 0)
	{
		wsize = sizeof(vkk_uiActionBar_t);
	}

	vkk_uiWidgetLayout_t layout =
	{
		.anchor = anchor
	};

	vkk_uiWidgetScroll_t scroll =
	{
		.scroll_bar = 0
	};

	vkk_uiListBoxFn_t lbfn =
	{
		.priv       = abfn->priv,
		.refresh_fn = vkk_uiActionBar_refresh,
	};

	cc_vec4f_t clear =
	{
		.a = 0.0f
	};

	vkk_uiActionBar_t* self;
	self = (vkk_uiActionBar_t*)
	       vkk_uiListBox_new(screen, wsize, &lbfn,
	                         &layout, &scroll,
	                         1 - orientation, &clear);
	if(self == NULL)
	{
		return NULL;
	}

	if((anchor == VKK_UI_WIDGET_ANCHOR_TL) ||
	   (anchor == VKK_UI_WIDGET_ANCHOR_TC) ||
	   (anchor == VKK_UI_WIDGET_ANCHOR_CL) ||
	   ((anchor      == VKK_UI_WIDGET_ANCHOR_BL) &&
	    (orientation == VKK_UI_LISTBOX_ORIENTATION_VERTICAL)) ||
	   ((anchor      == VKK_UI_WIDGET_ANCHOR_TR) &&
	    (orientation == VKK_UI_LISTBOX_ORIENTATION_HORIZONTAL)))
	{
		self->forward = 1;
	}

	self->actions = vkk_uiActionBar_newActions(screen,
	                                           anchor,
	                                           orientation);
	if(self->actions == NULL)
	{
		goto fail_actions;
	}

	self->space = vkk_uiWidget_newSpace(screen);
	if(self->space == NULL)
	{
		goto fail_space;
	}

	vkk_uiListBox_add(&self->base, (vkk_uiWidget_t*) self->actions);

	self->refresh_fn = abfn->refresh_fn;

	// success
	return self;

	// failure
	fail_space:
		vkk_uiListBox_delete(&self->actions);
	fail_actions:
		vkk_uiListBox_delete((vkk_uiListBox_t**) &self);
	return NULL;
}

void vkk_uiActionBar_delete(vkk_uiActionBar_t** _self)
{
	ASSERT(_self);

	vkk_uiActionBar_t* self = *_self;
	if(self)
	{
		vkk_uiListBox_clear(&self->base);
		vkk_uiWidget_delete(&self->space);
		vkk_uiListBox_delete(&self->actions);
		vkk_uiListBox_delete((vkk_uiListBox_t**) _self);
	}
}

vkk_uiListBox_t*
vkk_uiActionBar_actions(vkk_uiActionBar_t* self)
{
	ASSERT(self);

	return self->actions;
}
