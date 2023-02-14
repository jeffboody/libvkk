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

#define LOG_TAG "vkk"
#include "../../libcc/cc_log.h"
#include "../vkk_ui.h"

/***********************************************************
* private                                                  *
***********************************************************/

static vkk_uiWidget_t*
vkk_uiWindow_action(vkk_uiWidget_t* widget,
                    vkk_uiWidgetActionInfo_t* info)
{
	ASSERT(widget);
	ASSERT(info);

	vkk_uiWindow_t* self   = (vkk_uiWindow_t*) widget;
	vkk_uiWidget_t* title  = (vkk_uiWidget_t*) self->title;
	vkk_uiWidget_t* page   = (vkk_uiWidget_t*) self->page;
	vkk_uiWidget_t* layer0 = (vkk_uiWidget_t*) self->layer0;
	vkk_uiWidget_t* layer1 = (vkk_uiWidget_t*) self->layer1;
	vkk_uiWidget_t* footer = (vkk_uiWidget_t*) self->footer;

	// send events front-to-back

	vkk_uiWidget_t* tmp = NULL;
	if(title)
	{
		tmp = vkk_uiWidget_action(title, info);
		if(tmp)
		{
			return tmp;
		}
	}

	if(footer)
	{
		tmp = vkk_uiWidget_action(footer, info);
		if(tmp)
		{
			return tmp;
		}
	}

	if(layer1)
	{
		tmp = vkk_uiWidget_action(layer1, info);
		if(tmp)
		{
			return tmp;
		}
	}

	if(layer0)
	{
		tmp = vkk_uiWidget_action(layer0, info);
		if(tmp)
		{
			return tmp;
		}
	}

	if(page)
	{
		tmp = vkk_uiWidget_action(page, info);
		if(tmp)
		{
			return tmp;
		}
	}

	// only children receive actions
	return NULL;
}

static void
vkk_uiWindow_clickTitle(vkk_uiWidget_t* widget,
                        float x, float y)
{
	ASSERT(widget);

	vkk_uiScreen_popupSet(widget->screen, NULL, NULL);
}

static int
vkk_uiWindow_refresh(vkk_uiWidget_t* widget)
{
	ASSERT(widget);

	vkk_uiWindow_t* self   = (vkk_uiWindow_t*) widget;
	vkk_uiWidget_t* title  = (vkk_uiWidget_t*) self->title;
	vkk_uiWidget_t* page   = (vkk_uiWidget_t*) self->page;
	vkk_uiWidget_t* layer0 = (vkk_uiWidget_t*) self->layer0;
	vkk_uiWidget_t* layer1 = (vkk_uiWidget_t*) self->layer1;
	vkk_uiWidget_t* footer = (vkk_uiWidget_t*) self->footer;

	vkk_uiWidgetRefresh_fn refresh_fn = self->refresh_fn;
	if(refresh_fn && ((*refresh_fn)(widget) == 0))
	{
		return 0;
	}

	if(title)
	{
		vkk_uiWidget_refresh(title);
	}
	if(page)
	{
		vkk_uiWidget_refresh(page);
	}
	if(layer0)
	{
		vkk_uiWidget_refresh(layer0);
	}
	if(layer1)
	{
		vkk_uiWidget_refresh(layer1);
	}
	if(footer)
	{
		vkk_uiWidget_refresh(footer);
	}

	return 1;
}

static void
vkk_uiWindow_size(vkk_uiWidget_t* widget, float* w, float* h)
{
	ASSERT(widget);
	ASSERT(w);
	ASSERT(h);

	vkk_uiWindow_t* self   = (vkk_uiWindow_t*) widget;
	vkk_uiWidget_t* title  = (vkk_uiWidget_t*) self->title;
	vkk_uiWidget_t* page   = (vkk_uiWidget_t*) self->page;
	vkk_uiWidget_t* layer0 = (vkk_uiWidget_t*) self->layer0;
	vkk_uiWidget_t* layer1 = (vkk_uiWidget_t*) self->layer1;
	vkk_uiWidget_t* footer = (vkk_uiWidget_t*) self->footer;
	vkk_renderer_t* rend   = widget->screen->renderer;

	float wmax = 0.0f;
	float hsum = 0.0f;

	// layout title
	float tmp_w = *w;
	float tmp_h = *h;
	if(title)
	{
		vkk_uiWidget_layoutSize(title, &tmp_w, &tmp_h);
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

		vkk_uiWidget_layoutSize(footer, &tmp_w, &tmp_h);
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
		vkk_uiWidget_layoutSize(page, &tmp_w, &tmp_h);

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
		// override layer0 size
		if(self->flags & VKK_UI_WINDOW_FLAG_FULLSCREEN)
		{
			uint32_t width;
			uint32_t height;
			vkk_renderer_surfaceSize(rend, &width, &height);

			tmp_w = (float) width;
			tmp_h = (float) height;

			vkk_uiWidget_layoutSize(layer0, &tmp_w, &tmp_h);
		}
		else
		{
			tmp_w = *w;
			tmp_h = *h - hsum;
			if(tmp_h < 0.0f)
			{
				tmp_h = 0.0f;
			}

			vkk_uiWidget_layoutSize(layer0, &tmp_w, &tmp_h);

			if(tmp_w > wmax)
			{
				wmax = tmp_w;
			}

			if(tmp_h > hbody)
			{
				hbody = tmp_h;
			}
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
		vkk_uiWidget_layoutSize(layer1, &tmp_w, &tmp_h);

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
vkk_uiWindow_layout(vkk_uiWidget_t* widget,
                    int dragx, int dragy)
{
	ASSERT(widget);

	vkk_uiWidgetLayout_t* layout = &widget->layout;
	vkk_uiWindow_t*       self   = (vkk_uiWindow_t*) widget;
	vkk_uiWidget_t*       base   = &self->base;
	vkk_uiWidget_t*       title  = (vkk_uiWidget_t*) self->title;
	vkk_uiWidget_t*       page   = (vkk_uiWidget_t*) self->page;
	vkk_uiWidget_t*       layer0 = (vkk_uiWidget_t*) self->layer0;
	vkk_uiWidget_t*       layer1 = (vkk_uiWidget_t*) self->layer1;
	vkk_uiWidget_t*       footer = (vkk_uiWidget_t*) self->footer;
	vkk_renderer_t*       rend   = widget->screen->renderer;

	// note that the window layout is a bit unique because
	// the top/bottom borders are inserted between title/body
	// and footer/body rather than at the absolute top/bottom
	float h_bo = 0.0f;
	float v_bo = 0.0f;
	vkk_uiScreen_layoutBorder(widget->screen,
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
		vkk_uiWidget_layoutAnchor(title, &rect_draw, &x, &y);
		cc_rect1f_intersect(&rect_draw,
		                    &base->rect_clip,
		                    &rect_clip);
		vkk_uiWidget_layoutXYClip(title, x, y, &rect_clip,
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
		vkk_uiWidget_layoutAnchor(page, &rect_draw, &x, &y);
		cc_rect1f_intersect(&rect_draw,
		                    &base->rect_clip,
		                    &rect_clip);
		vkk_uiWidget_layoutXYClip(page, x, y, &rect_clip,
		                          dragx, dragy);
	}

	// layout layer1
	if(layer1)
	{
		rect_draw.t = t + title_h + v_bo;
		rect_draw.l = l;
		rect_draw.w = w;
		rect_draw.h = h - title_h - footer_h - 2.0f*v_bo;
		vkk_uiWidget_layoutAnchor(layer1, &rect_draw, &x, &y);
		cc_rect1f_intersect(&rect_draw,
		                    &base->rect_clip,
		                    &rect_clip);
		vkk_uiWidget_layoutXYClip(layer1, x, y, &rect_clip,
		                          dragx, dragy);
	}

	// layout footer
	if(footer)
	{
		rect_draw.t = t + h - footer_h;
		rect_draw.l = l;
		rect_draw.w = w;
		rect_draw.h = footer_h;
		vkk_uiWidget_layoutAnchor(footer, &rect_draw, &x, &y);
		cc_rect1f_intersect(&rect_draw,
		                    &base->rect_clip,
		                    &rect_clip);
		vkk_uiWidget_layoutXYClip(footer, x, y, &rect_clip,
		                          dragx, dragy);
	}

	// layout layer0
	if(layer0)
	{
		if(self->flags & VKK_UI_WINDOW_FLAG_FULLSCREEN)
		{
			// override layer0 layout
			uint32_t width;
			uint32_t height;
			vkk_renderer_surfaceSize(rend, &width, &height);

			rect_draw.t = 0;
			rect_draw.l = 0;
			rect_draw.w = (float) width;
			rect_draw.h = (float) height;
			vkk_uiWidget_layoutAnchor(layer0, &rect_draw, &x, &y);
			vkk_uiWidget_layoutXYClip(layer0, x, y, &rect_draw,
			                          dragx, dragy);
		}
		else
		{
			rect_draw.t = t + title_h;
			rect_draw.l = base->rect_border.l;
			rect_draw.w = base->rect_border.w;
			rect_draw.h = h - title_h - footer_h;
			vkk_uiWidget_layoutAnchor(layer0, &rect_draw, &x, &y);
			cc_rect1f_intersect(&rect_draw,
			                    &base->rect_clip,
			                    &rect_clip);
			vkk_uiWidget_layoutXYClip(layer0, x, y, &rect_clip,
			                          dragx, dragy);
		}
	}

	// tricolor values
	float a = t + title_h;
	float b = t + h - footer_h;
	vkk_uiWidget_tricolorAB(widget, a, b);
}

static void
vkk_uiWindow_drag(vkk_uiWidget_t* widget, float x, float y,
                  float dx, float dy)
{
	ASSERT(widget);

	vkk_uiWindow_t* self   = (vkk_uiWindow_t*) widget;
	vkk_uiWidget_t* title  = (vkk_uiWidget_t*) self->title;
	vkk_uiWidget_t* page   = (vkk_uiWidget_t*) self->page;
	vkk_uiWidget_t* layer0 = (vkk_uiWidget_t*) self->layer0;
	vkk_uiWidget_t* layer1 = (vkk_uiWidget_t*) self->layer1;
	vkk_uiWidget_t* footer = (vkk_uiWidget_t*) self->footer;

	if(title)
	{
		vkk_uiWidget_drag(title, x, y, dx, dy);
	}
	if(page)
	{
		vkk_uiWidget_drag(page, x, y, dx, dy);
	}
	if(layer0)
	{
		vkk_uiWidget_drag(layer0, x, y, dx, dy);
	}
	if(layer1)
	{
		vkk_uiWidget_drag(layer1, x, y, dx, dy);
	}
	if(footer)
	{
		vkk_uiWidget_drag(footer, x, y, dx, dy);
	}
}

static void
vkk_uiWindow_scrollTop(vkk_uiWidget_t* widget)
{
	ASSERT(widget);

	vkk_uiWindow_t* self   = (vkk_uiWindow_t*) widget;
	vkk_uiWidget_t* page   = (vkk_uiWidget_t*) self->page;
	vkk_uiWidget_t* layer0 = (vkk_uiWidget_t*) self->layer0;
	vkk_uiWidget_t* layer1 = (vkk_uiWidget_t*) self->layer1;

	if(page)
	{
		vkk_uiWidget_scrollTop(page);
	}
	if(layer0)
	{
		vkk_uiWidget_scrollTop(layer0);
	}
	if(layer1)
	{
		vkk_uiWidget_scrollTop(layer1);
	}
}

static void
vkk_uiWindow_draw(vkk_uiWidget_t* widget)
{
	ASSERT(widget);

	vkk_uiWindow_t* self   = (vkk_uiWindow_t*) widget;
	vkk_uiWidget_t* title  = (vkk_uiWidget_t*) self->title;
	vkk_uiWidget_t* page   = (vkk_uiWidget_t*) self->page;
	vkk_uiWidget_t* layer0 = (vkk_uiWidget_t*) self->layer0;
	vkk_uiWidget_t* layer1 = (vkk_uiWidget_t*) self->layer1;
	vkk_uiWidget_t* footer = (vkk_uiWidget_t*) self->footer;

	if(title)
	{
		vkk_uiWidget_draw(title);
	}
	if(page)
	{
		vkk_uiWidget_draw(page);
	}
	if(layer0)
	{
		vkk_uiWidget_draw(layer0);
	}
	if(layer1)
	{
		vkk_uiWidget_draw(layer1);
	}
	if(footer)
	{
		vkk_uiWidget_draw(footer);
	}
}

static vkk_uiListBox_t*
vkk_uiWindow_newPage(vkk_uiScreen_t* screen,
                     uint32_t flags)
{
	ASSERT(screen);

	vkk_uiWidgetLayout_t layout_default =
	{
		.border   = VKK_UI_WIDGET_BORDER_LARGE,
		.wrapx    = VKK_UI_WIDGET_WRAP_STRETCH_PARENT,
		.stretchx = 1.0f,
	};

	vkk_uiWidgetLayout_t layout_popup =
	{
		.wrapx    = VKK_UI_WIDGET_WRAP_STRETCH_PARENT,
		.stretchx = 1.0f,
	};

	vkk_uiWidgetScroll_t scroll =
	{
		.scroll_bar = 1
	};
	vkk_uiScreen_colorScroll0(screen, &scroll.color0);
	vkk_uiScreen_colorScroll1(screen, &scroll.color1);

	vkk_uiWidgetLayout_t* layout = &layout_default;
	if(flags & VKK_UI_WINDOW_FLAG_PAGE_POPUP)
	{
		layout            = &layout_popup;
		scroll.scroll_bar = 0;
	}

	vkk_uiListBoxFn_t lbfn =
	{
		.priv = NULL
	};

	cc_vec4f_t clear =
	{
		.a = 0.0f
	};

	return vkk_uiListBox_new(screen, 0, &lbfn, layout,
	                         &scroll,
	                         VKK_UI_LISTBOX_ORIENTATION_VERTICAL,
	                         &clear);
}

static vkk_uiLayer_t*
vkk_uiWindow_newLayer(vkk_uiScreen_t* screen, int border)
{
	ASSERT(screen);

	vkk_uiLayerFn_t lfn =
	{
		.priv = NULL,
	};

	vkk_uiWidgetLayout_t layout =
	{
		.border   = border,
		.wrapx    = VKK_UI_WIDGET_WRAP_STRETCH_PARENT,
		.wrapy    = VKK_UI_WIDGET_WRAP_STRETCH_PARENT,
		.stretchx = 1.0f,
		.stretchy = 1.0f
	};

	cc_vec4f_t color =
	{
		.a = 0.0f
	};

	return vkk_uiLayer_new(screen, 0, &lfn, &layout,
	                       &color);
}

static vkk_uiListBox_t*
vkk_uiWindow_newFooter(vkk_uiScreen_t* screen)
{
	ASSERT(screen);

	vkk_uiWidgetLayout_t layout =
	{
		.border   = VKK_UI_WIDGET_BORDER_NONE,
		.anchor   = VKK_UI_WIDGET_ANCHOR_TC,
		.wrapx    = VKK_UI_WIDGET_WRAP_STRETCH_PARENT,
		.stretchx = 1.0f,
	};

	vkk_uiWidgetScroll_t scroll =
	{
		.scroll_bar = 0
	};

	vkk_uiListBoxFn_t lbfn =
	{
		.priv = NULL
	};

	cc_vec4f_t clear =
	{
		.a = 0.0f
	};

	return vkk_uiListBox_new(screen, 0, &lbfn, &layout,
	                         &scroll,
	                         VKK_UI_LISTBOX_ORIENTATION_HORIZONTAL,
	                         &clear);
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_uiWindow_t*
vkk_uiWindow_new(vkk_uiScreen_t* screen,
                 size_t wsize,
                 vkk_uiWindowFn_t* wfn,
                 uint32_t flags)
{
	ASSERT(screen);
	ASSERT(wfn);

	// check for valid page flags
	int page_count = 0;
	if(flags & VKK_UI_WINDOW_FLAG_PAGE_DEFAULT)
	{
		++page_count;
	}
	if(flags & VKK_UI_WINDOW_FLAG_PAGE_POPUP)
	{
		++page_count;
	}
	if(page_count > 1)
	{
		LOGE("invalid flags=0x%X", flags);
		return NULL;
	}

	if(wsize == 0)
	{
		wsize = sizeof(vkk_uiWindow_t);
	}

	cc_vec4f_t clear =
	{
		.a = 0.0f
	};

	vkk_uiWidgetLayout_t layout_default =
	{
		.wrapx    = VKK_UI_WIDGET_WRAP_STRETCH_PARENT,
		.wrapy    = VKK_UI_WIDGET_WRAP_STRETCH_PARENT,
		.stretchx = 1.0f,
		.stretchy = 1.0f
	};

	vkk_uiWidgetLayout_t layout_popup =
	{
		.border   = VKK_UI_WIDGET_BORDER_MEDIUM,
		.anchor   = VKK_UI_WIDGET_ANCHOR_CC,
		.wrapx    = VKK_UI_WIDGET_WRAP_STRETCH_TEXT_HMEDIUM,
		.stretchx = 22.0f,
	};

	vkk_uiWidgetLayout_t* layout = &layout_default;
	if(flags & VKK_UI_WINDOW_FLAG_PAGE_POPUP)
	{
		layout = &layout_popup;
	}

	vkk_uiWidgetScroll_t scroll =
	{
		.scroll_bar = 0
	};

	vkk_uiWidgetFn_t fn =
	{
		.priv         = wfn->priv,
		.action_fn    = vkk_uiWindow_action,
		.drag_fn      = vkk_uiWindow_drag,
		.draw_fn      = vkk_uiWindow_draw,
		.layout_fn    = vkk_uiWindow_layout,
		.refresh_fn   = vkk_uiWindow_refresh,
		.scrollTop_fn = vkk_uiWindow_scrollTop,
		.size_fn      = vkk_uiWindow_size,
	};

	vkk_uiWindow_t* self;
	self = (vkk_uiWindow_t*)
	       vkk_uiWidget_new(screen, wsize, &clear,
	                        layout, &scroll, &fn);
	if(self == NULL)
	{
		return NULL;
	}

	self->flags = flags;

	cc_vec4f_t color_banner;
	cc_vec4f_t color_background;
	vkk_uiScreen_colorBanner(screen,     &color_banner);
	vkk_uiScreen_colorBackground(screen, &color_background);

	if(flags & VKK_UI_WINDOW_FLAG_TRANSPARENT)
	{
		color_background.a = 0.0f;
	}

	if(vkk_uiWidget_tricolor(&self->base,
	                         &color_banner,
	                         &color_background,
	                         &color_banner) == 0)
	{
		goto fail_tricolor;
	}

	vkk_uiBulletBoxStyle_t title_style =
	{
		.text_style =
		{
			.font_type = VKK_UI_TEXT_FONTTYPE_BOLD,
			.size      = VKK_UI_TEXT_SIZE_MEDIUM,
			.spacing   = VKK_UI_TEXT_SPACING_XLARGE,
		}
	};
	vkk_uiScreen_colorBannerText(screen, &title_style.color_icon);
	vkk_uiScreen_colorBannerText(screen, &title_style.text_style.color);

	const char* sprite_array_back[] =
	{
		"vkk/ui/icons/ic_arrow_back_white_24dp.png",
		NULL
	};

	const char* sprite_array_cancel[] =
	{
		"vkk/ui/icons/ic_cancel_white_24dp.png",
		NULL
	};

	const char** sprite_array = sprite_array_back;
	if(flags & VKK_UI_WINDOW_FLAG_PAGE_POPUP)
	{
		sprite_array = sprite_array_cancel;
	}

	if(flags & VKK_UI_WINDOW_FLAG_TITLE)
	{
		vkk_uiBulletBoxFn_t bbfn =
		{
			.click_fn = vkk_uiWidget_clickBack,
		};

		if(flags & VKK_UI_WINDOW_FLAG_PAGE_POPUP)
		{
			bbfn.click_fn = vkk_uiWindow_clickTitle;
		}

		self->title = vkk_uiBulletBox_new(screen, 0, &bbfn,
		                                  VKK_UI_WIDGET_ANCHOR_TL,
		                                  &title_style,
		                                  sprite_array);
		if(self->title == NULL)
		{
			goto fail_title;
		}
	}

	if((flags & VKK_UI_WINDOW_FLAG_PAGE_DEFAULT) ||
	   (flags & VKK_UI_WINDOW_FLAG_PAGE_POPUP))
	{
		self->page = vkk_uiWindow_newPage(screen, flags);
		if(self->page == NULL)
		{
			goto fail_page;
		}
	}

	if(flags & VKK_UI_WINDOW_FLAG_LAYER0)
	{
		self->layer0 = vkk_uiWindow_newLayer(screen,
		                                     VKK_UI_WIDGET_BORDER_NONE);
		if(self->layer0 == NULL)
		{
			goto fail_layer0;
		}
	}

	if(flags & VKK_UI_WINDOW_FLAG_LAYER1)
	{
		self->layer1 = vkk_uiWindow_newLayer(screen,
		                                     VKK_UI_WIDGET_BORDER_LARGE);
		if(self->layer1 == NULL)
		{
			goto fail_layer1;
		}
	}

	if(flags & VKK_UI_WINDOW_FLAG_FOOTER)
	{
		self->footer = vkk_uiWindow_newFooter(screen);
		if(self->footer == NULL)
		{
			goto fail_footer;
		}
	}

	if(color_background.a == 0.0f)
	{
		self->transparent = 1;
	}

	self->refresh_fn = wfn->refresh_fn;

	// success
	return self;

	// failure
	fail_footer:
		vkk_uiLayer_delete(&self->layer1);
	fail_layer1:
		vkk_uiLayer_delete(&self->layer0);
	fail_layer0:
		vkk_uiListBox_delete(&self->page);
	fail_page:
		vkk_uiBulletBox_delete(&self->title);
	fail_title:
	fail_tricolor:
		vkk_uiWidget_delete((vkk_uiWidget_t**) &self);
	return NULL;
}

void vkk_uiWindow_delete(vkk_uiWindow_t** _self)
{
	ASSERT(_self);

	vkk_uiWindow_t* self = *_self;
	if(self)
	{
		vkk_uiListBox_delete(&self->footer);
		vkk_uiLayer_delete(&self->layer1);
		vkk_uiLayer_delete(&self->layer0);
		vkk_uiListBox_delete(&self->page);
		vkk_uiBulletBox_delete(&self->title);
		vkk_uiWidget_delete((vkk_uiWidget_t**) _self);
	}
}

void vkk_uiWindow_fullscreen(vkk_uiWindow_t* self,
                             int fullscreen)
{
	ASSERT(self);

	vkk_uiWidget_t* base = &self->base;

	int enabled = self->flags & VKK_UI_WINDOW_FLAG_FULLSCREEN;
	if(fullscreen && (enabled == 0))
	{
		// enable fullscreen
		self->flags |= VKK_UI_WINDOW_FLAG_FULLSCREEN;
		vkk_uiScreen_layoutDirty(base->screen);
	}
	else if((fullscreen == 0) && enabled)
	{
		// disable fullscreen
		self->flags &= ~VKK_UI_WINDOW_FLAG_FULLSCREEN;
		vkk_uiScreen_layoutDirty(base->screen);
	}
}

void vkk_uiWindow_focus(vkk_uiWindow_t* self,
                        vkk_uiWidget_t* focus)
{
	// focus may be NULL
	ASSERT(self);

	self->focus = focus;
}

void vkk_uiWindow_select(vkk_uiWindow_t* self,
                         uint32_t index)
{
	ASSERT(self);

	if(self->title)
	{
		vkk_uiBulletBox_select(self->title, index);
	}
}

void vkk_uiWindow_label(vkk_uiWindow_t* self,
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

		vkk_uiBulletBox_label(self->title, "%s", string);
	}
}

vkk_uiListBox_t* vkk_uiWindow_page(vkk_uiWindow_t* self)
{
	ASSERT(self);

	return self->page;
}

vkk_uiLayer_t* vkk_uiWindow_layer0(vkk_uiWindow_t* self)
{
	ASSERT(self);

	return self->layer0;
}

vkk_uiLayer_t* vkk_uiWindow_layer1(vkk_uiWindow_t* self)
{
	ASSERT(self);

	return self->layer1;
}

vkk_uiListBox_t* vkk_uiWindow_footer(vkk_uiWindow_t* self)
{
	ASSERT(self);

	return self->footer;
}
