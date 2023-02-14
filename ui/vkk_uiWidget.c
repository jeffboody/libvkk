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

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "vkk"
#include "../../libcc/math/cc_rect12f.h"
#include "../../libcc/cc_memory.h"
#include "../../libcc/cc_log.h"
#include "../vkk_platform.h"
#include "../vkk_ui.h"

/***********************************************************
* private                                                  *
***********************************************************/

static void
vkk_uiWidget_makeRoundRect(cc_vec4f_t* xyuv, int steps,
                           float t, float l,
                           float b, float r,
                           float radius)
{
	ASSERT(xyuv);

	float h = b - t;
	float w = r - l;

	// top-right
	int   i;
	int   idx = 0;
	float s   = (float) (steps - 1);
	for(i = 0; i < steps; ++i)
	{
		float ang = 0.0f + 90.0f*((float) i/s);
		xyuv[idx].x = r + radius*cosf(ang*M_PI/180.0f);
		xyuv[idx].y = t - radius*sinf(ang*M_PI/180.0f);
		xyuv[idx].z = (xyuv[idx].x - l)/w;
		xyuv[idx].w = (xyuv[idx].y - t)/h;
		idx++;
	}

	// top-left
	for(i = 0; i < steps; ++i)
	{
		float ang = 90.0f + 90.0f*((float) i/s);
		xyuv[idx].x = l + radius*cosf(ang*M_PI/180.0f);
		xyuv[idx].y = t - radius*sinf(ang*M_PI/180.0f);
		xyuv[idx].z = (xyuv[idx].x - l)/w;
		xyuv[idx].w = (xyuv[idx].y - t)/h;
		idx++;
	}

	// bottom-left
	for(i = 0; i < steps; ++i)
	{
		float ang = 180.0f + 90.0f*((float) i/s);
		xyuv[idx].x = l + radius*cosf(ang*M_PI/180.0f);
		xyuv[idx].y = b - radius*sinf(ang*M_PI/180.0f);
		xyuv[idx].z = (xyuv[idx].x - l)/w;
		xyuv[idx].w = (xyuv[idx].y - t)/h;
		idx++;
	}

	// bottom-right
	for(i = 0; i < steps; ++i)
	{
		float ang = 270.0f + 90.0f*((float) i/s);
		xyuv[idx].x = r + radius*cosf(ang*M_PI/180.0f);
		xyuv[idx].y = b - radius*sinf(ang*M_PI/180.0f);
		xyuv[idx].z = (xyuv[idx].x - l)/w;
		xyuv[idx].w = (xyuv[idx].y - t)/h;
		idx++;
	}
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_uiWidget_t*
vkk_uiWidget_new(vkk_uiScreen_t* screen, size_t wsize,
                 cc_vec4f_t* color,
                 vkk_uiWidgetLayout_t* layout,
                 vkk_uiWidgetScroll_t* scroll,
                 vkk_uiWidgetFn_t* fn)
{
	ASSERT(screen);
	ASSERT(color);
	ASSERT(layout);
	ASSERT(scroll);
	ASSERT(fn);

	if(wsize == 0)
	{
		wsize = sizeof(vkk_uiWidget_t);
	}

	vkk_uiWidget_t* self = (vkk_uiWidget_t*) CALLOC(1, wsize);
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->screen = screen;

	memcpy(&self->color, color, sizeof(cc_vec4f_t));
	memcpy(&self->layout, layout, sizeof(vkk_uiWidgetLayout_t));
	memcpy(&self->scroll, scroll, sizeof(vkk_uiWidgetScroll_t));
	memcpy(&self->fn, fn, sizeof(vkk_uiWidgetFn_t));

	// check for invalid layouts
	if(layout->wrapx == VKK_UI_WIDGET_WRAP_SHRINK)
	{
		ASSERT(layout->stretchx == 0.0f);
	}

	if(layout->wrapy == VKK_UI_WIDGET_WRAP_SHRINK)
	{
		ASSERT(layout->stretchy == 0.0f);
	}

	// shader data
	self->vb_xyuv = vkk_buffer_new(screen->engine,
	                               VKK_UPDATE_MODE_ASYNCHRONOUS,
	                               VKK_BUFFER_USAGE_VERTEX,
	                               4*VKK_UI_WIDGET_BEZEL*sizeof(cc_vec4f_t),
	                               NULL);
	if(self->vb_xyuv == NULL)
	{
		goto fail_vb_xyuv;
	}

	self->ub10_color = vkk_buffer_new(screen->engine,
	                                VKK_UPDATE_MODE_ASYNCHRONOUS,
	                                VKK_BUFFER_USAGE_UNIFORM,
	                                sizeof(cc_vec4f_t),
	                                color);
	if(self->ub10_color == NULL)
	{
		goto fail_ub10_color;
	}

	vkk_uniformAttachment_t ua_array_color[1] =
	{
		// layout(std140, set=1, binding=0) uniform uniformColor
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.buffer  = self->ub10_color
		},
	};

	self->us1_color = vkk_uniformSet_new(screen->engine, 1, 1,
	                                     ua_array_color,
	                                     screen->usf1_color);
	if(self->us1_color == NULL)
	{
		goto fail_us1_color;
	}

	// add the optional scrollbar
	if(scroll->scroll_bar)
	{
		self->tricolor = vkk_uiTricolor_new(self->screen,
		                                    &scroll->color0,
		                                    &scroll->color1,
		                                    &scroll->color0);
		if(self->tricolor == NULL)
		{
			goto fail_tricolor;
		}
	}

	// success
	return self;

	// failure
	fail_tricolor:
		vkk_uniformSet_delete(&self->us1_color);
	fail_us1_color:
		vkk_buffer_delete(&self->ub10_color);
	fail_ub10_color:
		vkk_buffer_delete(&self->vb_xyuv);
	fail_vb_xyuv:
		FREE(self);
	return NULL;
}

vkk_uiWidget_t*
vkk_uiWidget_newSpace(vkk_uiScreen_t* screen)
{
	ASSERT(screen);

	cc_vec4f_t clear =
	{
		.a=0.0f
	};

	vkk_uiWidgetLayout_t layout =
	{
		.border   = VKK_UI_WIDGET_BORDER_NONE,
		.anchor   = VKK_UI_WIDGET_ANCHOR_CC,
		.wrapx    = VKK_UI_WIDGET_WRAP_STRETCH_TEXT_VMEDIUM,
		.wrapy    = VKK_UI_WIDGET_WRAP_STRETCH_TEXT_VMEDIUM,
		.stretchx = 0.4f,
		.stretchy = 0.4f
	};

	vkk_uiWidgetScroll_t scroll =
	{
		.scroll_bar = 0
	};

	vkk_uiWidgetFn_t fn =
	{
		.priv = NULL
	};

	return vkk_uiWidget_new(screen, 0, &clear, &layout,
	                        &scroll, &fn);
}

void vkk_uiWidget_delete(vkk_uiWidget_t** _self)
{
	ASSERT(_self);

	vkk_uiWidget_t* self = *_self;
	if(self)
	{
		vkk_uiScreen_detach(self->screen, self);
		vkk_uiTricolor_delete(&self->tricolor);
		vkk_uniformSet_delete(&self->us1_color);
		vkk_buffer_delete(&self->ub10_color);
		vkk_buffer_delete(&self->vb_xyuv);
		FREE(self);
		*_self = NULL;
	}
}

void vkk_uiWidget_override(vkk_uiWidget_t* self,
                           vkk_uiWidgetFn_t* fn_new,
                           vkk_uiWidgetFn_t* fn_old)
{
	ASSERT(self);
	ASSERT(fn_new);
	ASSERT(fn_old);

	vkk_uiWidgetFn_t* fn = &self->fn;

	if(fn_new->priv)
	{
		fn_old->priv = fn->priv;
		fn->priv     = fn_new->priv;
	}

	if(fn_new->action_fn)
	{
		fn_old->action_fn = fn->action_fn;
		fn->action_fn     = fn_new->action_fn;
	}

	if(fn_new->aspect_fn)
	{
		fn_old->aspect_fn = fn->aspect_fn;
		fn->aspect_fn     = fn_new->aspect_fn;
	}

	if(fn_new->click_fn)
	{
		fn_old->click_fn = fn->click_fn;
		fn->click_fn     = fn_new->click_fn;
	}

	if(fn_new->drag_fn)
	{
		fn_old->drag_fn = fn->drag_fn;
		fn->drag_fn     = fn_new->drag_fn;
	}

	if(fn_new->draw_fn)
	{
		fn_old->draw_fn = fn->draw_fn;
		fn->draw_fn     = fn_new->draw_fn;
	}

	if(fn_new->input_fn)
	{
		fn_old->input_fn = fn->input_fn;
		fn->input_fn     = fn_new->input_fn;
	}

	if(fn_new->keyPress_fn)
	{
		fn_old->keyPress_fn = fn->keyPress_fn;
		fn->keyPress_fn     = fn_new->keyPress_fn;
	}

	if(fn_new->layout_fn)
	{
		fn_old->layout_fn = fn->layout_fn;
		fn->layout_fn     = fn_new->layout_fn;
	}

	if(fn_new->reflow_fn)
	{
		fn_old->reflow_fn = fn->reflow_fn;
		fn->reflow_fn     = fn_new->reflow_fn;
	}

	if(fn_new->refresh_fn)
	{
		fn_old->refresh_fn = fn->refresh_fn;
		fn->refresh_fn     = fn_new->refresh_fn;
	}

	if(fn_new->scrollTop_fn)
	{
		fn_old->scrollTop_fn = fn->scrollTop_fn;
		fn->scrollTop_fn     = fn_new->scrollTop_fn;
	}

	if(fn_new->size_fn)
	{
		fn_old->size_fn = fn->size_fn;
		fn->size_fn     = fn_new->size_fn;
	}

	if(fn_new->value_fn)
	{
		fn_old->value_fn = fn->value_fn;
		fn->value_fn     = fn_new->value_fn;
	}
}

void vkk_uiWidget_layoutXYClip(vkk_uiWidget_t* self,
                               float x, float y,
                               cc_rect1f_t* clip,
                               int dragx, int dragy)
{
	ASSERT(self);
	ASSERT(clip);

	vkk_uiScreen_t* screen = self->screen;

	vkk_uiWidgetLayout_t* layout = &self->layout;
	vkk_uiWidgetScroll_t* scroll = &self->scroll;
	vkk_uiWidgetFn_t*     fn     = &self->fn;

	float w  = self->rect_border.w;
	float h  = self->rect_border.h;
	float ct = clip->t;
	float cl = clip->l;
	float cw = clip->w;
	float ch = clip->h;
	float w2 = w/2.0f;
	float h2 = h/2.0f;
	float t  = y;
	float l  = x;

	cc_rect1f_copy(clip, &self->rect_clip);

	// anchor the widget origin
	if(layout->anchor == VKK_UI_WIDGET_ANCHOR_TC)
	{
		l = x - w2;
	}
	else if(layout->anchor == VKK_UI_WIDGET_ANCHOR_TR)
	{
		l = x - w;
	}
	else if(layout->anchor == VKK_UI_WIDGET_ANCHOR_CL)
	{
		t = y - h2;
	}
	else if(layout->anchor == VKK_UI_WIDGET_ANCHOR_CC)
	{
		t = y - h2;
		l = x - w2;
	}
	else if(layout->anchor == VKK_UI_WIDGET_ANCHOR_CR)
	{
		t = y - h2;
		l = x - w;
	}
	else if(layout->anchor == VKK_UI_WIDGET_ANCHOR_BL)
	{
		t = y - h;
	}
	else if(layout->anchor == VKK_UI_WIDGET_ANCHOR_BC)
	{
		t = y - h;
		l = x - w2;
	}
	else if(layout->anchor == VKK_UI_WIDGET_ANCHOR_BR)
	{
		t = y - h;
		l = x - w;
	}

	// drag the widget (see dragable rules)
	if((self->layout.wrapx == VKK_UI_WIDGET_WRAP_SHRINK) &&
	   dragx && (w > cw))
	{
		l += self->drag_dx;

		if(l > cl)
		{
			self->drag_dx -= l - cl;
			l = cl;
		}
		else if((l + w) < (cl + cw))
		{
			self->drag_dx += (cl + cw) - (l + w);
			l = cl + cw - w;
		}
		dragx = 0;
	}

	if((self->layout.wrapy == VKK_UI_WIDGET_WRAP_SHRINK) &&
	   dragy && (h > ch))
	{
		t += self->drag_dy;

		if(t > ct)
		{
			self->drag_dy -= t - ct;
			t = ct;
		}
		else if((t + h) < (ct + ch))
		{
			self->drag_dy += (ct + ch) - (t + h);
			t = ct + ch - h;
		}
		dragy = 0;
	}

	// set the layout
	float h_bo = 0.0f;
	float v_bo = 0.0f;
	vkk_uiScreen_layoutBorder(screen, layout->border,
	                          &h_bo, &v_bo);
	self->rect_border.t = t;
	self->rect_border.l = l;
	self->rect_draw.t   = t + v_bo;
	self->rect_draw.l   = l + h_bo;

	// allow the widget to layout it's children
	vkk_uiWidgetLayout_fn layout_fn = fn->layout_fn;
	if(layout_fn)
	{
		(*layout_fn)(self, dragx, dragy);
	}

	// initialize rounded rectangle
	float b = self->rect_border.t + self->rect_border.h;
	float r = self->rect_border.l + self->rect_border.w;
	float radius = (h_bo == v_bo) ? h_bo : 0.0f;
	cc_vec4f_t xyuv[4*VKK_UI_WIDGET_BEZEL];
	vkk_uiWidget_makeRoundRect(xyuv, VKK_UI_WIDGET_BEZEL,
	                           t + v_bo, l + h_bo,
	                           b - v_bo, r - v_bo,
	                           radius);
	// vb_xyuv
	vkk_renderer_updateBuffer(screen->renderer,
	                          self->vb_xyuv,
	                          sizeof(xyuv),
	                          (const void*) xyuv);

	cc_rect1f_t rect_border_clip;
	if(cc_rect1f_intersect(&self->rect_border,
	                       &self->rect_clip,
	                       &rect_border_clip))
	{
		if(scroll->scroll_bar)
		{
			float h_bo = 0.0f;
			float v_bo = 0.0f;
			vkk_uiScreen_layoutBorder(self->screen,
			                          VKK_UI_WIDGET_BORDER_MEDIUM,
			                          &h_bo, &v_bo);

			float w = h_bo;
			float t = rect_border_clip.t;
			float l = rect_border_clip.l + rect_border_clip.w - w;
			float h = rect_border_clip.h;
			cc_rect1f_t rect =
			{
				.t = t,
				.l = l,
				.w = w,
				.h = h,
			};
			vkk_uiTricolor_rect(self->tricolor, &rect);
		}
	}
}

void vkk_uiWidget_layoutSize(vkk_uiWidget_t* self,
                             float* w, float* h)
{
	ASSERT(self);
	ASSERT(w);
	ASSERT(h);

	vkk_uiWidgetLayout_t* layout = &self->layout;
	vkk_uiWidgetFn_t*     fn     = &self->fn;

	vkk_uiFont_t* font;
	font = vkk_uiScreen_font(self->screen,
	                         VKK_UI_TEXT_FONTTYPE_REGULAR);

	float ar = vkk_uiFont_aspectRatioAvg(font);

	float narrow = self->screen->h;
	if(self->screen->w < self->screen->h)
	{
		narrow = self->screen->w;
	}

	// initialize the widget aspect ratio
	float war = 1.0f;
	vkk_uiWidgetAspectRatio_fn aspect_fn = fn->aspect_fn;
	if(aspect_fn)
	{
		(*aspect_fn)(self, &war);
	}

	// initialize size
	int   style;
	float sz    = 0.0f;
	float h_bo  = 0.0f;
	float sh_bo = self->scroll.scroll_bar ? 3.0f : 2.0f;
	float v_bo  = 0.0f;
	vkk_uiScreen_layoutBorder(self->screen, layout->border,
	                          &h_bo, &v_bo);
	if(layout->wrapx == VKK_UI_WIDGET_WRAP_SHRINK)
	{
		self->rect_draw.w   = *w - sh_bo*h_bo;
		self->rect_border.w = *w;
	}
	else if(layout->wrapx == VKK_UI_WIDGET_WRAP_STRETCH_PARENT)
	{
		sz = *w;
		sz *= war*layout->stretchx;
		self->rect_draw.w   = sz - sh_bo*h_bo;
		self->rect_border.w = sz;
	}
	else if((layout->wrapx >= VKK_UI_WIDGET_WRAP_STRETCH_TEXT_VSMALL) &&
	        (layout->wrapx <= VKK_UI_WIDGET_WRAP_STRETCH_TEXT_VLARGE))
	{
		style = layout->wrapx - VKK_UI_WIDGET_WRAP_STRETCH_TEXT_VSMALL;
		sz = war*vkk_uiScreen_layoutText(self->screen, style);
		sz *= layout->stretchx;
		self->rect_draw.w   = sz;
		self->rect_border.w = sz + sh_bo*h_bo;
	}
	else if((layout->wrapx >= VKK_UI_WIDGET_WRAP_STRETCH_TEXT_HSMALL) &&
	        (layout->wrapx <= VKK_UI_WIDGET_WRAP_STRETCH_TEXT_HLARGE))
	{
		style = layout->wrapx - VKK_UI_WIDGET_WRAP_STRETCH_TEXT_HSMALL;
		sz = war*ar*vkk_uiScreen_layoutText(self->screen, style);
		sz *= layout->stretchx;
		self->rect_draw.w   = sz;
		self->rect_border.w = sz + sh_bo*h_bo;
	}
	else if(layout->wrapx == VKK_UI_WIDGET_WRAP_STRETCH_SCREEN)
	{
		sz = war*layout->stretchx*self->screen->w;
		self->rect_draw.w   = sz - sh_bo*h_bo;
		self->rect_border.w = sz;
	}
	else if(layout->wrapx == VKK_UI_WIDGET_WRAP_STRETCH_SCREEN_NARROW)
	{
		sz = war*layout->stretchx*narrow;
		self->rect_draw.w   = sz - sh_bo*h_bo;
		self->rect_border.w = sz;
	}

	// intersect draw with border interior
	if(self->rect_draw.w < 0.0f)
	{
		self->rect_draw.w = 0.0f;
	}

	float rh = 0.0f;
	if(layout->wrapy == VKK_UI_WIDGET_WRAP_SHRINK)
	{
		self->rect_draw.h   = *h - 2.0f*v_bo;
		self->rect_border.h = *h;
	}
	else if(layout->wrapy == VKK_UI_WIDGET_WRAP_STRETCH_PARENT)
	{
		rh = *h;
		rh *= layout->stretchy;
		self->rect_draw.h   = rh - 2.0f*v_bo;
		self->rect_border.h = rh;
	}
	else if((layout->wrapy >= VKK_UI_WIDGET_WRAP_STRETCH_TEXT_VSMALL) &&
	        (layout->wrapy <= VKK_UI_WIDGET_WRAP_STRETCH_TEXT_VLARGE))
	{
		style = layout->wrapy - VKK_UI_WIDGET_WRAP_STRETCH_TEXT_VSMALL;
		sz = vkk_uiScreen_layoutText(self->screen, style);
		sz *= layout->stretchy;
		self->rect_draw.h   = sz;
		self->rect_border.h = sz + 2.0f*v_bo;
	}
	else if((layout->wrapy >= VKK_UI_WIDGET_WRAP_STRETCH_TEXT_HSMALL) &&
	        (layout->wrapy <= VKK_UI_WIDGET_WRAP_STRETCH_TEXT_HLARGE))
	{
		style = layout->wrapy - VKK_UI_WIDGET_WRAP_STRETCH_TEXT_HSMALL;
		sz = ar*vkk_uiScreen_layoutText(self->screen, style);
		sz *= layout->stretchy;
		self->rect_draw.h   = sz;
		self->rect_border.h = sz + 2.0f*v_bo;
	}
	else if(layout->wrapy == VKK_UI_WIDGET_WRAP_STRETCH_SCREEN)
	{
		rh = layout->stretchy*self->screen->h;
		self->rect_draw.h   = rh - 2.0f*v_bo;
		self->rect_border.h = rh;
	}
	else if(layout->wrapy == VKK_UI_WIDGET_WRAP_STRETCH_SCREEN_NARROW)
	{
		rh = layout->stretchy*narrow;
		self->rect_draw.h   = rh - 2.0f*v_bo;
		self->rect_border.h = rh;
	}

	// intersect draw with border interior
	if(self->rect_draw.h < 0.0f)
	{
		self->rect_draw.h = 0.0f;
	}

	// reflow dynamically sized widgets (e.g. textbox)
	// this makes the most sense for stretched widgets
	float draw_w = self->rect_draw.w;
	float draw_h = self->rect_draw.h;
	vkk_uiWidgetReflow_fn reflow_fn = fn->reflow_fn;
	if(reflow_fn)
	{
		(*reflow_fn)(self, draw_w, draw_h);
	}

	// compute draw size for shrink wrapped widgets and
	// recursively compute size of any children
	// the draw size of the widget also becomes the border
	// size of any children
	vkk_uiWidgetSize_fn size_fn = fn->size_fn;
	if(size_fn)
	{
		(*size_fn)(self, &draw_w, &draw_h);
	}

	// wrap width
	if(layout->wrapx == VKK_UI_WIDGET_WRAP_SHRINK)
	{
		self->rect_draw.w   = draw_w;
		self->rect_border.w = draw_w + sh_bo*h_bo;
	}

	// wrap height
	if(layout->wrapy == VKK_UI_WIDGET_WRAP_SHRINK)
	{
		self->rect_draw.h   = draw_h;
		self->rect_border.h = draw_h + 2.0f*v_bo;
	}

	*w = self->rect_border.w;
	*h = self->rect_border.h;
}

void vkk_uiWidget_layoutAnchor(vkk_uiWidget_t* self,
                               cc_rect1f_t* rect,
                               float* x, float * y)
{
	ASSERT(self);
	ASSERT(rect);
	ASSERT(x);
	ASSERT(y);

	vkk_uiWidgetLayout_t* layout = &self->layout;

	// initialize to tl corner
	*x = rect->l;
	*y = rect->t;

	float w  = rect->w;
	float h  = rect->h;
	float w2 = w/2.0f;
	float h2 = h/2.0f;
	if(layout->anchor == VKK_UI_WIDGET_ANCHOR_TC)
	{
		*x += w2;
	}
	else if(layout->anchor == VKK_UI_WIDGET_ANCHOR_TR)
	{
		*x += w;
	}
	else if(layout->anchor == VKK_UI_WIDGET_ANCHOR_CL)
	{
		*y += h2;
	}
	else if(layout->anchor == VKK_UI_WIDGET_ANCHOR_CC)
	{
		*x += w2;
		*y += h2;
	}
	else if(layout->anchor == VKK_UI_WIDGET_ANCHOR_CR)
	{
		*x += w;
		*y += h2;
	}
	else if(layout->anchor == VKK_UI_WIDGET_ANCHOR_BL)
	{
		*y += h;
	}
	else if(layout->anchor == VKK_UI_WIDGET_ANCHOR_BC)
	{
		*x += w2;
		*y += h;
	}
	else if(layout->anchor == VKK_UI_WIDGET_ANCHOR_BR)
	{
		*x += w;
		*y += h;
	}
}

vkk_uiWidget_t*
vkk_uiWidget_action(vkk_uiWidget_t* self,
                    vkk_uiWidgetActionInfo_t* info)
{
	ASSERT(self);
	ASSERT(info);

	vkk_uiWidgetFn_t* fn = &self->fn;

	// check if the widget supports actions
	vkk_uiWidgetClick_fn  click_fn  = fn->click_fn;
	vkk_uiWidgetAction_fn action_fn = fn->action_fn;
	if((action_fn == NULL) && (click_fn == NULL))
	{
		return NULL;
	}

	// always apply the UP/CLICK action for action_fn
	// UP may be used after DRAG, ROTATE, SCALE
	// CLICK may be used after DOWN
	int try_action = 1;
	vkk_uiWidget_t* tmp;
	if(action_fn &&
	   ((info->action == VKK_UI_WIDGET_ACTION_UP) ||
	    (info->action == VKK_UI_WIDGET_ACTION_CLICK)))
	{
		tmp = (*action_fn)(self, info);
		if(tmp)
		{
			return tmp;
		}

		try_action = 0;
	}

	// check if coord(s) intersect with widget
	if(info->count == 1)
	{
		float x0 = info->coord0.x;
		float y0 = info->coord0.y;
		if((cc_rect1f_contains(&self->rect_clip, x0, y0)   == 0) ||
		   (cc_rect1f_contains(&self->rect_border, x0, y0) == 0))
		{
			return NULL;
		}
	}
	else if(info->count == 2)
	{
		float x0 = info->coord0.x;
		float y0 = info->coord0.y;
		float x1 = info->coord1.x;
		float y1 = info->coord1.y;
		if((cc_rect1f_contains(&self->rect_clip, x0, y0)   == 0) ||
		   (cc_rect1f_contains(&self->rect_border, x0, y0) == 0) ||
		   (cc_rect1f_contains(&self->rect_clip, x1, y1)   == 0) ||
		   (cc_rect1f_contains(&self->rect_border, x1, y1) == 0))
		{
			return NULL;
		}
	}

	// handle action
	if(action_fn && try_action)
	{
		tmp = (*action_fn)(self, info);
		if(tmp)
		{
			return tmp;
		}
	}

	// handle click
	if(click_fn)
	{
		if(info->action == VKK_UI_WIDGET_ACTION_CLICK)
		{
			float x0 = info->coord0.x;
			float y0 = info->coord0.y;
			(*click_fn)(self, x0, y0);
			return self;
		}
		if(info->action == VKK_UI_WIDGET_ACTION_DOWN)
		{
			return self;
		}
	}

	return NULL;
}

int vkk_uiWidget_keyPress(vkk_uiWidget_t* self,
                          int keycode, int meta)
{
	ASSERT(self);

	vkk_uiWidgetFn_t*       fn          = &self->fn;
	vkk_uiWidgetKeyPress_fn keyPress_fn = fn->keyPress_fn;
	if(keyPress_fn == NULL)
	{
		return 0;
	}

	return (*keyPress_fn)(self, keycode, meta);
}

void vkk_uiWidget_drag(vkk_uiWidget_t* self,
                       float x, float y,
                       float dx, float dy)
{
	ASSERT(self);

	vkk_uiWidgetLayout_t* layout = &self->layout;
	vkk_uiWidgetFn_t*     fn     = &self->fn;

	if((cc_rect1f_contains(&self->rect_clip, x, y) == 0) ||
	   (cc_rect1f_contains(&self->rect_border, x, y) == 0))
	{
		// don't drag if the pointer is outside the rect
		return;
	}

	if(layout->wrapx == VKK_UI_WIDGET_WRAP_SHRINK)
	{
		self->drag_dx += dx;
		dx = 0.0f;
	}

	if(layout->wrapy == VKK_UI_WIDGET_WRAP_SHRINK)
	{
		self->drag_dy += dy;
		dy = 0.0f;
	}

	// a shrink wrapped widget drags it's children by
	// changing it's own layout
	vkk_uiWidgetDrag_fn drag_fn = fn->drag_fn;
	if(drag_fn &&
	   ((layout->wrapx > VKK_UI_WIDGET_WRAP_SHRINK) ||
	    (layout->wrapy > VKK_UI_WIDGET_WRAP_SHRINK)))
	{
		(*drag_fn)(self, x, y, dx, dy);
	}
}

void vkk_uiWidget_draw(vkk_uiWidget_t* self)
{
	ASSERT(self);

	vkk_uiWidgetScroll_t* scroll = &self->scroll;
	vkk_uiWidgetFn_t*     fn     = &self->fn;

	cc_rect1f_t rect_border_clip;
	if(cc_rect1f_intersect(&self->rect_border,
	                       &self->rect_clip,
	                       &rect_border_clip) == 0)
	{
		return;
	}

	if(cc_rect1f_intersect(&self->rect_draw,
	                       &self->rect_clip,
	                       &self->rect_scissor) == 0)
	{
		return;
	}

	// fill the widget
	vkk_uiScreen_t* screen = self->screen;
	cc_vec4f_t*     color  = &self->color;
	if(color->a > 0.0f)
	{
		vkk_renderer_updateBuffer(screen->renderer,
		                          self->ub10_color,
		                          sizeof(cc_vec4f_t),
		                          (const void*) &self->color);

		vkk_uniformAttachment_t ua =
		{
			.binding = 1,
			.type    = VKK_UNIFORM_TYPE_IMAGE_REF,
			.image   = screen->img21
		};

		vkk_renderer_updateUniformSetRefs(screen->renderer,
		                                  screen->us2_multiplyImage,
		                                  1, &ua);

		vkk_uiScreen_scissor(screen, &rect_border_clip);
		vkk_uiScreen_bind(screen, VKK_UI_SCREEN_BIND_COLOR);
		vkk_uniformSet_t* us_array[] =
		{
			screen->us0_mvp,
			self->us1_color,
			screen->us2_multiplyImage,
			screen->us3_tricolor,
		};
		vkk_renderer_bindUniformSets(screen->renderer,
		                             4, us_array);
		vkk_renderer_draw(screen->renderer, 4*VKK_UI_WIDGET_BEZEL,
		                  1, &self->vb_xyuv);
	}
	else if((scroll->scroll_bar == 0) && self->tricolor)
	{
		// The following is an optimization which eliminates
		// unnecessary window filling when the transparent flag
		// is set.
		float top    = rect_border_clip.t;
		float bot    = rect_border_clip.t + rect_border_clip.h;
		float a      = self->tricolor->a;
		float b      = self->tricolor->b;
		float alpha0 = self->tricolor->alpha0;
		float alpha1 = self->tricolor->alpha1;
		float alpha2 = self->tricolor->alpha2;
		float a0     = alpha0*(a - top);
		float a1     = alpha1*(b - a);
		float a2     = alpha2*(bot - b);
		if(a1 != 0.0f)
		{
			// draw all fill
			vkk_uiScreen_scissor(screen, &rect_border_clip);
			vkk_uiTricolor_drawBuffer0(self->tricolor,
			                           4*VKK_UI_WIDGET_BEZEL,
			                           self->vb_xyuv);
		}
		else
		{
			// draw title fill
			if(a0 != 0.0f)
			{
				cc_rect1f_t rect_border_clip0 =
				{
					.t = top,
					.l = rect_border_clip.l,
					.w = rect_border_clip.w,
					.h = a - top,
				};
				vkk_uiScreen_scissor(screen, &rect_border_clip0);
				vkk_uiTricolor_drawBuffer0(self->tricolor,
				                           4*VKK_UI_WIDGET_BEZEL,
				                           self->vb_xyuv);
			}

			// draw footer fill
			if(a2 != 0.0f)
			{
				cc_rect1f_t rect_border_clip2 =
				{
					.t = b,
					.l = rect_border_clip.l,
					.w = rect_border_clip.w,
					.h = bot - b,
				};
				vkk_uiScreen_scissor(screen, &rect_border_clip2);
				vkk_uiTricolor_drawBuffer1(self->tricolor,
				                           4*VKK_UI_WIDGET_BEZEL,
				                           self->vb_xyuv);
			}
		}
	}

	// draw the contents
	vkk_uiWidgetDraw_fn draw_fn = fn->draw_fn;
	if(draw_fn)
	{
		vkk_uiScreen_scissor(screen, &self->rect_scissor);
		(*draw_fn)(self);
	}

	// draw the scroll bar
	// the 0.99f constant is due to the fact that s can be
	// affected by small precision errors
	float s = self->rect_clip.h/self->rect_border.h;
	if(scroll->scroll_bar && (s < 0.99f))
	{
		// clamp the start/end points
		float a = -self->drag_dy/self->rect_border.h;
		float b = a + s;
		if(a < 0.0f)
		{
			a = 0.0f;
		}
		else if(a > 1.0f)
		{
			a = 1.0f;
		}

		if(b < 0.0f)
		{
			b = 0.0f;
		}
		else if(b > 1.0f)
		{
			b = 1.0f;
		}
		a = rect_border_clip.t + a*rect_border_clip.h;
		b = rect_border_clip.t + b*rect_border_clip.h;

		vkk_uiTricolor_ab(self->tricolor, a, b);
		vkk_uiScreen_scissor(screen, &rect_border_clip);
		vkk_uiTricolor_drawRect(self->tricolor);
	}
}

void vkk_uiWidget_refresh(vkk_uiWidget_t* self)
{
	ASSERT(self);

	vkk_uiWidgetFn_t* fn = &self->fn;

	vkk_uiWidgetRefresh_fn refresh_fn = fn->refresh_fn;
	if(refresh_fn)
	{
		(*refresh_fn)(self);
	}
}

void vkk_uiWidget_color(vkk_uiWidget_t* self,
                        cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	cc_vec4f_copy(color, &self->color);
}

int vkk_uiWidget_tricolor(vkk_uiWidget_t* self,
                          cc_vec4f_t* color0,
                          cc_vec4f_t* color1,
                          cc_vec4f_t* color2)
{
	ASSERT(self);
	ASSERT(color0);
	ASSERT(color1);
	ASSERT(color2);

	if(self->tricolor)
	{
		LOGE("invalid");
		return 0;
	}

	self->tricolor = vkk_uiTricolor_new(self->screen, color0,
	                                    color1, color2);
	if(self->tricolor == NULL)
	{
		return 0;
	}

	return 1;
}

void vkk_uiWidget_tricolorAB(vkk_uiWidget_t* self,
                             float a, float b)
{
	ASSERT(self);

	vkk_uiTricolor_ab(self->tricolor, a, b);
}

void vkk_uiWidget_scrollTop(vkk_uiWidget_t* self)
{
	ASSERT(self);

	vkk_uiWidgetFn_t* fn = &self->fn;

	self->drag_dx = 0.0f;
	self->drag_dy = 0.0f;

	vkk_uiWidgetScrollTop_fn scrollTop_fn = fn->scrollTop_fn;
	if(scrollTop_fn)
	{
		(*scrollTop_fn)(self);
	}
}

int vkk_uiWidget_hasFocus(vkk_uiWidget_t* self)
{
	ASSERT(self);

	return self == self->screen->focus_widget;
}

cc_rect1f_t* vkk_uiWidget_rectDraw(vkk_uiWidget_t* self)
{
	ASSERT(self);

	return &self->rect_draw;
}

cc_rect1f_t* vkk_uiWidget_rectScissor(vkk_uiWidget_t* self)
{
	ASSERT(self);

	return &self->rect_scissor;
}

void* vkk_uiWidget_priv(vkk_uiWidget_t* self)
{
	ASSERT(self);

	return self->fn.priv;
}

void vkk_uiWidget_clickBack(vkk_uiWidget_t* widget,
                            float x0, float y0)
{
	ASSERT(widget);

	vkk_uiScreen_windowPop(widget->screen);
}

void vkk_uiWidget_clickUrl(vkk_uiWidget_t* widget,
                           float x0, float y0)
{
	ASSERT(widget);

	vkk_uiScreen_t* screen = widget->screen;

	const char* url;
	url = (const char*) vkk_uiWidget_priv(widget);

	vkk_engine_platformCmdLoadUrl(screen->engine, url);
}

void vkk_uiWidget_clickTransition(vkk_uiWidget_t* widget,
                                  float x0, float y0)
{
	ASSERT(widget);

	vkk_uiWindow_t** _window;
	_window = (vkk_uiWindow_t**) vkk_uiWidget_priv(widget);

	vkk_uiWindow_t* window = *_window;
	if(window)
	{
		vkk_uiScreen_windowPush(widget->screen, window);
	}
}

void vkk_uiWidget_value(vkk_uiWidget_t* widget,
                        int value)
{
	ASSERT(widget);

	int* _value = (int*) vkk_uiWidget_priv(widget);
	*_value = value;
}
