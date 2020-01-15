/*
 * Copyright (c) 2019 Jeff Boody
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
#include "../../libcc/math/cc_vec2f.h"
#include "../../libcc/math/cc_vec4f.h"
#include "../../libcc/cc_log.h"
#include "../../libcc/cc_memory.h"
#include "vkui_tricolor.h"
#include "vkui_screen.h"
#include "vkui_widget.h"

/***********************************************************
* public                                                   *
***********************************************************/

vkui_tricolor_t* vkui_tricolor_new(vkui_screen_t* screen,
                                   cc_vec4f_t* color0,
                                   cc_vec4f_t* color1,
                                   cc_vec4f_t* color2)
{
	ASSERT(screen);
	ASSERT(color0);
	ASSERT(color1);
	ASSERT(color2);

	vkui_tricolor_t* self;
	self = (vkui_tricolor_t*)
	       CALLOC(1, sizeof(vkui_tricolor_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->screen = screen;

	self->vb_xyuv = vkk_buffer_new(screen->engine,
	                               VKK_UPDATE_MODE_DEFAULT,
	                               VKK_BUFFER_USAGE_VERTEX,
	                               4*sizeof(cc_vec4f_t),
	                               NULL);
	if(self->vb_xyuv == NULL)
	{
		goto fail_vb_xyuv;
	}

	self->ub30_color0 = vkk_buffer_new(screen->engine,
	                                   VKK_UPDATE_MODE_STATIC,
	                                   VKK_BUFFER_USAGE_UNIFORM,
	                                   sizeof(cc_vec4f_t),
	                                   color0);
	if(self->ub30_color0 == NULL)
	{
		goto fail_ub30_color0;
	}

	self->ub31_color1 = vkk_buffer_new(screen->engine,
	                                   VKK_UPDATE_MODE_STATIC,
	                                   VKK_BUFFER_USAGE_UNIFORM,
	                                   sizeof(cc_vec4f_t),
	                                   color1);
	if(self->ub31_color1 == NULL)
	{
		goto fail_ub31_color1;
	}

	self->ub32_color2 = vkk_buffer_new(screen->engine,
	                                   VKK_UPDATE_MODE_STATIC,
	                                   VKK_BUFFER_USAGE_UNIFORM,
	                                   sizeof(cc_vec4f_t),
	                                   color2);
	if(self->ub32_color2 == NULL)
	{
		goto fail_ub32_color2;
	}

	self->ub33_ab = vkk_buffer_new(screen->engine,
	                               VKK_UPDATE_MODE_DEFAULT,
	                               VKK_BUFFER_USAGE_UNIFORM,
	                               sizeof(cc_vec2f_t), NULL);
	if(self->ub33_ab == NULL)
	{
		goto fail_ub33_ab;
	}

	vkk_uniformAttachment_t ua_array[4] =
	{
		// layout(std140, set=3, binding=0) uniform uniformColor0
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.buffer  = self->ub30_color0
		},
		// layout(std140, set=3, binding=1) uniform uniformColor1
		{
			.binding = 1,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.buffer  = self->ub31_color1
		},
		// layout(std140, set=3, binding=2) uniform uniformColor2
		{
			.binding = 2,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.buffer  = self->ub32_color2
		},
		// layout(std140, set=3, binding=3) uniform uniformAb
		{
			.binding = 3,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.buffer  = self->ub33_ab
		},
	};

	self->us = vkk_uniformSet_new(screen->engine, 3, 4,
	                              ua_array,
	                              screen->usf3_tricolor);
	if(self->us == NULL)
	{
		goto fail_us;
	}

	// success
	return self;

	// failure
	fail_us:
		vkk_buffer_delete(&self->ub33_ab);
	fail_ub33_ab:
		vkk_buffer_delete(&self->ub32_color2);
	fail_ub32_color2:
		vkk_buffer_delete(&self->ub31_color1);
	fail_ub31_color1:
		vkk_buffer_delete(&self->ub30_color0);
	fail_ub30_color0:
		vkk_buffer_delete(&self->vb_xyuv);
	fail_vb_xyuv:
		FREE(self);
	return NULL;
}

void vkui_tricolor_delete(vkui_tricolor_t** _self)
{
	ASSERT(_self);

	vkui_tricolor_t* self = *_self;
	if(self)
	{
		vkk_uniformSet_delete(&self->us);
		vkk_buffer_delete(&self->ub33_ab);
		vkk_buffer_delete(&self->ub32_color2);
		vkk_buffer_delete(&self->ub31_color1);
		vkk_buffer_delete(&self->ub30_color0);
		vkk_buffer_delete(&self->vb_xyuv);
		FREE(self);
	}
}

void vkui_tricolor_ab(vkui_tricolor_t* self,
                      float a, float b)
{
	ASSERT(self);

	self->a = a;
	self->b = b;
}

void vkui_tricolor_rect(vkui_tricolor_t* self,
                        cc_rect1f_t* rect)
{
	ASSERT(self);
	ASSERT(rect);

	cc_rect1f_copy(rect, &self->rect);
}

void vkui_tricolor_drawBuffer(vkui_tricolor_t* self,
                              uint32_t vc,
                              vkk_buffer_t* vb)
{
	ASSERT(self);
	ASSERT(vb);

	vkui_screen_t* screen = self->screen;

	cc_vec2f_t ab =
	{
		.x = self->a,
		.y = self->b,
	};

	vkk_renderer_updateBuffer(screen->renderer,
	                          self->ub33_ab,
	                          sizeof(cc_vec2f_t),
	                          (const void*) &ab);

	vkui_screen_bind(screen, VKUI_SCREEN_BIND_TRICOLOR);

	vkk_uniformSet_t* us_tri[] =
	{
		screen->us0_mvp,
		screen->us1_color,
		screen->us2_multiplyImage,
		self->us,
	};
	vkk_renderer_bindUniformSets(screen->renderer,
	                             screen->pl, 4,
	                             us_tri);
	vkk_renderer_draw(screen->renderer, vc, 1, &vb);
}

void vkui_tricolor_drawRect(vkui_tricolor_t* self)
{
	ASSERT(self);

	vkui_screen_t* screen = self->screen;
	cc_rect1f_t*   rect   = &self->rect;

	cc_vec2f_t ab =
	{
		.x = self->a,
		.y = self->b,
	};

	vkk_renderer_updateBuffer(screen->renderer,
	                          self->ub33_ab,
	                          sizeof(cc_vec2f_t),
	                          (const void*) &ab);

	float t = rect->t;
	float l = rect->l;
	float b = rect->t + rect->h;
	float r = rect->l + rect->w;
	float xyuv[] =
	{
		r, t, 1.0f, 0.0f, // tr
		l, t, 0.0f, 0.0f, // tl
		l, b, 0.0f, 1.0f, // bl
		r, b, 1.0f, 1.0f, // br
	};

	vkk_renderer_updateBuffer(screen->renderer,
	                          self->vb_xyuv,
	                          4*sizeof(cc_vec4f_t),
	                          (const void*) xyuv);

	vkui_screen_bind(screen, VKUI_SCREEN_BIND_TRICOLOR);
	vkk_uniformSet_t* us_tri[] =
	{
		screen->us0_mvp,
		screen->us1_color,
		screen->us2_multiplyImage,
		self->us,
	};
	vkk_renderer_bindUniformSets(screen->renderer,
	                             screen->pl, 4,
	                             us_tri);
	vkk_renderer_draw(screen->renderer, 4, 1, &self->vb_xyuv);
}
