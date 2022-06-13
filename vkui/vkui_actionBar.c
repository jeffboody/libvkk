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

#define LOG_TAG "vkui"
#include "../../libcc/cc_log.h"
#include "../../libvkk/vkui/vkui_widget.h"
#include "vkui_actionBar.h"

/***********************************************************
* private                                                  *
***********************************************************/

static vkui_listbox_t*
vkui_actionBar_newActions(vkui_screen_t* screen,
                          int anchor,
                          int orientation)
{
	ASSERT(self);

	vkui_widgetLayout_t layout =
	{
		.border = VKUI_WIDGET_BORDER_MEDIUM,
		.anchor = anchor
	};

	vkui_widgetScroll_t scroll =
	{
		.scroll_bar = 0
	};

	vkui_widgetFn_t fn =
	{
		.priv = NULL
	};

	cc_vec4f_t color;
	vkui_screen_colorBackground(screen, &color);

	return vkui_listbox_new(screen, 0, &layout,
	                        &scroll, &fn,
	                        orientation,
	                        &color);
}

/***********************************************************
* public                                                   *
***********************************************************/

vkui_actionBar_t*
vkui_actionBar_new(vkui_screen_t* screen,
                   size_t wsize,
                   int anchor,
                   int orientation,
                   vkui_widget_refreshFn refresh_fn)
{
	// refresh_fn may be NULL
	ASSERT(screen);

	if(wsize == 0)
	{
		wsize = sizeof(vkui_actionBar_t);
	}

	vkui_widgetLayout_t layout =
	{
		.anchor = anchor
	};

	vkui_widgetScroll_t scroll =
	{
		.scroll_bar = 0
	};

	vkui_widgetFn_t fn =
	{
		.refresh_fn = refresh_fn
	};

	cc_vec4f_t clear =
	{
		.a = 0.0f
	};

	vkui_actionBar_t* self;
	self = (vkui_actionBar_t*)
	       vkui_listbox_new(screen, wsize,
	                        &layout, &scroll, &fn,
	                        1 - orientation,
	                        &clear);
	if(self == NULL)
	{
		return NULL;
	}

	if((anchor == VKUI_WIDGET_ANCHOR_TL) ||
	   (anchor == VKUI_WIDGET_ANCHOR_TC) ||
	   (anchor == VKUI_WIDGET_ANCHOR_CL) ||
	   ((anchor      == VKUI_WIDGET_ANCHOR_BL) &&
	    (orientation == VKUI_LISTBOX_ORIENTATION_VERTICAL)) ||
	   ((anchor      == VKUI_WIDGET_ANCHOR_TR) &&
	    (orientation == VKUI_LISTBOX_ORIENTATION_HORIZONTAL)))
	{
		self->forward = 1;
	}

	self->actions = vkui_actionBar_newActions(screen,
	                                          anchor,
	                                          orientation);
	if(self->actions == NULL)
	{
		goto fail_actions;
	}

	self->space = vkui_widget_newSpace(screen);
	if(self->space == NULL)
	{
		goto fail_space;
	}

	vkui_listbox_t* listbox = (vkui_listbox_t*) self;
	vkui_listbox_add(listbox, (vkui_widget_t*) self->actions);

	// success
	return self;

	// failure
	fail_space:
		vkui_listbox_delete(&self->actions);
	fail_actions:
		vkui_listbox_delete((vkui_listbox_t**) &self);
	return NULL;
}

void vkui_actionBar_delete(vkui_actionBar_t** _self)
{
	ASSERT(_self);

	vkui_actionBar_t* self = *_self;
	if(self)
	{
		vkui_listbox_t* listbox = (vkui_listbox_t*) self;

		vkui_listbox_clear(listbox);
		vkui_widget_delete(&self->space);
		vkui_listbox_delete(&self->actions);
		vkui_listbox_delete((vkui_listbox_t**) _self);
	}
}

vkui_listbox_t*
vkui_actionBar_actions(vkui_actionBar_t* self)
{
	ASSERT(self);

	return self->actions;
}

int vkui_actionBar_popup(vkui_actionBar_t* self,
                         vkui_sprite_t* action,
                         vkui_window_t* popup)
{
	// action and popup may be NULL
	ASSERT(self);

	vkui_widget_t*  widget  = (vkui_widget_t*) self;
	vkui_screen_t*  screen  = widget->screen;
	vkui_listbox_t* listbox = (vkui_listbox_t*) self;

	if((self->selected == NULL) &&
	   (action == NULL) && (popup == NULL))
	{
		return 0;
	}

	if(self->selected)
	{
		cc_vec4f_t color;
		vkui_screen_colorActionIcon0(screen, &color);
		vkui_sprite_color(self->selected, &color);
	}

	// toggle the popup selected
	vkui_listbox_clear(listbox);
	if((action == NULL) || (popup == NULL) ||
	   (action == self->selected))
	{
		self->selected = NULL;

		vkui_listbox_add(listbox, (vkui_widget_t*) self->actions);
	}
	else
	{
		cc_vec4f_t color;
		vkui_screen_colorActionIcon1(screen, &color);
		vkui_sprite_color(action, &color);

		self->selected = action;

		if(self->forward)
		{
			vkui_listbox_add(listbox, (vkui_widget_t*) self->actions);
			vkui_listbox_add(listbox, (vkui_widget_t*) self->space);
			vkui_listbox_add(listbox, (vkui_widget_t*) popup);
		}
		else
		{
			vkui_listbox_add(listbox, (vkui_widget_t*) popup);
			vkui_listbox_add(listbox, (vkui_widget_t*) self->space);
			vkui_listbox_add(listbox, (vkui_widget_t*) self->actions);
		}
	}

	return 1;
}

int vkui_actionBar_clickPopupFn(vkui_widget_t* widget,
                                void* priv, int state,
                                float x, float y)
{
	ASSERT(widget);
	ASSERT(priv);

	vkui_actionBar_t* ab;
	vkui_sprite_t*    action;
	vkui_window_t**   _popup;
	ab     = (vkui_actionBar_t*) priv;
	action = (vkui_sprite_t*) widget;
	_popup = (vkui_window_t**) widget->fn.arg;
	if(state == VKUI_WIDGET_POINTER_UP)
	{
		if((ab == NULL) || (_popup == NULL))
		{
			LOGE("invalid action_bar=%p, popup=%p",
			     ab, _popup);
			return 0;
		}

		vkui_actionBar_popup(ab, action, *_popup);
	}
	return 1;
}
