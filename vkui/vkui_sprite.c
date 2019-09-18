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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "vkui"
#include "../../libcc/math/cc_mat4f.h"
#include "../../libcc/cc_log.h"
#include "vkui_screen.h"
#include "vkui_sprite.h"
#include "vkui_widget.h"
#include "vkui.h"

static const float XYUV[] =
{
	 0.5f, -0.5f, 1.0f, 0.0f,   // top-right
	-0.5f, -0.5f, 0.0f, 0.0f,   // top-left
	-0.5f,  0.5f, 0.0f, 1.0f,   // bottom-left
	 0.5f,  0.5f, 1.0f, 1.0f,   // bottom-right
};

/***********************************************************
* private                                                  *
***********************************************************/

static void vkui_sprite_draw(vkui_widget_t* widget)
{
	assert(widget);

	vkui_sprite_t*      self   = (vkui_sprite_t*) widget;
	vkui_screen_t*      screen = widget->screen;
	vkui_spriteImage_t* img    = self->image_array[self->index];

	vkui_screen_bind(widget->screen, VKUI_SCREEN_BIND_IMAGE);

	int multiply   = 1;
	int img_format = vkk_image_format(img->image);
	if(img_format == VKK_IMAGE_FORMAT_R8)
	{
		multiply = 0;
	}
	vkk_renderer_updateBuffer(screen->renderer,
	                          self->ub_multiply,
	                          sizeof(int),
	                          (const void*) &multiply);

	// rotation
	cc_mat4f_t mvp;
	cc_mat4f_t mvm;
	cc_mat4f_t pm;
	float w  = 0.0f;
	float h  = 0.0f;
	float x  = widget->rect_draw.l;
	float y  = widget->rect_draw.t;
	float ww = widget->rect_draw.w;
	float hh = widget->rect_draw.h;
	vkui_screen_sizef(screen, &w, &h);
	cc_mat4f_ortho(&pm, 1, 0.0f, w, 0.0f, h, 0.0f, 2.0f);
	cc_mat4f_translate(&mvm, 1, x + ww/2.0f, y + hh/2.0f, 0.0f);
	cc_mat4f_rotate(&mvm, 0, self->theta, 0.0f, 0.0f, 1.0f);
	cc_mat4f_scale(&mvm, 0, ww, hh, 1.0f);
	cc_mat4f_mulm_copy(&pm, &mvm, &mvp);
	vkk_renderer_updateBuffer(screen->renderer, self->ub_mvp,
	                          sizeof(cc_mat4f_t),
	                          (const void*) &mvp);

	vkk_uniformSet_t* us_array[4] =
	{
		self->us_mvp,
		self->us_color,
		self->us_multiply,
		img->us_image
	};

	vkk_renderer_bindUniformSets(screen->renderer,
	                             screen->pl, 4,
	                             us_array);
	vkk_renderer_draw(screen->renderer, 4,
	                  1, &self->vb_color_xyuv);
}

/***********************************************************
* public                                                   *
***********************************************************/

vkui_sprite_t*
vkui_sprite_new(vkui_screen_t* screen,
                size_t wsize,
                vkui_widgetLayout_t* layout,
                vkui_widgetFn_t* fn,
                cc_vec4f_t* color,
                const char** sprite_array)
{
	assert(screen);
	assert(layout);
	assert(fn);
	assert(color);
	assert(sprite_array);

	uint32_t count = 0;
	while(sprite_array[count])
	{
		++count;
	}

	if(count <= 0)
	{
		LOGE("invalid count=%i", count);
		return NULL;
	}

	if((layout->wrapx == VKUI_WIDGET_WRAP_SHRINK) ||
	   (layout->wrapy == VKUI_WIDGET_WRAP_SHRINK))
	{
		LOGE("invalid wrapx=%i, wrapy=%i",
		     layout->wrapx, layout->wrapy);
		return NULL;
	}

	if(wsize == 0)
	{
		wsize = sizeof(vkui_sprite_t);
	}

	vkui_widgetScroll_t scroll =
	{
		.scroll_bar = 0
	};

	vkui_widgetPrivFn_t priv_fn =
	{
		.draw_fn = vkui_sprite_draw,
	};

	cc_vec4f_t clear =
	{
		.r = 0.0f,
		.g = 0.0f,
		.b = 0.0f,
		.a = 0.0f,
	};

	vkui_sprite_t* self;
	self = (vkui_sprite_t*)
	       vkui_widget_new(screen, wsize, &clear,
	                       layout, &scroll, fn, &priv_fn);
	if(self == NULL)
	{
		return NULL;
	}

	self->count = count;
	self->index = 0;
	self->theta = 0.0f;

	self->image_array = (vkui_spriteImage_t**)
	                    calloc(count,
	                           sizeof(vkui_spriteImage_t));
	if(self->image_array == NULL)
	{
		LOGE("calloc failed");
		goto fail_image_array;
	}

	int i;
	for(i = 0; i < count; ++i)
	{
		vkui_spriteImage_t* img;
		img = vkui_screen_spriteImage(screen,
		                              sprite_array[i]);
		if(img == NULL)
		{
			goto fail_img;
		}
		self->image_array[i] = img;
	}

	self->ub_mvp = vkk_engine_newBuffer(screen->engine,
	                                    VKK_UPDATE_MODE_DEFAULT,
	                                    VKK_BUFFER_USAGE_UNIFORM,
	                                    sizeof(cc_mat4f_t),
	                                    NULL);
	if(self->ub_mvp == NULL)
	{
		goto fail_ub_mvp;
	}

	self->vb_color_xyuv = vkk_engine_newBuffer(screen->engine,
	                                           VKK_UPDATE_MODE_DEFAULT,
	                                           VKK_BUFFER_USAGE_VERTEX,
	                                           4*sizeof(cc_vec4f_t),
	                                           XYUV);
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

	int multiply = 0;
	self->ub_multiply = vkk_engine_newBuffer(screen->engine,
	                                         VKK_UPDATE_MODE_DEFAULT,
	                                         VKK_BUFFER_USAGE_UNIFORM,
	                                         sizeof(int),
	                                         &multiply);
	if(self->ub_multiply == NULL)
	{
		goto fail_ub_multiply;
	}

	vkk_uniformAttachment_t ua_array_mvp[1] =
	{
		// layout(std140, set=0, binding=0) uniform uniformMvp
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.buffer  = self->ub_mvp
		},
	};

	self->us_mvp = vkk_engine_newUniformSet(screen->engine,
	                                        0, 1,
	                                        ua_array_mvp,
	                                        screen->usf0_mvp);
	if(self->us_mvp == NULL)
	{
		goto fail_us_mvp;
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

	vkk_uniformAttachment_t ua_array_multiply[1] =
	{
		// layout(std140, set=2, binding=0) uniform uniformMultiply
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.buffer  = self->ub_multiply
		},
	};

	self->us_multiply = vkk_engine_newUniformSet(screen->engine,
	                                             2, 1,
	                                             ua_array_multiply,
	                                             screen->usf2_multiply);
	if(self->us_multiply == NULL)
	{
		goto fail_us_multiply;
	}

	// success
	return self;

	// failure
	fail_us_multiply:
		vkk_engine_deleteUniformSet(screen->engine,
		                            &self->us_color);
	fail_us_color:
		vkk_engine_deleteUniformSet(screen->engine,
		                            &self->us_mvp);
	fail_us_mvp:
		vkk_engine_deleteBuffer(screen->engine,
		                        &self->ub_multiply);
	fail_ub_multiply:
		vkk_engine_deleteBuffer(screen->engine,
		                        &self->ub_color);
	fail_ub_color:
		vkk_engine_deleteBuffer(screen->engine,
		                        &self->vb_color_xyuv);
	fail_vb_color_xyuv:
		vkk_engine_deleteBuffer(screen->engine,
		                        &self->ub_mvp);
	fail_ub_mvp:
	fail_img:
		free(self->image_array);
	fail_image_array:
		vkui_widget_delete((vkui_widget_t**) &self);
	return NULL;
}

void vkui_sprite_delete(vkui_sprite_t** _self)
{
	assert(_self);

	vkui_sprite_t* self = *_self;
	if(self)
	{
		vkui_widget_t* widget = (vkui_widget_t*) self;
		vkui_screen_t* screen = widget->screen;

		vkk_engine_deleteUniformSet(screen->engine,
		                            &self->us_multiply);
		vkk_engine_deleteUniformSet(screen->engine,
		                            &self->us_color);
		vkk_engine_deleteUniformSet(screen->engine,
		                            &self->us_mvp);
		vkk_engine_deleteBuffer(screen->engine,
		                        &self->ub_multiply);
		vkk_engine_deleteBuffer(screen->engine,
		                        &self->ub_color);
		vkk_engine_deleteBuffer(screen->engine,
		                        &self->vb_color_xyuv);
		vkk_engine_deleteBuffer(screen->engine,
		                        &self->ub_mvp);
		free(self->image_array);
		vkui_widget_delete((vkui_widget_t**) _self);
	}
}

void vkui_sprite_select(vkui_sprite_t* self,
                        uint32_t index)
{
	assert(self);

	// check for invalid index
	if((index < 0) || (index >= self->count))
	{
		LOGW("invalid index=%i, count=%i", index,
		     self->count);
		return;
	}

	self->index = index;
}

void vkui_sprite_rotate(vkui_sprite_t* self, float theta)
{
	assert(self);

	self->theta = theta;
}
