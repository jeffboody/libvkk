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
#include <string.h>

#define LOG_TAG "vkk"
#include "../../libcc/math/cc_mat4f.h"
#include "../../libcc/cc_memory.h"
#include "../../libcc/cc_log.h"
#include "../vkk_ui.h"

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

static void
vkk_uiSprite_aspect(vkk_uiWidget_t* widget, float* ar)
{
	ASSERT(widget);

	vkk_uiSprite_t* self  = (vkk_uiSprite_t*) widget;
	vkk_image_t*    img21 = self->img21_array[self->index];

	uint32_t w;
	uint32_t h;
	uint32_t d;
	vkk_image_size(img21, &w, &h, &d);
	*ar = ((float) w)/((float) h);
}

static void vkk_uiSprite_draw(vkk_uiWidget_t* widget)
{
	ASSERT(widget);

	vkk_uiSprite_t* self   = (vkk_uiSprite_t*) widget;
	vkk_uiScreen_t* screen = widget->screen;
	vkk_image_t*    img21  = self->img21_array[self->index];

	vkk_uiScreen_bind(widget->screen, VKK_UI_SCREEN_BIND_IMAGE);

	int multiply = 1;
	if(vkk_image_format(img21) == VKK_IMAGE_FORMAT_R8)
	{
		multiply = 0;
	}
	vkk_renderer_updateBuffer(screen->renderer,
	                          self->ub20_multiply,
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
	vkk_uiScreen_sizef(screen, &w, &h);
	cc_mat4f_orthoVK(&pm, 1, 0.0f, w, h, 0.0f, 0.0f, 2.0f);
	cc_mat4f_translate(&mvm, 1, x + ww/2.0f, y + hh/2.0f, 0.0f);
	cc_mat4f_rotate(&mvm, 0, self->theta, 0.0f, 0.0f, 1.0f);
	cc_mat4f_scale(&mvm, 0, ww, hh, 1.0f);
	cc_mat4f_mulm_copy(&pm, &mvm, &mvp);
	vkk_renderer_updateBuffer(screen->renderer, self->ub00_mvp,
	                          sizeof(cc_mat4f_t),
	                          (const void*) &mvp);

	vkk_renderer_updateBuffer(screen->renderer,
	                          self->ub10_color,
	                          sizeof(cc_vec4f_t),
	                          (const void*) &self->color);

	vkk_uniformAttachment_t ua =
	{
		.binding = 1,
		.type    = VKK_UNIFORM_TYPE_IMAGE_REF,
		.image   = img21
	};

	vkk_renderer_updateUniformSetRefs(screen->renderer,
	                                  self->us2_multiplyImage,
	                                  1, &ua);

	vkk_uniformSet_t* us_array[] =
	{
		self->us0_mvp,
		self->us1_color,
		self->us2_multiplyImage,
		screen->us3_tricolor,
	};

	vkk_renderer_bindUniformSets(screen->renderer, 4,
	                             us_array);
	vkk_renderer_draw(screen->renderer, 4,
	                  1, &self->vb_color_xyuv);
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_uiSprite_t*
vkk_uiSprite_new(vkk_uiScreen_t* screen,
                 size_t wsize,
                 vkk_uiSpriteFn_t* sfn,
                 vkk_uiWidgetLayout_t* layout,
                 cc_vec4f_t* color,
                 const char** sprite_array)
{
	ASSERT(screen);
	ASSERT(sfn);
	ASSERT(layout);
	ASSERT(color);
	ASSERT(sprite_array);

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

	if((layout->wrapx == VKK_UI_WIDGET_WRAP_SHRINK) ||
	   (layout->wrapy == VKK_UI_WIDGET_WRAP_SHRINK))
	{
		LOGE("invalid wrapx=%i, wrapy=%i",
		     layout->wrapx, layout->wrapy);
		return NULL;
	}

	if(wsize == 0)
	{
		wsize = sizeof(vkk_uiSprite_t);
	}

	vkk_uiWidgetScroll_t scroll =
	{
		.scroll_bar = 0
	};

	vkk_uiWidgetFn_t fn =
	{
		.priv      = sfn->priv,
		.action_fn = sfn->action_fn,
		.aspect_fn = vkk_uiSprite_aspect,
		.click_fn  = sfn->click_fn,
		.draw_fn   = vkk_uiSprite_draw,
	};

	cc_vec4f_t clear =
	{
		.a = 0.0f
	};

	vkk_uiSprite_t* self;
	self = (vkk_uiSprite_t*)
	       vkk_uiWidget_new(screen, wsize, &clear,
	                        layout, &scroll, &fn);
	if(self == NULL)
	{
		return NULL;
	}

	self->count = count;
	self->index = 0;
	self->theta = 0.0f;

	cc_vec4f_copy(color, &self->color);

	self->img21_array = (vkk_image_t**)
	                    CALLOC(count,
	                           sizeof(vkk_image_t*));
	if(self->img21_array == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_img21_array;
	}

	int i;
	for(i = 0; i < count; ++i)
	{
		vkk_image_t* image;
		image = vkk_uiScreen_spriteImage(screen, sprite_array[i],
		                                 NULL);
		if(image == NULL)
		{
			goto fail_image;
		}
		self->img21_array[i] = image;
	}

	self->ub00_mvp = vkk_buffer_new(screen->engine,
	                                VKK_UPDATE_MODE_ASYNCHRONOUS,
	                                VKK_BUFFER_USAGE_UNIFORM,
	                                sizeof(cc_mat4f_t),
	                                NULL);
	if(self->ub00_mvp == NULL)
	{
		goto fail_ub00_mvp;
	}

	self->vb_color_xyuv = vkk_buffer_new(screen->engine,
	                                     VKK_UPDATE_MODE_ASYNCHRONOUS,
	                                     VKK_BUFFER_USAGE_VERTEX,
	                                     4*sizeof(cc_vec4f_t),
	                                     XYUV);
	if(self->vb_color_xyuv == NULL)
	{
		goto fail_vb_color_xyuv;
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

	int multiply = 0;
	self->ub20_multiply = vkk_buffer_new(screen->engine,
	                                     VKK_UPDATE_MODE_ASYNCHRONOUS,
	                                     VKK_BUFFER_USAGE_UNIFORM,
	                                     sizeof(int),
	                                     &multiply);
	if(self->ub20_multiply == NULL)
	{
		goto fail_ub20_multiply;
	}

	vkk_uniformAttachment_t ua_array_mvp[1] =
	{
		// layout(std140, set=0, binding=0) uniform uniformMvp
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.buffer  = self->ub00_mvp
		},
	};

	self->us0_mvp = vkk_uniformSet_new(screen->engine, 0, 1,
	                                  ua_array_mvp,
	                                  screen->usf0_mvp);
	if(self->us0_mvp == NULL)
	{
		goto fail_us0_mvp;
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

	vkk_uniformAttachment_t ua_array_multiply[1] =
	{
		// layout(std140, set=2, binding=0) uniform uniformMultiply
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.buffer  = self->ub20_multiply
		},
	};

	self->us2_multiplyImage = vkk_uniformSet_new(screen->engine,
	                                             2, 1,
	                                             ua_array_multiply,
	                                             screen->usf2_multiplyImage);
	if(self->us2_multiplyImage == NULL)
	{
		goto fail_us2_multiplyImage;
	}

	// success
	return self;

	// failure
	fail_us2_multiplyImage:
		vkk_uniformSet_delete(&self->us1_color);
	fail_us1_color:
		vkk_uniformSet_delete(&self->us0_mvp);
	fail_us0_mvp:
		vkk_buffer_delete(&self->ub20_multiply);
	fail_ub20_multiply:
		vkk_buffer_delete(&self->ub10_color);
	fail_ub10_color:
		vkk_buffer_delete(&self->vb_color_xyuv);
	fail_vb_color_xyuv:
		vkk_buffer_delete(&self->ub00_mvp);
	fail_ub00_mvp:
	fail_image:
		FREE(self->img21_array);
	fail_img21_array:
		vkk_uiWidget_delete((vkk_uiWidget_t**) &self);
	return NULL;
}

vkk_uiSprite_t*
vkk_uiSprite_newPageImage(vkk_uiScreen_t* screen,
                          vkk_uiSpriteFn_t* sfn,
                          const char** sprite_array)
{
	ASSERT(screen);
	ASSERT(sfn);
	ASSERT(sprite_array);

	vkk_uiWidgetLayout_t sprite_layout =
	{
		.anchor   = VKK_UI_WIDGET_ANCHOR_TC,
		.wrapx    = VKK_UI_WIDGET_WRAP_STRETCH_TEXT_VLARGE,
		.wrapy    = VKK_UI_WIDGET_WRAP_STRETCH_TEXT_VLARGE,
		.stretchx = 5.5f,
		.stretchy = 5.5f
	};

	cc_vec4f_t color;
	vkk_uiScreen_colorPageImage(screen, &color);

	return vkk_uiSprite_new(screen, 0, sfn,
	                        &sprite_layout, &color,
	                        sprite_array);
}

vkk_uiSprite_t*
vkk_uiSprite_newStatusIcon(vkk_uiScreen_t* screen,
                           vkk_uiSpriteFn_t* sfn,
                           const char** sprite_array)
{
	ASSERT(screen);
	ASSERT(sfn);
	ASSERT(sprite_array);

	vkk_uiWidgetLayout_t sprite_layout =
	{
		.border   = VKK_UI_WIDGET_BORDER_SMALL,
		.wrapx    = VKK_UI_WIDGET_WRAP_STRETCH_TEXT_VSMALL,
		.wrapy    = VKK_UI_WIDGET_WRAP_STRETCH_TEXT_VSMALL,
		.stretchx = 1.0f,
		.stretchy = 1.0f
	};

	cc_vec4f_t color;
	vkk_uiScreen_colorStatusIcon(screen, &color);

	return vkk_uiSprite_new(screen, 0, sfn,
	                        &sprite_layout, &color,
	                        sprite_array);
}

void vkk_uiSprite_delete(vkk_uiSprite_t** _self)
{
	ASSERT(_self);

	vkk_uiSprite_t* self = *_self;
	if(self)
	{
		vkk_uniformSet_delete(&self->us2_multiplyImage);
		vkk_uniformSet_delete(&self->us1_color);
		vkk_uniformSet_delete(&self->us0_mvp);
		vkk_buffer_delete(&self->ub20_multiply);
		vkk_buffer_delete(&self->ub10_color);
		vkk_buffer_delete(&self->vb_color_xyuv);
		vkk_buffer_delete(&self->ub00_mvp);
		FREE(self->img21_array);
		vkk_uiWidget_delete((vkk_uiWidget_t**) _self);
	}
}

void vkk_uiSprite_select(vkk_uiSprite_t* self,
                         uint32_t index)
{
	ASSERT(self);

	// check for invalid index
	if((index < 0) || (index >= self->count))
	{
		LOGW("invalid index=%i, count=%i", index,
		     self->count);
		return;
	}

	self->index = index;
}

void vkk_uiSprite_rotate(vkk_uiSprite_t* self, float theta)
{
	ASSERT(self);

	self->theta = theta;
}

void vkk_uiSprite_color(vkk_uiSprite_t* self,
                        cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	cc_vec4f_copy(color, &self->color);
}

void vkk_uiSprite_fill(vkk_uiSprite_t* self,
                       cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	vkk_uiWidget_t* base = &self->base;
	vkk_uiWidget_color(base, color);
}
