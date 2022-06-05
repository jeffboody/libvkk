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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define LOG_TAG "vkui"
#include "../../libcc/cc_log.h"
#include "vkui_bulletbox.h"
#include "vkui_screen.h"
#include "vkui_window.h"
#include "vkui_widget.h"

/***********************************************************
* private                                                  *
***********************************************************/

static vkui_widget_t*
vkui_window_body(vkui_window_t* self)
{
	ASSERT(self);

	if(self->page)
	{
		return (vkui_widget_t*) self->page;
	}
	return (vkui_widget_t*) self->workspace;
}

static int
vkui_window_click(vkui_widget_t* widget, void* priv,
                  int state, float x, float y)
{
	// priv may be NULL
	ASSERT(widget);

	vkui_window_t* self = (vkui_window_t*) widget;
	vkui_widget_t* bullet    = (vkui_widget_t*) self->bullet;
	vkui_widget_t* body      = vkui_window_body(self);
	vkui_widget_t* footer    = (vkui_widget_t*) self->footer;
	if(vkui_widget_click(bullet, state, x, y) ||
	   vkui_widget_click(body, state, x, y)   ||
	   (footer && vkui_widget_click(footer, state, x, y)))
	{
		return 1;
	}

	// windows are always clicked unless transparent
	if(self->transparent)
	{
		return 0;
	}
	return 1;
}

static void
vkui_window_refresh(vkui_widget_t* widget, void* priv)
{
	// priv may be NULL
	ASSERT(widget);

	vkui_window_t* self = (vkui_window_t*) widget;
	vkui_widget_t* body = vkui_window_body(self);
	vkui_widget_refresh((vkui_widget_t*) self->bullet);
	vkui_widget_refresh(body);
	if(self->footer)
	{
		vkui_widget_refresh((vkui_widget_t*) self->footer);
	}
}

static void
vkui_window_size(vkui_widget_t* widget, float* w, float* h)
{
	ASSERT(widget);
	ASSERT(w);
	ASSERT(h);

	vkui_window_t* self   = (vkui_window_t*) widget;
	vkui_widget_t* bullet = (vkui_widget_t*) self->bullet;
	vkui_widget_t* body   = vkui_window_body(self);
	vkui_widget_t* footer = (vkui_widget_t*) self->footer;

	float wmax     = 0.0f;
	float hsum     = 0.0f;
	float bullet_w = *w;
	float bullet_h = *h;

	// layout bullet
	vkui_widget_layoutSize(bullet, &bullet_w, &bullet_h);
	wmax = bullet_w;
	hsum = bullet_h;

	// layout footer
	if(footer)
	{
		float footer_w = *w;
		float footer_h = *h - hsum;
		if(footer_h < 0.0f)
		{
			footer_h = 0.0f;
		}

		vkui_widget_layoutSize(footer, &footer_w, &footer_h);
		if(footer_w > wmax)
		{
			wmax = footer_w;
		}

		hsum += footer_h;
	}

	// layout body
	float body_w = *w;
	float body_h = *h - hsum;
	if(body_h < 0.0f)
	{
		body_h = 0.0f;
	}
	vkui_widget_layoutSize(body, &body_w, &body_h);

	if(body_w > wmax)
	{
		wmax = body_w;
	}
	hsum += body_h;

	*w = wmax;
	*h = hsum;
}

static void
vkui_window_layout(vkui_widget_t* widget,
                   int dragx, int dragy)
{
	ASSERT(widget);

	vkui_widgetLayout_t* layout = &widget->layout;
	vkui_window_t*       self   = (vkui_window_t*) widget;
	vkui_widget_t*       bullet = (vkui_widget_t*) self->bullet;
	vkui_widget_t*       body   = vkui_window_body(self);
	vkui_widget_t*       footer = (vkui_widget_t*) self->footer;

	// note that the window layout is a bit unique because
	// the top/bottom borders are inserted between header/body
	// and footer/body rather than at the absolute top/bottom

	// initialize the layout
	float x  = 0.0f;
	float y  = 0.0f;
	float t  = self->widget.rect_border.t;
	float l  = self->widget.rect_draw.l;
	float w  = self->widget.rect_draw.w;
	float h  = self->widget.rect_border.h;
	float bullet_h = bullet->rect_border.h;
	float footer_h = 0.0f;
	cc_rect1f_t rect_clip;
	cc_rect1f_t rect_draw;

	// layout bullet
	rect_draw.t = t;
	rect_draw.l = l;
	rect_draw.w = w;
	rect_draw.h = bullet_h;
	vkui_widget_layoutAnchor(bullet, &rect_draw, &x, &y);
	cc_rect1f_intersect(&rect_draw,
	                    &self->widget.rect_clip,
	                    &rect_clip);
	vkui_widget_layoutXYClip(bullet, x, y, &rect_clip,
	                         dragx, dragy);

	// get separator size
	float h_bo = 0.0f;
	float v_bo = 0.0f;
	vkui_screen_layoutBorder(widget->screen,
	                         layout->border,
	                         &h_bo, &v_bo);

	// layout body
	if(footer)
	{
		footer_h = footer->rect_border.h;
		rect_draw.t = t + bullet_h + v_bo;
		rect_draw.h = h - bullet_h - footer_h - 2.0f*v_bo;
	}
	else
	{
		rect_draw.t = t + bullet_h + v_bo;
		rect_draw.h = h - bullet_h - 2.0f*v_bo;
	}
	vkui_widget_layoutAnchor(body, &rect_draw, &x, &y);
	cc_rect1f_intersect(&rect_draw,
	                    &self->widget.rect_clip,
	                    &rect_clip);
	vkui_widget_layoutXYClip(body, x, y, &rect_clip,
	                         dragx, dragy);

	// tricolor values
	float a = bullet->rect_border.t + bullet->rect_border.h;
	float b = widget->rect_border.t + widget->rect_border.h;

	// layout footer
	if(footer)
	{
		rect_draw.t = t + h - footer_h;
		rect_draw.l = l;
		rect_draw.w = w;
		rect_draw.h = footer_h;
		vkui_widget_layoutAnchor(footer, &rect_draw, &x, &y);
		cc_rect1f_intersect(&rect_draw,
		                    &self->widget.rect_clip,
		                    &rect_clip);
		vkui_widget_layoutXYClip(footer, x, y, &rect_clip,
		                         dragx, dragy);

		b = footer->rect_border.t;
	}

	vkui_widget_tricolorAB(widget, a, b);
}

static void
vkui_window_drag(vkui_widget_t* widget, float x, float y,
                 float dx, float dy)
{
	ASSERT(widget);

	vkui_window_t* self = (vkui_window_t*) widget;
	vkui_widget_t* body = vkui_window_body(self);
	vkui_widget_drag((vkui_widget_t*) self->bullet,
	                 x, y, dx, dy);
	vkui_widget_drag(body, x, y, dx, dy);
	if(self->footer)
	{
		vkui_widget_drag((vkui_widget_t*) self->footer, x, y, dx, dy);
	}
}

static void
vkui_window_scrollTop(vkui_widget_t* widget)
{
	ASSERT(widget);

	vkui_window_t* self = (vkui_window_t*) widget;
	vkui_widget_t* body = vkui_window_body(self);

	vkui_widget_scrollTop(body);
}

static void
vkui_window_draw(vkui_widget_t* widget)
{
	ASSERT(widget);

	vkui_window_t* self = (vkui_window_t*) widget;
	vkui_widget_t* body = vkui_window_body(self);

	vkui_widget_draw((vkui_widget_t*) self->bullet);
	vkui_widget_draw((vkui_widget_t*) body);
	if(self->footer)
	{
		vkui_widget_draw((vkui_widget_t*) self->footer);
	}
}

static vkui_listbox_t*
vkui_window_newPage(vkui_screen_t* screen,
                    uint32_t flags)
{
	ASSERT(screen);

	vkui_widgetLayout_t layout_default =
	{
		.border   = VKUI_WIDGET_BORDER_LARGE,
		.wrapx    = VKUI_WIDGET_WRAP_STRETCH_PARENT,
		.stretchx = 1.0f,
	};

	vkui_widgetLayout_t layout_popup =
	{
		.wrapx    = VKUI_WIDGET_WRAP_STRETCH_PARENT,
		.stretchx = 1.0f,
	};

	vkui_widgetLayout_t layout_sidebar =
	{
		.border   = VKUI_WIDGET_BORDER_MEDIUM,
		.wrapx    = VKUI_WIDGET_WRAP_STRETCH_PARENT,
		.stretchx = 1.0f,
	};

	vkui_widgetScroll_t scroll =
	{
		.scroll_bar = 1
	};
	vkui_screen_colorScroll0(screen, &scroll.color0);
	vkui_screen_colorScroll1(screen, &scroll.color1);

	vkui_widgetLayout_t* layout = &layout_default;
	if(flags & VKUI_WINDOW_FLAG_POPUP)
	{
		layout            = &layout_popup;
		scroll.scroll_bar = 0;
	}
	else if(flags & VKUI_WINDOW_FLAG_SIDEBAR)
	{
		layout = &layout_sidebar;
	}

	vkui_widgetFn_t empty_fn =
	{
		.priv = NULL
	};

	cc_vec4f_t clear =
	{
		.a = 0.0f
	};

	return vkui_listbox_new(screen, 0, layout,
	                        &scroll, &empty_fn,
	                        VKUI_LISTBOX_ORIENTATION_VERTICAL,
	                        &clear);
}

static vkui_layer_t*
vkui_window_newWorkspace(vkui_screen_t* screen)
{
	ASSERT(screen);

	vkui_widgetLayout_t layout =
	{
		.border   = VKUI_WIDGET_BORDER_LARGE,
		.wrapx    = VKUI_WIDGET_WRAP_STRETCH_PARENT,
		.wrapy    = VKUI_WIDGET_WRAP_STRETCH_PARENT,
		.stretchx = 1.0f,
		.stretchy = 1.0f
	};

	cc_vec4f_t color =
	{
		.a = 0.0f
	};

	return vkui_layer_new(screen, 0, &layout, &color);
}

static vkui_listbox_t*
vkui_window_newFooter(vkui_screen_t* screen)
{
	ASSERT(screen);

	vkui_widgetLayout_t layout =
	{
		.border   = VKUI_WIDGET_BORDER_NONE,
		.anchor   = VKUI_WIDGET_ANCHOR_TC,
		.wrapx    = VKUI_WIDGET_WRAP_STRETCH_PARENT,
		.stretchx = 1.0f,
	};

	vkui_widgetScroll_t scroll =
	{
		.scroll_bar = 0
	};

	vkui_widgetFn_t empty_fn =
	{
		.priv = NULL
	};

	cc_vec4f_t clear =
	{
		.a = 0.0f
	};

	return vkui_listbox_new(screen, 0, &layout,
	                        &scroll, &empty_fn,
	                        VKUI_LISTBOX_ORIENTATION_HORIZONTAL,
	                        &clear);
}

/***********************************************************
* public                                                   *
***********************************************************/

vkui_window_t*
vkui_window_new(vkui_screen_t* screen,
                size_t wsize,
                vkui_windowInfo_t* info)
{
	ASSERT(screen);
	ASSERT(info);

	// check for invalid flag combinations
	if((info->flags & VKUI_WINDOW_FLAG_WORKSPACE) &&
	   ((info->flags & VKUI_WINDOW_FLAG_POPUP)   ||
	    (info->flags & VKUI_WINDOW_FLAG_SIDEBAR)))
	{
		LOGE("invalid flags=%u", info->flags);
		return NULL;
	}

	if(wsize == 0)
	{
		wsize = sizeof(vkui_window_t);
	}

	cc_vec4f_t clear =
	{
		.a = 0.0f
	};

	vkui_widgetLayout_t layout_default =
	{
		.wrapx    = VKUI_WIDGET_WRAP_STRETCH_PARENT,
		.wrapy    = VKUI_WIDGET_WRAP_STRETCH_PARENT,
		.stretchx = 1.0f,
		.stretchy = 1.0f
	};

	vkui_widgetLayout_t layout_popup =
	{
		.border   = VKUI_WIDGET_BORDER_MEDIUM,
		.wrapx    = VKUI_WIDGET_WRAP_STRETCH_TEXT_HMEDIUM,
		.stretchx = 22.0f,
	};

	vkui_widgetLayout_t layout_sidebar =
	{
		.wrapx    = VKUI_WIDGET_WRAP_STRETCH_TEXT_HMEDIUM,
		.wrapy    = VKUI_WIDGET_WRAP_STRETCH_PARENT,
		.stretchx = 24.0f,
		.stretchy = 1.0f
	};

	vkui_widgetLayout_t* layout = &layout_default;
	if(info->flags & VKUI_WINDOW_FLAG_POPUP)
	{
		layout = &layout_popup;
	}
	else if(info->flags & VKUI_WINDOW_FLAG_SIDEBAR)
	{
		layout = &layout_sidebar;
	}

	vkui_widgetScroll_t scroll =
	{
		.scroll_bar = 0
	};

	vkui_widgetFn_t window_fn =
	{
		.click_fn   = vkui_window_click,
		.refresh_fn = vkui_window_refresh
	};

	vkui_widgetPrivFn_t priv_fn =
	{
		.size_fn      = vkui_window_size,
		.layout_fn    = vkui_window_layout,
		.drag_fn      = vkui_window_drag,
		.scrollTop_fn = vkui_window_scrollTop,
		.draw_fn      = vkui_window_draw,
	};

	vkui_window_t* self;
	self = (vkui_window_t*)
	       vkui_widget_new(screen, wsize, &clear,
	                       layout, &scroll, &window_fn,
	                       &priv_fn);
	if(self == NULL)
	{
		return NULL;
	}

	vkui_widget_soundFx((vkui_widget_t*) self, 0);

	cc_vec4f_t color_banner;
	cc_vec4f_t color_background;
	vkui_screen_colorBanner(screen,     &color_banner);
	vkui_screen_colorBackground(screen, &color_background);

	if(info->flags & VKUI_WINDOW_FLAG_TRANSPARENT)
	{
		color_background.a = 0.0f;
	}

	if(vkui_widget_tricolor((vkui_widget_t*) self,
	                        &color_banner,
	                        &color_background,
	                        &color_banner) == 0)
	{
		goto fail_tricolor;
	}

	vkui_bulletboxStyle_t bulletbox_style =
	{
		.text_style =
		{
			.font_type = VKUI_TEXT_FONTTYPE_BOLD,
			.size      = VKUI_TEXT_SIZE_MEDIUM,
			.spacing   = VKUI_TEXT_SPACING_LARGE
		}
	};
	vkui_screen_colorBannerText(screen, &bulletbox_style.color_icon);
	vkui_screen_colorBannerText(screen, &bulletbox_style.text_style.color);

	const char* sprite_array_back[] =
	{
		"vkui/icons/ic_arrow_back_white_24dp.png",
		NULL
	};

	const char* sprite_array_cancel[] =
	{
		"vkui/icons/ic_cancel_white_24dp.png",
		NULL
	};

	const char** sprite_array = sprite_array_back;
	if((info->flags & VKUI_WINDOW_FLAG_POPUP) ||
	   (info->flags & VKUI_WINDOW_FLAG_SIDEBAR))
	{
		sprite_array = sprite_array_cancel;
	}

	self->bullet = vkui_bulletbox_new(screen, 0,
	                                  VKUI_WIDGET_ANCHOR_TL,
	                                  &info->fn,
	                                  &bulletbox_style,
	                                  sprite_array);
	if(self->bullet == NULL)
	{
		goto fail_bullet;
	}
	vkui_window_label(self, "%s", info->label);

	if(color_background.a == 0.0f)
	{
		self->transparent = 1;
	}

	if(info->flags & VKUI_WINDOW_FLAG_WORKSPACE)
	{
		self->workspace = vkui_window_newWorkspace(screen);
		if(self->workspace == NULL)
		{
			goto fail_body;
		}
	}
	else
	{
		self->page = vkui_window_newPage(screen, info->flags);
		if(self->page == NULL)
		{
			goto fail_body;
		}
	}

	// add optional footer
	if(info->flags & VKUI_WINDOW_FLAG_FOOTER)
	{
		self->footer = vkui_window_newFooter(screen);
		if(self->footer == NULL)
		{
			goto fail_footer;
		}
	}

	// success
	return self;

	// failure
	fail_footer:
		vkui_layer_delete(&self->workspace);
		vkui_listbox_delete(&self->page);
	fail_body:
		vkui_bulletbox_delete(&self->bullet);
	fail_bullet:
	fail_tricolor:
		vkui_widget_delete((vkui_widget_t**) &self);
	return NULL;
}

void vkui_window_delete(vkui_window_t** _self)
{
	ASSERT(_self);

	vkui_window_t* self = *_self;
	if(self)
	{
		vkui_listbox_delete(&self->footer);
		vkui_layer_delete(&self->workspace);
		vkui_listbox_delete(&self->page);
		vkui_bulletbox_delete(&self->bullet);
		vkui_widget_delete((vkui_widget_t**) _self);
	}
}

void vkui_window_select(vkui_window_t* self,
                        uint32_t index)
{
	ASSERT(self);

	vkui_bulletbox_select(self->bullet, index);
}

void vkui_window_label(vkui_window_t* self,
                       const char* fmt, ...)
{
	ASSERT(self);
	ASSERT(fmt);

	// decode string
	char    string[256];
	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(string, 256, fmt, argptr);
	va_end(argptr);

	vkui_bulletbox_label(self->bullet, "%s", string);
}

vkui_listbox_t* vkui_window_page(vkui_window_t* self)
{
	ASSERT(self);

	return self->page;
}

vkui_layer_t* vkui_window_workspace(vkui_window_t* self)
{
	ASSERT(self);

	return self->workspace;
}

vkui_listbox_t* vkui_window_footer(vkui_window_t* self)
{
	ASSERT(self);

	return self->footer;
}
