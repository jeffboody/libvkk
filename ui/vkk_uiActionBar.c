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

static vkk_uiListbox_t*
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

	vkk_uiWidgetFn_t fn =
	{
		.priv = NULL
	};

	cc_vec4f_t color;
	vkk_uiScreen_colorBackground(screen, &color);

	return vkk_uiListbox_new(screen, 0, &layout,
	                         &scroll, &fn,
	                         orientation,
	                         &color);
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_uiActionBar_t*
vkk_uiActionBar_new(vkk_uiScreen_t* screen,
                    size_t wsize,
                    int anchor,
                    int orientation,
                    vkk_uiWidget_refreshFn refresh_fn)
{
	// refresh_fn may be NULL
	ASSERT(screen);

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

	vkk_uiWidgetFn_t fn =
	{
		.refresh_fn = refresh_fn
	};

	cc_vec4f_t clear =
	{
		.a = 0.0f
	};

	vkk_uiActionBar_t* self;
	self = (vkk_uiActionBar_t*)
	       vkk_uiListbox_new(screen, wsize,
	                         &layout, &scroll, &fn,
	                         1 - orientation,
	                         &clear);
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

	vkk_uiListbox_t* base = &self->base;
	vkk_uiListbox_add(base, (vkk_uiWidget_t*) self->actions);

	// success
	return self;

	// failure
	fail_space:
		vkk_uiListbox_delete(&self->actions);
	fail_actions:
		vkk_uiListbox_delete((vkk_uiListbox_t**) &self);
	return NULL;
}

void vkk_uiActionBar_delete(vkk_uiActionBar_t** _self)
{
	ASSERT(_self);

	vkk_uiActionBar_t* self = *_self;
	if(self)
	{
		vkk_uiListbox_t* base = &self->base;

		vkk_uiListbox_clear(base);
		vkk_uiWidget_delete(&self->space);
		vkk_uiListbox_delete(&self->actions);
		vkk_uiListbox_delete((vkk_uiListbox_t**) _self);
	}
}

vkk_uiListbox_t*
vkk_uiActionBar_actions(vkk_uiActionBar_t* self)
{
	ASSERT(self);

	return self->actions;
}

int vkk_uiActionBar_popup(vkk_uiActionBar_t* self,
                          vkk_uiSprite_t* action,
                          vkk_uiWindow_t* popup)
{
	// action and popup may be NULL
	ASSERT(self);

	vkk_uiWidget_t*  widget = (vkk_uiWidget_t*) self;
	vkk_uiScreen_t*  screen = widget->screen;
	vkk_uiListbox_t* base   = &self->base;

	if((self->selected == NULL) &&
	   (action == NULL) && (popup == NULL))
	{
		return 0;
	}

	if(self->selected)
	{
		cc_vec4f_t color;
		vkk_uiScreen_colorActionIcon0(screen, &color);
		vkk_uiSprite_color(self->selected, &color);
	}

	// toggle the popup selected
	vkk_uiListbox_clear(base);
	if((action == NULL) || (popup == NULL) ||
	   (action == self->selected))
	{
		self->selected = NULL;

		vkk_uiListbox_add(base, (vkk_uiWidget_t*) self->actions);
	}
	else
	{
		cc_vec4f_t color;
		vkk_uiScreen_colorActionIcon1(screen, &color);
		vkk_uiSprite_color(action, &color);

		self->selected = action;

		if(self->forward)
		{
			vkk_uiListbox_add(base, (vkk_uiWidget_t*) self->actions);
			vkk_uiListbox_add(base, (vkk_uiWidget_t*) self->space);
			vkk_uiListbox_add(base, (vkk_uiWidget_t*) popup);
		}
		else
		{
			vkk_uiListbox_add(base, (vkk_uiWidget_t*) popup);
			vkk_uiListbox_add(base, (vkk_uiWidget_t*) self->space);
			vkk_uiListbox_add(base, (vkk_uiWidget_t*) self->actions);
		}
	}

	return 1;
}

int vkk_uiActionBar_clickPopupFn(vkk_uiWidget_t* widget,
                                 int state,
                                 float x, float y)
{
	ASSERT(widget);

	vkk_uiActionBar_t* ab;
	vkk_uiSprite_t*    action;
	vkk_uiWindow_t**   _popup;
	ab     = (vkk_uiActionBar_t*)
	          vkk_uiWidget_widgetFnPriv(widget);
	action = (vkk_uiSprite_t*) widget;
	_popup = (vkk_uiWindow_t**)
	         vkk_uiWidget_widgetFnArg(widget);
	if(state == VKK_UI_WIDGET_POINTER_UP)
	{
		if((ab == NULL) || (_popup == NULL))
		{
			LOGE("invalid action_bar=%p, popup=%p",
			     ab, _popup);
			return 0;
		}

		vkk_uiActionBar_popup(ab, action, *_popup);
	}
	return 1;
}
