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
#include "vkui.h"

/***********************************************************
* private                                                  *
***********************************************************/

static int
vkui_window_click(vkui_widget_t* widget,
                  int state, float x, float y)
{
	ASSERT(widget);

	vkui_window_t* self   = (vkui_window_t*) widget;
	vkui_widget_t* title  = (vkui_widget_t*) self->title;
	vkui_widget_t* page   = (vkui_widget_t*) self->page;
	vkui_widget_t* layer0 = (vkui_widget_t*) self->layer0;
	vkui_widget_t* layer1 = (vkui_widget_t*) self->layer1;
	vkui_widget_t* footer = (vkui_widget_t*) self->footer;
	if((title  && vkui_widget_click(title,  state, x, y)) ||
	   (page   && vkui_widget_click(page,   state, x, y)) ||
	   (layer0 && vkui_widget_click(layer0, state, x, y)) ||
	   (layer1 && vkui_widget_click(layer1, state, x, y)) ||
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
vkui_window_refresh(vkui_widget_t* widget)
{
	ASSERT(widget);

	vkui_window_t* self   = (vkui_window_t*) widget;
	vkui_widget_t* title  = (vkui_widget_t*) self->title;
	vkui_widget_t* page   = (vkui_widget_t*) self->page;
	vkui_widget_t* layer0 = (vkui_widget_t*) self->layer0;
	vkui_widget_t* layer1 = (vkui_widget_t*) self->layer1;
	vkui_widget_t* footer = (vkui_widget_t*) self->footer;
	if(title)
	{
		vkui_widget_refresh(title);
	}
	if(page)
	{
		vkui_widget_refresh(page);
	}
	if(layer0)
	{
		vkui_widget_refresh(layer0);
	}
	if(layer1)
	{
		vkui_widget_refresh(layer1);
	}
	if(footer)
	{
		vkui_widget_refresh(footer);
	}
}

static void
vkui_window_size(vkui_widget_t* widget, float* w, float* h)
{
	ASSERT(widget);
	ASSERT(w);
	ASSERT(h);

	vkui_window_t* self   = (vkui_window_t*) widget;
	vkui_widget_t* title  = (vkui_widget_t*) self->title;
	vkui_widget_t* page   = (vkui_widget_t*) self->page;
	vkui_widget_t* layer0 = (vkui_widget_t*) self->layer0;
	vkui_widget_t* layer1 = (vkui_widget_t*) self->layer1;
	vkui_widget_t* footer = (vkui_widget_t*) self->footer;

	float wmax = 0.0f;
	float hsum = 0.0f;

	// layout title
	float tmp_w = *w;
	float tmp_h = *h;
	if(title)
	{
		vkui_widget_layoutSize(title, &tmp_w, &tmp_h);
		wmax = tmp_w;
		hsum = tmp_h;
	}

	// layout footer
	if(footer)
	{
		tmp_w = *w;
		tmp_h = *h - hsum;
		if(tmp_h < 0.0f)
		{
			tmp_h = 0.0f;
		}

		vkui_widget_layoutSize(footer, &tmp_w, &tmp_h);
		if(tmp_w > wmax)
		{
			wmax = tmp_w;
		}

		hsum += tmp_h;
	}

	// layout page
	float hbody = 0.0f;
	if(page)
	{
		tmp_w = *w;
		tmp_h = *h - hsum;
		if(tmp_h < 0.0f)
		{
			tmp_h = 0.0f;
		}
		vkui_widget_layoutSize(page, &tmp_w, &tmp_h);

		if(tmp_w > wmax)
		{
			wmax = tmp_w;
		}

		if(tmp_h > hbody)
		{
			hbody = tmp_h;
		}
	}

	// layout layer0
	if(layer0)
	{
		tmp_w = *w;
		tmp_h = *h - hsum;
		if(tmp_h < 0.0f)
		{
			tmp_h = 0.0f;
		}
		vkui_widget_layoutSize(layer0, &tmp_w, &tmp_h);

		if(tmp_w > wmax)
		{
			wmax = tmp_w;
		}

		if(tmp_h > hbody)
		{
			hbody = tmp_h;
		}
	}

	// layout layer1
	if(layer1)
	{
		tmp_w = *w;
		tmp_h = *h - hsum;
		if(tmp_h < 0.0f)
		{
			tmp_h = 0.0f;
		}
		vkui_widget_layoutSize(layer1, &tmp_w, &tmp_h);

		if(tmp_w > wmax)
		{
			wmax = tmp_w;
		}

		if(tmp_h > hbody)
		{
			hbody = tmp_h;
		}
	}

	*w = wmax;
	*h = hsum + hbody;
}

static void
vkui_window_layout(vkui_widget_t* widget,
                   int dragx, int dragy)
{
	ASSERT(widget);

	vkui_widgetLayout_t* layout = &widget->layout;
	vkui_window_t*       self   = (vkui_window_t*) widget;
	vkui_widget_t*       base   = &self->base;
	vkui_widget_t*       title  = (vkui_widget_t*) self->title;
	vkui_widget_t*       page   = (vkui_widget_t*) self->page;
	vkui_widget_t*       layer0 = (vkui_widget_t*) self->layer0;
	vkui_widget_t*       layer1 = (vkui_widget_t*) self->layer1;
	vkui_widget_t*       footer = (vkui_widget_t*) self->footer;

	// note that the window layout is a bit unique because
	// the top/bottom borders are inserted between title/body
	// and footer/body rather than at the absolute top/bottom
	float h_bo = 0.0f;
	float v_bo = 0.0f;
	vkui_screen_layoutBorder(widget->screen,
	                         layout->border,
	                         &h_bo, &v_bo);

	// initialize the layout
	float t = base->rect_border.t;
	float l = base->rect_draw.l;
	float w = base->rect_draw.w;
	float h = base->rect_border.h;

	// layout title
	float x;
	float y;
	float title_h = 0.0f;
	cc_rect1f_t rect_clip;
	cc_rect1f_t rect_draw;
	if(title)
	{
		title_h     = title->rect_border.h;
		rect_draw.t = t;
		rect_draw.l = l;
		rect_draw.w = w;
		rect_draw.h = title_h;
		vkui_widget_layoutAnchor(title, &rect_draw, &x, &y);
		cc_rect1f_intersect(&rect_draw,
		                    &base->rect_clip,
		                    &rect_clip);
		vkui_widget_layoutXYClip(title, x, y, &rect_clip,
		                         dragx, dragy);
	}

	// init footer
	float footer_h = 0.0f;
	if(footer)
	{
		footer_h = footer->rect_border.h;
	}

	// layout page
	if(page)
	{
		rect_draw.t = t + title_h + v_bo;
		rect_draw.l = l;
		rect_draw.w = w;
		rect_draw.h = h - title_h - footer_h - 2.0f*v_bo;
		vkui_widget_layoutAnchor(page, &rect_draw, &x, &y);
		cc_rect1f_intersect(&rect_draw,
		                    &base->rect_clip,
		                    &rect_clip);
		vkui_widget_layoutXYClip(page, x, y, &rect_clip,
		                         dragx, dragy);
	}

	// layout layer1
	if(layer1)
	{
		rect_draw.t = t + title_h + v_bo;
		rect_draw.l = l;
		rect_draw.w = w;
		rect_draw.h = h - title_h - footer_h - 2.0f*v_bo;
		vkui_widget_layoutAnchor(layer1, &rect_draw, &x, &y);
		cc_rect1f_intersect(&rect_draw,
		                    &base->rect_clip,
		                    &rect_clip);
		vkui_widget_layoutXYClip(layer1, x, y, &rect_clip,
		                         dragx, dragy);
	}

	// layout footer
	if(footer)
	{
		rect_draw.t = t + h - footer_h;
		rect_draw.l = l;
		rect_draw.w = w;
		rect_draw.h = footer_h;
		vkui_widget_layoutAnchor(footer, &rect_draw, &x, &y);
		cc_rect1f_intersect(&rect_draw,
		                    &base->rect_clip,
		                    &rect_clip);
		vkui_widget_layoutXYClip(footer, x, y, &rect_clip,
		                         dragx, dragy);
	}

	// layout layer0
	if(layer0)
	{
		rect_draw.t = t + title_h;
		rect_draw.l = base->rect_border.l;
		rect_draw.w = base->rect_border.w;
		rect_draw.h = h - title_h - footer_h;
		vkui_widget_layoutAnchor(layer0, &rect_draw, &x, &y);
		cc_rect1f_intersect(&rect_draw,
		                    &base->rect_clip,
		                    &rect_clip);
		vkui_widget_layoutXYClip(layer0, x, y, &rect_clip,
		                         dragx, dragy);
	}

	// tricolor values
	float a = t + title_h;
	float b = t + h - footer_h;
	vkui_widget_tricolorAB(widget, a, b);
}

static void
vkui_window_drag(vkui_widget_t* widget, float x, float y,
                 float dx, float dy)
{
	ASSERT(widget);

	vkui_window_t* self   = (vkui_window_t*) widget;
	vkui_widget_t* title  = (vkui_widget_t*) self->title;
	vkui_widget_t* page   = (vkui_widget_t*) self->page;
	vkui_widget_t* layer0 = (vkui_widget_t*) self->layer0;
	vkui_widget_t* layer1 = (vkui_widget_t*) self->layer1;
	vkui_widget_t* footer = (vkui_widget_t*) self->footer;

	if(title)
	{
		vkui_widget_drag(title, x, y, dx, dy);
	}
	if(page)
	{
		vkui_widget_drag(page, x, y, dx, dy);
	}
	if(layer0)
	{
		vkui_widget_drag(layer0, x, y, dx, dy);
	}
	if(layer1)
	{
		vkui_widget_drag(layer1, x, y, dx, dy);
	}
	if(footer)
	{
		vkui_widget_drag(footer, x, y, dx, dy);
	}
}

static void
vkui_window_scrollTop(vkui_widget_t* widget)
{
	ASSERT(widget);

	vkui_window_t* self   = (vkui_window_t*) widget;
	vkui_widget_t* page   = (vkui_widget_t*) self->page;
	vkui_widget_t* layer0 = (vkui_widget_t*) self->layer0;
	vkui_widget_t* layer1 = (vkui_widget_t*) self->layer1;

	if(page)
	{
		vkui_widget_scrollTop(page);
	}
	if(layer0)
	{
		vkui_widget_scrollTop(layer0);
	}
	if(layer1)
	{
		vkui_widget_scrollTop(layer1);
	}
}

static void
vkui_window_draw(vkui_widget_t* widget)
{
	ASSERT(widget);

	vkui_window_t* self   = (vkui_window_t*) widget;
	vkui_widget_t* title  = (vkui_widget_t*) self->title;
	vkui_widget_t* page   = (vkui_widget_t*) self->page;
	vkui_widget_t* layer0 = (vkui_widget_t*) self->layer0;
	vkui_widget_t* layer1 = (vkui_widget_t*) self->layer1;
	vkui_widget_t* footer = (vkui_widget_t*) self->footer;

	if(title)
	{
		vkui_widget_draw(title);
	}
	if(page)
	{
		vkui_widget_draw(page);
	}
	if(layer0)
	{
		vkui_widget_draw(layer0);
	}
	if(layer1)
	{
		vkui_widget_draw(layer1);
	}
	if(footer)
	{
		vkui_widget_draw(footer);
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
		.anchor   = VKUI_WIDGET_ANCHOR_CC,
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
	if(flags & VKUI_WINDOW_FLAG_PAGE_SIDEBAR)
	{
		layout = &layout_sidebar;
	}
	else if(flags & VKUI_WINDOW_FLAG_PAGE_POPUP)
	{
		layout            = &layout_popup;
		scroll.scroll_bar = 0;
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
vkui_window_newLayer(vkui_screen_t* screen, int border)
{
	ASSERT(screen);

	vkui_widgetLayout_t layout =
	{
		.border   = border,
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

	// check for valid page flags
	int page_count = 0;
	if(info->flags & VKUI_WINDOW_FLAG_PAGE_DEFAULT)
	{
		++page_count;
	}
	if(info->flags & VKUI_WINDOW_FLAG_PAGE_SIDEBAR)
	{
		++page_count;
	}
	if(info->flags & VKUI_WINDOW_FLAG_PAGE_POPUP)
	{
		++page_count;
	}
	if(page_count > 1)
	{
		LOGE("invalid flags=0x%X", info->flags);
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
	if(info->flags & VKUI_WINDOW_FLAG_PAGE_SIDEBAR)
	{
		layout = &layout_sidebar;
	}
	else if(info->flags & VKUI_WINDOW_FLAG_PAGE_POPUP)
	{
		layout = &layout_popup;
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

	vkui_widget_t* base = &self->base;
	vkui_widget_soundFx(base, 0);

	cc_vec4f_t color_banner;
	cc_vec4f_t color_background;
	vkui_screen_colorBanner(screen,     &color_banner);
	vkui_screen_colorBackground(screen, &color_background);

	if(info->flags & VKUI_WINDOW_FLAG_TRANSPARENT)
	{
		color_background.a = 0.0f;
	}

	if(vkui_widget_tricolor(base,
	                        &color_banner,
	                        &color_background,
	                        &color_banner) == 0)
	{
		goto fail_tricolor;
	}

	vkui_bulletboxStyle_t title_style =
	{
		.text_style =
		{
			.font_type = VKUI_TEXT_FONTTYPE_BOLD,
			.size      = VKUI_TEXT_SIZE_MEDIUM,
			.spacing   = VKUI_TEXT_SPACING_LARGE
		}
	};
	vkui_screen_colorBannerText(screen, &title_style.color_icon);
	vkui_screen_colorBannerText(screen, &title_style.text_style.color);

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
	if((info->flags & VKUI_WINDOW_FLAG_PAGE_POPUP) ||
	   (info->flags & VKUI_WINDOW_FLAG_PAGE_SIDEBAR))
	{
		sprite_array = sprite_array_cancel;
	}

	if(info->flags & VKUI_WINDOW_FLAG_TITLE)
	{
		self->title = vkui_bulletbox_new(screen, 0,
		                                 VKUI_WIDGET_ANCHOR_TL,
		                                 &info->fn,
		                                 &title_style,
		                                 sprite_array);
		if(self->title == NULL)
		{
			goto fail_title;
		}
		vkui_window_label(self, "%s", info->label);
	}

	if((info->flags & VKUI_WINDOW_FLAG_PAGE_DEFAULT) ||
	   (info->flags & VKUI_WINDOW_FLAG_PAGE_SIDEBAR) ||
	   (info->flags & VKUI_WINDOW_FLAG_PAGE_POPUP))
	{
		self->page = vkui_window_newPage(screen, info->flags);
		if(self->page == NULL)
		{
			goto fail_page;
		}
	}

	if(info->flags & VKUI_WINDOW_FLAG_LAYER0)
	{
		self->layer0 = vkui_window_newLayer(screen,
		                                    VKUI_WIDGET_BORDER_NONE);
		if(self->layer0 == NULL)
		{
			goto fail_layer0;
		}
	}

	if(info->flags & VKUI_WINDOW_FLAG_LAYER1)
	{
		self->layer1 = vkui_window_newLayer(screen,
		                                    VKUI_WIDGET_BORDER_LARGE);
		if(self->layer1 == NULL)
		{
			goto fail_layer1;
		}
	}

	if(info->flags & VKUI_WINDOW_FLAG_FOOTER)
	{
		self->footer = vkui_window_newFooter(screen);
		if(self->footer == NULL)
		{
			goto fail_footer;
		}
	}

	if(color_background.a == 0.0f)
	{
		self->transparent = 1;
	}

	// success
	return self;

	// failure
	fail_footer:
		vkui_layer_delete(&self->layer1);
	fail_layer1:
		vkui_layer_delete(&self->layer0);
	fail_layer0:
		vkui_listbox_delete(&self->page);
	fail_page:
		vkui_bulletbox_delete(&self->title);
	fail_title:
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
		vkui_layer_delete(&self->layer1);
		vkui_layer_delete(&self->layer0);
		vkui_listbox_delete(&self->page);
		vkui_bulletbox_delete(&self->title);
		vkui_widget_delete((vkui_widget_t**) _self);
	}
}

void vkui_window_focus(vkui_window_t* self,
                       vkui_widget_t* focus)
{
	// focus may be NULL
	ASSERT(self);

	self->focus = focus;
}

void vkui_window_select(vkui_window_t* self,
                        uint32_t index)
{
	ASSERT(self);

	if(self->title)
	{
		vkui_bulletbox_select(self->title, index);
	}
}

void vkui_window_label(vkui_window_t* self,
                       const char* fmt, ...)
{
	ASSERT(self);
	ASSERT(fmt);

	if(self->title)
	{
		// decode string
		char    string[256];
		va_list argptr;
		va_start(argptr, fmt);
		vsnprintf(string, 256, fmt, argptr);
		va_end(argptr);

		vkui_bulletbox_label(self->title, "%s", string);
	}
}

vkui_listbox_t* vkui_window_page(vkui_window_t* self)
{
	ASSERT(self);

	return self->page;
}

vkui_layer_t* vkui_window_layer0(vkui_window_t* self)
{
	ASSERT(self);

	return self->layer0;
}

vkui_layer_t* vkui_window_layer1(vkui_window_t* self)
{
	ASSERT(self);

	return self->layer1;
}

vkui_listbox_t* vkui_window_footer(vkui_window_t* self)
{
	ASSERT(self);

	return self->footer;
}
