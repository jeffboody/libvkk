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

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "vkui"
#include "../../libcc/math/cc_rect12f.h"
#include "../../libcc/cc_log.h"
#include "vkui_screen.h"
#include "vkui_widget.h"
#include "vkui.h"

/***********************************************************
* private                                                  *
***********************************************************/

static void
vkui_widget_makeRoundRect(cc_vec4f_t* xyuv, int steps,
                          float t, float l,
                          float b, float r,
                          float radius)
{
	assert(xyuv);

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

vkui_widget_t*
vkui_widget_new(vkui_screen_t* screen, size_t wsize,
                cc_vec4f_t* color,
                vkui_widgetLayout_t* layout,
                vkui_widgetScroll_t* scroll,
                vkui_widgetFn_t* fn,
                vkui_widgetPrivFn_t* priv_fn)
{
	assert(screen);
	assert(color);
	assert(layout);
	assert(scroll);
	assert(fn);
	assert(priv_fn);

	if(wsize == 0)
	{
		wsize = sizeof(vkui_widget_t);
	}

	vkui_widget_t* self = (vkui_widget_t*) calloc(1, wsize);
	if(self == NULL)
	{
		LOGE("calloc failed");
		return NULL;
	}

	self->screen   = screen;
	self->anchor   = VKUI_WIDGET_ANCHOR_TL;
	self->sound_fx = 1;

	memcpy(&self->color, color, sizeof(cc_vec4f_t));
	memcpy(&self->layout, layout, sizeof(vkui_widgetLayout_t));
	memcpy(&self->scroll, scroll, sizeof(vkui_widgetScroll_t));
	memcpy(&self->fn, fn, sizeof(vkui_widgetFn_t));
	memcpy(&self->priv_fn, priv_fn, sizeof(vkui_widgetPrivFn_t));

	// check for invalid layouts
	if(layout->wrapx == VKUI_WIDGET_WRAP_SHRINK)
	{
		assert(layout->aspectx != VKUI_WIDGET_ASPECT_SQUARE);
	}

	if(layout->wrapy == VKUI_WIDGET_WRAP_SHRINK)
	{
		assert(layout->aspecty != VKUI_WIDGET_ASPECT_SQUARE);
	}

	// shader data
	self->vb_color_xyuv = vkk_engine_newBuffer(screen->engine,
	                                           VKK_UPDATE_MODE_DEFAULT,
	                                           VKK_BUFFER_USAGE_VERTEX,
	                                           4*VKUI_WIDGET_BEZEL*sizeof(cc_vec4f_t),
	                                           NULL);
	if(self->vb_color_xyuv == NULL)
	{
		goto fail_vb_color_xyuv;
	}

	self->ub_color = vkk_engine_newBuffer(screen->engine,
	                                      VKK_UPDATE_MODE_STATIC,
	                                      VKK_BUFFER_USAGE_UNIFORM,
	                                      sizeof(cc_vec4f_t),
	                                      color);
	if(self->ub_color == NULL)
	{
		goto fail_ub_color;
	}

	vkk_uniformAttachment_t ua_array_color[1] =
	{
		// layout(std140, set=1, binding=0) uniform uniformColor
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.buffer  = self->ub_color
		},
	};

	self->us_color = vkk_engine_newUniformSet(screen->engine,
	                                          1, 1,
	                                          ua_array_color,
	                                          screen->usf1_color);
	if(self->us_color == NULL)
	{
		goto fail_us_color;
	}

	// success
	return self;

	// failure
	fail_us_color:
		vkk_engine_deleteBuffer(screen->engine, &self->ub_color);
	fail_ub_color:
		vkk_engine_deleteBuffer(screen->engine, &self->vb_color_xyuv);
	fail_vb_color_xyuv:
		free(self);
	return NULL;
}

void vkui_widget_delete(vkui_widget_t** _self)
{
	assert(_self);

	vkui_widget_t* self = *_self;
	if(self)
	{
		vkui_screen_t* screen = self->screen;

		// TODO - screen top

		if(vkui_widget_hasFocus(self))
		{
			vkui_screen_focus(screen, NULL);
		}

		vkk_engine_deleteUniformSet(screen->engine,
		                            &self->us_color);
		vkk_engine_deleteBuffer(screen->engine,
		                        &self->ub_color);
		vkk_engine_deleteBuffer(screen->engine,
		                        &self->vb_color_xyuv);
		free(self);
		*_self = NULL;
	}
}

void vkui_widget_layoutXYClip(vkui_widget_t* self,
                              float x, float y,
                              cc_rect1f_t* clip,
                              int dragx, int dragy)
{
	assert(self);
	assert(clip);

	vkui_screen_t* screen = self->screen;

	vkui_widgetLayout_t* layout  = &self->layout;
	//vkui_widgetScroll_t* scroll  = &self->scroll;
	vkui_widgetPrivFn_t* priv_fn = &self->priv_fn;

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
	if(self->anchor == VKUI_WIDGET_ANCHOR_TC)
	{
		l = x - w2;
	}
	else if(self->anchor == VKUI_WIDGET_ANCHOR_TR)
	{
		l = x - w;
	}
	else if(self->anchor == VKUI_WIDGET_ANCHOR_CL)
	{
		t = y - h2;
	}
	else if(self->anchor == VKUI_WIDGET_ANCHOR_CC)
	{
		t = y - h2;
		l = x - w2;
	}
	else if(self->anchor == VKUI_WIDGET_ANCHOR_CR)
	{
		t = y - h2;
		l = x - w;
	}
	else if(self->anchor == VKUI_WIDGET_ANCHOR_BL)
	{
		t = y - h;
	}
	else if(self->anchor == VKUI_WIDGET_ANCHOR_BC)
	{
		t = y - h;
		l = x - w2;
	}
	else if(self->anchor == VKUI_WIDGET_ANCHOR_BR)
	{
		t = y - h;
		l = x - w;
	}

	// drag the widget (see dragable rules)
	if(dragx && (w > cw))
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

	if(dragy && (h > ch))
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
	vkui_screen_layoutBorder(screen, layout->border,
	                         &h_bo, &v_bo);
	self->rect_border.t = t;
	self->rect_border.l = l;
	self->rect_draw.t   = t + v_bo;
	self->rect_draw.l   = l + h_bo;

	// allow the widget to layout it's children
	vkui_widget_layoutFn layout_fn = priv_fn->layout_fn;
	if(layout_fn)
	{
		(*layout_fn)(self, dragx, dragy);
	}

	// initialize rounded rectangle
	float b = self->rect_border.t + self->rect_border.h;
	float r = self->rect_border.l + self->rect_border.w;
	float radius = (h_bo == v_bo) ? h_bo : 0.0f;
	cc_vec4f_t xyuv[4*VKUI_WIDGET_BEZEL];
	vkui_widget_makeRoundRect(xyuv, VKUI_WIDGET_BEZEL,
	                         t + v_bo, l + h_bo,
	                         b - v_bo, r - v_bo,
	                         radius);
	// vb_color_xyuv
	vkk_renderer_updateBuffer(screen->renderer,
	                          self->vb_color_xyuv,
	                          sizeof(xyuv),
	                          (const void*) xyuv);

	// TODO - scroll_bar
}

void vkui_widget_layoutSize(vkui_widget_t* self,
                            float* w, float* h)
{
	assert(self);
	assert(w);
	assert(h);

	vkui_widgetLayout_t* layout  = &self->layout;
	vkui_widgetPrivFn_t* priv_fn = &self->priv_fn;

	float sw;
	float sh;
	vkui_screen_sizef(self->screen, &sw, &sh);

	// TODO - font
	//vkui_font_t* font = vkui_screen_font(self->screen,
	//                                     VKUI_TEXT_FONTTYPE_REGULAR);
	//float ar    = vkui_font_aspectRatioAvg(font);
	float ar    = 0.5f;
	int   style = layout->wrapy - VKUI_WIDGET_WRAP_STRETCH_TEXT_SMALL;
	float th    = vkui_screen_layoutText(self->screen, style);
	float tw    = ar*th;

	// screen/text/parent square
	float ssq = (sw > sh) ? sh : sw;
	float tsq = th;   // always use the height for square
	float psq = (*w > *h) ? *h : *w;
	int   sqw = (layout->aspectx == VKUI_WIDGET_ASPECT_SQUARE);
	int   sqh = (layout->aspecty == VKUI_WIDGET_ASPECT_SQUARE);

	// initialize size
	float h_bo = 0.0f;
	float v_bo = 0.0f;
	vkui_screen_layoutBorder(self->screen, layout->border,
	                         &h_bo, &v_bo);
	if(layout->wrapx == VKUI_WIDGET_WRAP_SHRINK)
	{
		self->rect_draw.w   = *w - 2.0f*h_bo;
		self->rect_border.w = *w;
	}
	else
	{
		float rw = 0.0f;
		if(layout->wrapx == VKUI_WIDGET_WRAP_STRETCH_SCREEN)
		{
			rw = sqw ? ssq : sw;
			rw *= layout->stretchx;
			self->rect_draw.w   = rw - 2.0f*h_bo;
			self->rect_border.w = rw;
		}
		else if((layout->wrapx >= VKUI_WIDGET_WRAP_STRETCH_TEXT_SMALL) &&
		        (layout->wrapx <= VKUI_WIDGET_WRAP_STRETCH_TEXT_LARGE))
		{
			rw = sqw ? tsq : tw;
			rw *= layout->stretchx;
			self->rect_draw.w   = rw;
			self->rect_border.w = rw + 2.0f*h_bo;
		}
		else
		{
			rw = sqw ? psq : *w;
			rw *= layout->stretchx;
			self->rect_draw.w   = rw - 2.0f*h_bo;
			self->rect_border.w = rw;
		}
	}

	// intersect draw with border interior
	if(self->rect_draw.w < 0.0f)
	{
		self->rect_draw.w = 0.0f;
	}

	if(layout->wrapy == VKUI_WIDGET_WRAP_SHRINK)
	{
		self->rect_draw.h   = *h - 2.0f*v_bo;
		self->rect_border.h = *h;
	}
	else
	{
		float rh = 0.0f;
		if(layout->wrapy == VKUI_WIDGET_WRAP_STRETCH_SCREEN)
		{
			rh = sqh ? ssq : sh;
			rh *= layout->stretchy;
			self->rect_draw.h   = rh - 2.0f*v_bo;
			self->rect_border.h = rh;
		}
		else if((layout->wrapy >= VKUI_WIDGET_WRAP_STRETCH_TEXT_SMALL) &&
		        (layout->wrapy <= VKUI_WIDGET_WRAP_STRETCH_TEXT_LARGE))
		{
			rh = sqh ? tsq : th;
			rh *= layout->stretchy;
			self->rect_draw.h   = rh;
			self->rect_border.h = rh + 2.0f*v_bo;
		}
		else
		{
			rh = sqh ? psq : *h;
			rh *= layout->stretchy;
			self->rect_draw.h   = rh - 2.0f*v_bo;
			self->rect_border.h = rh;
		}
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
	vkui_widget_reflowFn reflow_fn = priv_fn->reflow_fn;
	if(reflow_fn)
	{
		(*reflow_fn)(self, draw_w, draw_h);
	}

	// compute draw size for shrink wrapped widgets and
	// recursively compute size of any children
	// the draw size of the widget also becomes the border
	// size of any children
	vkui_widget_sizeFn size_fn = priv_fn->size_fn;
	if(size_fn)
	{
		(*size_fn)(self, &draw_w, &draw_h);
	}

	// wrap width
	if(layout->wrapx == VKUI_WIDGET_WRAP_SHRINK)
	{
		self->rect_draw.w   = draw_w;
		self->rect_border.w = draw_w + 2.0f*h_bo;
	}

	// wrap height
	if(layout->wrapy == VKUI_WIDGET_WRAP_SHRINK)
	{
		self->rect_draw.h   = draw_h;
		self->rect_border.h = draw_h + 2.0f*v_bo;
	}

	*w = self->rect_border.w;
	*h = self->rect_border.h;
}

void vkui_widget_layoutAnchor(vkui_widget_t* self,
                              cc_rect1f_t* rect,
                              float* x, float * y)
{
	assert(self);
	assert(rect);
	assert(x);
	assert(y);

	// initialize to tl corner
	*x = rect->l;
	*y = rect->t;

	float w  = rect->w;
	float h  = rect->h;
	float w2 = w/2.0f;
	float h2 = h/2.0f;
	if(self->anchor == VKUI_WIDGET_ANCHOR_TC)
	{
		*x += w2;
	}
	else if(self->anchor == VKUI_WIDGET_ANCHOR_TR)
	{
		*x += w;
	}
	else if(self->anchor == VKUI_WIDGET_ANCHOR_CL)
	{
		*y += h2;
	}
	else if(self->anchor == VKUI_WIDGET_ANCHOR_CC)
	{
		*x += w2;
		*y += h2;
	}
	else if(self->anchor == VKUI_WIDGET_ANCHOR_CR)
	{
		*x += w;
		*y += h2;
	}
	else if(self->anchor == VKUI_WIDGET_ANCHOR_BL)
	{
		*y += h;
	}
	else if(self->anchor == VKUI_WIDGET_ANCHOR_BC)
	{
		*x += w2;
		*y += h;
	}
	else if(self->anchor == VKUI_WIDGET_ANCHOR_BR)
	{
		*x += w;
		*y += h;
	}
}

int vkui_widget_click(vkui_widget_t* self, int state,
                      float x, float y)
{
	assert(self);

	vkui_widgetFn_t* fn = &self->fn;

	vkui_widget_clickFn click_fn = fn->click_fn;
	if(click_fn == NULL)
	{
		return 0;
	}

	if((cc_rect1f_contains(&self->rect_clip, x, y) == 0) ||
	   (cc_rect1f_contains(&self->rect_border, x, y) == 0))
	{
		// x, y is outside intersection of rect_border and rect_clip
		return 0;
	}

	int clicked = (*click_fn)(self, fn->priv, state, x, y);
	if(clicked && self->sound_fx &&
	   (state == VKUI_WIDGET_POINTER_UP))
	{
		vkui_screen_playClick(self->screen);
	}

	return clicked;
}

int vkui_widget_keyPress(vkui_widget_t* self,
                         int keycode, int meta)
{
	assert(self);

	vkui_widgetFn_t*     fn      = &self->fn;
	vkui_widgetPrivFn_t* priv_fn = &self->priv_fn;

	vkui_widget_keyPressFn keyPress_fn = priv_fn->keyPress_fn;
	if(keyPress_fn == NULL)
	{
		return 0;
	}

	return (*keyPress_fn)(self, fn->priv, keycode, meta);
}

void vkui_widget_drag(vkui_widget_t* self,
                      float x, float y,
                      float dx, float dy)
{
	assert(self);

	vkui_widgetLayout_t* layout  = &self->layout;
	vkui_widgetPrivFn_t* priv_fn = &self->priv_fn;

	if((cc_rect1f_contains(&self->rect_clip, x, y) == 0) ||
	   (cc_rect1f_contains(&self->rect_border, x, y) == 0))
	{
		// don't drag if the pointer is outside the rect
		return;
	}

	if(layout->wrapx == VKUI_WIDGET_WRAP_SHRINK)
	{
		self->drag_dx += dx;
		dx = 0.0f;
	}

	if(layout->wrapy == VKUI_WIDGET_WRAP_SHRINK)
	{
		self->drag_dy += dy;
		dy = 0.0f;
	}

	// a shrink wrapped widget drags it's children by
	// changing it's own layout
	vkui_widget_dragFn drag_fn = priv_fn->drag_fn;
	if(drag_fn &&
	   ((layout->wrapx > VKUI_WIDGET_WRAP_SHRINK) ||
	    (layout->wrapy > VKUI_WIDGET_WRAP_SHRINK)))
	{
		(*drag_fn)(self, x, y, dx, dy);
	}
}

void vkui_widget_draw(vkui_widget_t* self)
{
	assert(self);

	// TODO - scroll
	// vkui_widgetScroll_t* scroll  = &self->scroll;
	vkui_widgetPrivFn_t* priv_fn = &self->priv_fn;

	cc_rect1f_t rect_border_clip;
	if(cc_rect1f_intersect(&self->rect_border,
	                       &self->rect_clip,
	                       &rect_border_clip) == 0)
	{
		return;
	}

	// fill the widget
	vkui_screen_t* screen       = self->screen;
	cc_vec4f_t*  color = &self->color;
	if(color->a > 0.0f)
	{
		vkui_screen_scissor(screen, &rect_border_clip);
		vkui_screen_bind(screen, VKUI_SCREEN_BIND_COLOR);
		vkk_renderer_bindUniformSets(screen->renderer,
		                             screen->pl, 1,
		                             &self->us_color);
		vkk_renderer_draw(screen->renderer, 4*VKUI_WIDGET_BEZEL,
		                  1, &self->vb_color_xyuv);
	}

	// draw the contents
	cc_rect1f_t rect_draw_clip;
	if(cc_rect1f_intersect(&self->rect_draw, &self->rect_clip,
	                       &rect_draw_clip))
	{
		vkui_widget_drawFn draw_fn = priv_fn->draw_fn;
		if(draw_fn)
		{
			vkui_screen_scissor(screen, &rect_draw_clip);
			(*draw_fn)(self);
		}

		// TODO - draw the scroll bar
	}
}

void vkui_widget_refresh(vkui_widget_t* self)
{
	assert(self);

	vkui_widgetFn_t* fn = &self->fn;

	vkui_widget_refreshFn refresh_fn = fn->refresh_fn;
	if(refresh_fn)
	{
		(*refresh_fn)(self, fn->priv);
	}
}

void vkui_widget_anchor(vkui_widget_t* self, int anchor)
{
	assert(self);

	self->anchor = anchor;
}

void vkui_widget_soundFx(vkui_widget_t* self,
                        int sound_fx)
{
	assert(self);

	self->sound_fx = sound_fx;
}

void vkui_widget_headerY(vkui_widget_t* self, float y)
{
	assert(self);

	self->header_y = y;
}

void vkui_widget_scrollTop(vkui_widget_t* self)
{
	assert(self);

	vkui_widgetPrivFn_t* priv_fn = &self->priv_fn;

	self->drag_dx = 0.0f;
	self->drag_dy = 0.0f;

	vkui_widget_scrollTopFn scrollTop_fn = priv_fn->scrollTop_fn;
	if(scrollTop_fn)
	{
		(*scrollTop_fn)(self);
	}
}

int vkui_widget_hasFocus(vkui_widget_t* self)
{
	assert(self);

	vkui_screen_t* screen = self->screen;
	return self == screen->focus_widget;
}

void
vkui_widget_privReflowFn(vkui_widget_t* self,
                         vkui_widget_reflowFn reflow_fn)
{
	assert(self);
	assert(reflow_fn);

	vkui_widgetPrivFn_t* priv_fn = &self->priv_fn;
	assert(priv_fn->reflow_fn == NULL);

	priv_fn->reflow_fn = reflow_fn;
}
