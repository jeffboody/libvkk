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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "vkk"
#include "../../libcc/math/cc_float.h"
#include "../../libcc/math/cc_mat4f.h"
#include "../../libcc/math/cc_vec2f.h"
#include "../../libcc/cc_memory.h"
#include "../../libcc/cc_log.h"
#include "../../libcc/cc_timestamp.h"
#include "../../libvkk/vkk_platform.h"
#include "../../libbfs/bfs_file.h"
#include "../../texgz/texgz_png.h"
#include "../../texgz/texgz_jpeg.h"
#include "../vkk_ui.h"

/***********************************************************
* protected                                                *
***********************************************************/

void vkk_uiScreen_sizei(vkk_uiScreen_t* self,
                        int* w, int* h)
{
	ASSERT(self);

	*w = self->w;
	*h = self->h;
}

void
vkk_uiScreen_sizef(vkk_uiScreen_t* self,
                   float* w, float* h)
{
	ASSERT(self);

	*w = (float) self->w;
	*h = (float) self->h;
}

int vkk_uiScreen_scalei(vkk_uiScreen_t* self)
{
	ASSERT(self);

	return self->scale;
}

float vkk_uiScreen_scalef(vkk_uiScreen_t* self)
{
	ASSERT(self);

	if(self->scale == VKK_UI_SCREEN_SCALE_XSMALL)
	{
		return 0.79f;
	}
	else if(self->scale == VKK_UI_SCREEN_SCALE_SMALL)
	{
		return 0.89f;
	}
	else if(self->scale == VKK_UI_SCREEN_SCALE_LARGE)
	{
		return 1.13f;
	}
	else if(self->scale == VKK_UI_SCREEN_SCALE_XLARGE)
	{
		return 1.27f;
	}
	return 1.0f;
}

void vkk_uiScreen_layoutDirty(vkk_uiScreen_t* self)
{
	ASSERT(self);

	self->layout_dirty = 1;
}

void
vkk_uiScreen_layoutBorder(vkk_uiScreen_t* self, int border,
                          float* hborder, float* vborder)
{
	ASSERT(self);
	ASSERT(hborder);
	ASSERT(vborder);

	*hborder = 0.0f;
	*vborder = 0.0f;

	float size = 0.25f*vkk_uiScreen_layoutText(self,
	                                           VKK_UI_TEXT_SIZE_MEDIUM);

	// horizontal
	if(border & VKK_UI_WIDGET_BORDER_HSMALL)
	{
		*hborder = 0.66f*size;
	}
	else if(border & VKK_UI_WIDGET_BORDER_HMEDIUM)
	{
		*hborder = size;
	}
	else if(border & VKK_UI_WIDGET_BORDER_HLARGE)
	{
		*hborder = 1.5f*size;
	}
	else if(border & VKK_UI_WIDGET_BORDER_HXLARGE)
	{
		*hborder = 2.0f*size;
	}

	// vertical
	if(border & VKK_UI_WIDGET_BORDER_VSMALL)
	{
		*vborder = 0.66f*size;
	}
	else if(border & VKK_UI_WIDGET_BORDER_VMEDIUM)
	{
		*vborder = size;
	}
	else if(border & VKK_UI_WIDGET_BORDER_VLARGE)
	{
		*vborder = 1.5f*size;
	}
	else if(border & VKK_UI_WIDGET_BORDER_VXLARGE)
	{
		*vborder = 2.0f*size;
	}
}

float
vkk_uiScreen_layoutText(vkk_uiScreen_t* self, int size)
{
	ASSERT(self);

	// default size is 24 px at density 1.0
	float sizef = 24.0f*self->density*vkk_uiScreen_scalef(self);
	if(size == VKK_UI_TEXT_SIZE_XSMALL)
	{
		return 0.45f*sizef;
	}
	else if(size == VKK_UI_TEXT_SIZE_SMALL)
	{
		return 0.66f*sizef;
	}
	else if(size == VKK_UI_TEXT_SIZE_LARGE)
	{
		return 1.5f*sizef;
	}
	return sizef;
}

int vkk_uiScreen_depthDirty(vkk_uiScreen_t* self,
                            vkk_uiWidget_t* widget)
{
	ASSERT(self);
	ASSERT(widget);

	cc_listIter_t* iter;
	iter = cc_list_head(self->depth_dirty);
	while(iter)
	{
		vkk_uiWidget_t* tmp;
		tmp = (vkk_uiWidget_t*) cc_list_peekIter(iter);

		cc_rect1f_t rect;
		if(cc_rect1f_intersect(vkk_uiWidget_rectScissor(widget),
		                       vkk_uiWidget_rectScissor(tmp),
		                       &rect))
		{
			return 1;
		}
	}

	return 0;
}

void vkk_uiScreen_depthMark(vkk_uiScreen_t* self,
                            vkk_uiWidget_t* widget)
{
	ASSERT(self);
	ASSERT(widget);

	cc_list_append(self->depth_dirty, NULL, widget);
}

void vkk_uiScreen_depthReset(vkk_uiScreen_t* self)
{
	ASSERT(self);

	cc_list_discard(self->depth_dirty);
}

void vkk_uiScreen_bind(vkk_uiScreen_t* self, int bind)
{
	ASSERT(self);

	if(bind == self->gp_bound)
	{
		return;
	}

	if(bind == VKK_UI_SCREEN_BIND_COLOR)
	{
		vkk_renderer_bindGraphicsPipeline(self->renderer,
		                                  self->gp_color);
	}
	else if(bind == VKK_UI_SCREEN_BIND_IMAGE)
	{
		vkk_renderer_bindGraphicsPipeline(self->renderer,
		                                  self->gp_image);
	}
	else if(bind == VKK_UI_SCREEN_BIND_TEXT)
	{
		vkk_renderer_bindGraphicsPipeline(self->renderer,
		                                  self->gp_text);
	}
	else if(bind == VKK_UI_SCREEN_BIND_TRICOLOR)
	{
		vkk_renderer_bindGraphicsPipeline(self->renderer,
		                                  self->gp_tricolor);
	}

	self->gp_bound = bind;
}

void
vkk_uiScreen_scissor(vkk_uiScreen_t* self,
                     cc_rect1f_t* rect)
{
	ASSERT(self);
	ASSERT(rect);

	vkk_renderer_scissor(self->renderer,
	                     (int32_t)  (rect->l + 0.5f),
	                     (int32_t)  (rect->t + 0.5f),
	                     (uint32_t) (rect->w + 0.5f),
	                     (uint32_t) (rect->h + 0.5f));
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_uiScreen_t*
vkk_uiScreen_new(size_t wsize,
                 vkk_engine_t* engine,
                 vkk_renderer_t* renderer,
                 const char* resource,
                 vkk_uiWidgetStyle_t* widget_style)
{
	ASSERT(engine);
	ASSERT(resource);
	ASSERT(widget_style);

	if(wsize == 0)
	{
		wsize = sizeof(vkk_uiScreen_t);
	}

	vkk_uiScreen_t* self;
	self = (vkk_uiScreen_t*) CALLOC(1, wsize);
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->engine       = engine;
	self->density      = 1.0f;
	self->scale        = VKK_UI_SCREEN_SCALE_MEDIUM;
	self->layout_dirty = 1;

	memcpy(&self->widget_style, widget_style,
	       sizeof(vkk_uiWidgetStyle_t));
	snprintf(self->resource, 256, "%s", resource);

	self->renderer = renderer;

	vkk_uniformBinding_t ub0_array[1] =
	{
		// layout(std140, set=0, binding=0) uniform uniformMvp
		{
			.binding  = 0,
			.type     = VKK_UNIFORM_TYPE_BUFFER,
			.stage    = VKK_STAGE_VS,
		},
	};

	self->usf0_mvp = vkk_uniformSetFactory_new(engine,
	                                           VKK_UPDATE_MODE_ASYNCHRONOUS,
	                                           1,
	                                           ub0_array);
	if(self->usf0_mvp == NULL)
	{
		goto fail_usf0_mvp;
	}

	vkk_uniformBinding_t ub1_array[1] =
	{
		// layout(std140, set=1, binding=0) uniform uniformColor
		{
			.binding  = 0,
			.type     = VKK_UNIFORM_TYPE_BUFFER,
			.stage    = VKK_STAGE_FS,
		},
	};

	self->usf1_color = vkk_uniformSetFactory_new(engine,
	                                             VKK_UPDATE_MODE_ASYNCHRONOUS,
	                                             1,
	                                             ub1_array);
	if(self->usf1_color == NULL)
	{
		goto fail_usf1_color;
	}

	vkk_uniformBinding_t ub2_array[2] =
	{
		// layout(std140, set=2, binding=0) uniform uniformMultiply
		{
			.binding  = 0,
			.type     = VKK_UNIFORM_TYPE_BUFFER,
			.stage    = VKK_STAGE_FS,
		},
		// layout(set=2, binding=1) uniform sampler2D image;
		{
			.binding  = 1,
			.type     = VKK_UNIFORM_TYPE_IMAGE_REF,
			.stage    = VKK_STAGE_FS,
			.si       =
			{
				.min_filter  = VKK_SAMPLER_FILTER_LINEAR,
				.mag_filter  = VKK_SAMPLER_FILTER_LINEAR,
				.mipmap_mode = VKK_SAMPLER_MIPMAP_MODE_NEAREST,
			}
		},
	};

	self->usf2_multiplyImage = vkk_uniformSetFactory_new(engine,
	                                                     VKK_UPDATE_MODE_ASYNCHRONOUS,
	                                                     2,
	                                                     ub2_array);
	if(self->usf2_multiplyImage == NULL)
	{
		goto fail_usf2_multiplyImage;
	}

	vkk_uniformBinding_t ub3_array[4] =
	{
		// layout(std140, set=3, binding=0) uniform uniformColor0
		{
			.binding  = 0,
			.type     = VKK_UNIFORM_TYPE_BUFFER,
			.stage    = VKK_STAGE_FS,
		},
		// layout(std140, set=3, binding=1) uniform uniformColor1
		{
			.binding  = 1,
			.type     = VKK_UNIFORM_TYPE_BUFFER,
			.stage    = VKK_STAGE_FS,
		},
		// layout(std140, set=3, binding=2) uniform uniformColor2
		{
			.binding  = 2,
			.type     = VKK_UNIFORM_TYPE_BUFFER,
			.stage    = VKK_STAGE_FS,
		},
		// layout(std140, set=3, binding=3) uniform uniformAb
		{
			.binding  = 3,
			.type     = VKK_UNIFORM_TYPE_BUFFER,
			.stage    = VKK_STAGE_FS,
		},
	};

	self->usf3_tricolor = vkk_uniformSetFactory_new(engine,
	                                                VKK_UPDATE_MODE_ASYNCHRONOUS,
	                                                4,
	                                                ub3_array);
	if(self->usf3_tricolor == NULL)
	{
		goto fail_usf3_tricolor;
	}

	vkk_uniformSetFactory_t* usf_array[4] =
	{
		self->usf0_mvp,
		self->usf1_color,
		self->usf2_multiplyImage,
		self->usf3_tricolor,
	};

	self->pl = vkk_pipelineLayout_new(engine, 4, usf_array);
	if(self->pl == NULL)
	{
		goto fail_pl;
	}

	vkk_vertexBufferInfo_t vbi =
	{
		.location   = 0,
		.components = 4,
		.format     = VKK_VERTEX_FORMAT_FLOAT
	};

	vkk_graphicsPipelineInfo_t gpi_color =
	{
		.renderer          = self->renderer,
		.pl                = self->pl,
		.vs                = "vkk/ui/shaders/default_vert.spv",
		.fs                = "vkk/ui/shaders/color_frag.spv",
		.vb_count          = 1,
		.vbi               = &vbi,
		.primitive         = VKK_PRIMITIVE_TRIANGLE_FAN,
		.primitive_restart = 0,
		.cull_back         = 0,
		.depth_test        = 0,
		.depth_write       = 0,
		.blend_mode        = VKK_BLEND_MODE_TRANSPARENCY
	};

	self->gp_color = vkk_graphicsPipeline_new(engine,
	                                          &gpi_color);
	if(self->gp_color == NULL)
	{
		goto fail_gp_color;
	}

	vkk_graphicsPipelineInfo_t gpi_image =
	{
		.renderer          = self->renderer,
		.pl                = self->pl,
		.vs                = "vkk/ui/shaders/default_vert.spv",
		.fs                = "vkk/ui/shaders/image_frag.spv",
		.vb_count          = 1,
		.vbi               = &vbi,
		.primitive         = VKK_PRIMITIVE_TRIANGLE_FAN,
		.primitive_restart = 0,
		.cull_back         = 0,
		.depth_test        = 0,
		.depth_write       = 0,
		.blend_mode        = VKK_BLEND_MODE_TRANSPARENCY
	};

	self->gp_image = vkk_graphicsPipeline_new(engine,
	                                          &gpi_image);
	if(self->gp_image == NULL)
	{
		goto fail_gp_image;
	}

	// text requires triangles
	gpi_image.primitive = VKK_PRIMITIVE_TRIANGLE_LIST;
	self->gp_text = vkk_graphicsPipeline_new(engine,
	                                         &gpi_image);
	if(self->gp_text == NULL)
	{
		goto fail_gp_text;
	}

	vkk_graphicsPipelineInfo_t gpi_tricolor =
	{
		.renderer          = self->renderer,
		.pl                = self->pl,
		.vs                = "vkk/ui/shaders/default_vert.spv",
		.fs                = "vkk/ui/shaders/tricolor_frag.spv",
		.vb_count          = 1,
		.vbi               = &vbi,
		.primitive         = VKK_PRIMITIVE_TRIANGLE_FAN,
		.primitive_restart = 0,
		.cull_back         = 0,
		.depth_test        = 0,
		.depth_write       = 0,
		.blend_mode        = VKK_BLEND_MODE_TRANSPARENCY
	};

	self->gp_tricolor = vkk_graphicsPipeline_new(engine,
	                                             &gpi_tricolor);
	if(self->gp_tricolor == NULL)
	{
		goto fail_gp_tricolor;
	}

	self->ub00_mvp = vkk_buffer_new(engine,
	                                VKK_UPDATE_MODE_ASYNCHRONOUS,
	                                VKK_BUFFER_USAGE_UNIFORM,
	                                sizeof(cc_mat4f_t), NULL);
	if(self->ub00_mvp == NULL)
	{
		goto fail_ub00_mvp;
	}

	cc_vec4f_t clear = { .r=0.0f, .g=0.0f, .b=0.0f, .a=0.0f };
	self->ub10_color = vkk_buffer_new(engine,
	                                  VKK_UPDATE_MODE_STATIC,
	                                  VKK_BUFFER_USAGE_UNIFORM,
	                                  sizeof(cc_vec4f_t),
	                                  &clear);
	if(self->ub10_color == NULL)
	{
		goto fail_ub10_color;
	}

	int multiply = 0;
	self->ub20_multiply = vkk_buffer_new(engine,
	                                     VKK_UPDATE_MODE_STATIC,
	                                     VKK_BUFFER_USAGE_UNIFORM,
	                                     sizeof(int),
	                                     &multiply);
	if(self->ub20_multiply == NULL)
	{
		goto fail_ub20_multiply;
	}

	unsigned char pixel21[] = { 0, 0, 0, 255 };
	self->img21 = vkk_image_new(engine,
	                            1, 1, 1,
	                            VKK_IMAGE_FORMAT_RGBA8888,
	                            1, VKK_STAGE_FS,
	                            (const void*) pixel21);
	if(self->img21 == NULL)
	{
		goto fail_img21;
	}

	self->ub30_color0 = vkk_buffer_new(engine,
	                                   VKK_UPDATE_MODE_STATIC,
	                                   VKK_BUFFER_USAGE_UNIFORM,
	                                   sizeof(cc_vec4f_t),
	                                   &clear);
	if(self->ub30_color0 == NULL)
	{
		goto fail_ub30_color0;
	}

	self->ub31_color1 = vkk_buffer_new(engine,
	                                   VKK_UPDATE_MODE_STATIC,
	                                   VKK_BUFFER_USAGE_UNIFORM,
	                                   sizeof(cc_vec4f_t),
	                                   &clear);
	if(self->ub31_color1 == NULL)
	{
		goto fail_ub31_color1;
	}

	self->ub32_color2 = vkk_buffer_new(engine,
	                                   VKK_UPDATE_MODE_STATIC,
	                                   VKK_BUFFER_USAGE_UNIFORM,
	                                   sizeof(cc_vec4f_t),
	                                   &clear);
	if(self->ub32_color2 == NULL)
	{
		goto fail_ub32_color2;
	}

	cc_vec2f_t ab = { .x=0.0f, .y=0.0f };
	self->ub33_ab = vkk_buffer_new(engine,
	                               VKK_UPDATE_MODE_STATIC,
	                               VKK_BUFFER_USAGE_UNIFORM,
	                               sizeof(cc_vec2f_t),
	                               &ab);
	if(self->ub33_ab == NULL)
	{
		goto fail_ub33_ab;
	}

	vkk_uniformAttachment_t ua_array0[] =
	{
		// layout(std140, set=0, binding=0) uniform uniformMvp
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.buffer  = self->ub00_mvp
		}
	};

	self->us0_mvp = vkk_uniformSet_new(engine, 0, 1,
	                                   ua_array0,
	                                   self->usf0_mvp);
	if(self->us0_mvp == NULL)
	{
		goto fail_us0_mvp;
	}

	vkk_uniformAttachment_t ua_array1[] =
	{
		// layout(std140, set=1, binding=0) uniform uniformColor
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.buffer  = self->ub10_color
		}
	};

	self->us1_color = vkk_uniformSet_new(engine, 1, 1,
	                                     ua_array1,
	                                     self->usf1_color);
	if(self->us1_color == NULL)
	{
		goto fail_us1_color;
	}

	vkk_uniformAttachment_t ua_array2[] =
	{
		// layout(std140, set=2, binding=0) uniform uniformMultiply
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.buffer  = self->ub20_multiply
		},
	};

	self->us2_multiplyImage = vkk_uniformSet_new(engine, 2, 1,
	                                             ua_array2,
	                                             self->usf2_multiplyImage);
	if(self->us2_multiplyImage == NULL)
	{
		goto fail_us2_multiplyImage;
	}

	vkk_uniformAttachment_t ua_array3[] =
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

	self->us3_tricolor = vkk_uniformSet_new(engine, 3, 4,
	                                        ua_array3,
	                                        self->usf3_tricolor);
	if(self->us3_tricolor == NULL)
	{
		goto fail_us3_tricolor;
	}

	self->window_stack = cc_list_new();
	if(self->window_stack == NULL)
	{
		goto fail_window_stack;
	}

	self->depth_dirty = cc_list_new();
	if(self->depth_dirty == NULL)
	{
		goto fail_depth_dirty;
	}

	self->sprite_map = cc_map_new();
	if(self->sprite_map == NULL)
	{
		goto fail_sprite_map;
	}

	self->font_array[0] = vkk_uiFont_new(self, resource,
	                                     "vkk/ui/fonts/BarlowSemiCondensed-Regular-64.png",
	                                     "vkk/ui/fonts/BarlowSemiCondensed-Regular-64.xml");
	if(self->font_array[0] == NULL)
	{
		goto fail_font_array0;
	}

	self->font_array[1] = vkk_uiFont_new(self, resource,
	                                     "vkk/ui/fonts/BarlowSemiCondensed-Bold-64.png",
	                                     "vkk/ui/fonts/BarlowSemiCondensed-Bold-64.xml");
	if(self->font_array[1] == NULL)
	{
		goto fail_font_array1;
	}

	self->font_array[2] = vkk_uiFont_new(self, resource,
	                                     "vkk/ui/fonts/BarlowSemiCondensed-Medium-32.png",
	                                     "vkk/ui/fonts/BarlowSemiCondensed-Medium-32.xml");
	if(self->font_array[2] == NULL)
	{
		goto fail_font_array2;
	}

	self->map_text_vb = cc_multimap_new(NULL);
	if(self->map_text_vb == NULL)
	{
		goto fail_map_text_vb;
	}

	// success
	return self;

	// failure
	fail_map_text_vb:
		vkk_uiFont_delete(&self->font_array[2]);
	fail_font_array2:
		vkk_uiFont_delete(&self->font_array[1]);
	fail_font_array1:
		vkk_uiFont_delete(&self->font_array[0]);
	fail_font_array0:
		cc_map_delete(&self->sprite_map);
	fail_sprite_map:
		cc_list_delete(&self->depth_dirty);
	fail_depth_dirty:
		cc_list_delete(&self->window_stack);
	fail_window_stack:
		vkk_uniformSet_delete(&self->us3_tricolor);
	fail_us3_tricolor:
		vkk_uniformSet_delete(&self->us2_multiplyImage);
	fail_us2_multiplyImage:
		vkk_uniformSet_delete(&self->us1_color);
	fail_us1_color:
		vkk_uniformSet_delete(&self->us0_mvp);
	fail_us0_mvp:
		vkk_buffer_delete(&self->ub33_ab);
	fail_ub33_ab:
		vkk_buffer_delete(&self->ub32_color2);
	fail_ub32_color2:
		vkk_buffer_delete(&self->ub31_color1);
	fail_ub31_color1:
		vkk_buffer_delete(&self->ub30_color0);
	fail_ub30_color0:
		vkk_image_delete(&self->img21);
	fail_img21:
		vkk_buffer_delete(&self->ub20_multiply);
	fail_ub20_multiply:
		vkk_buffer_delete(&self->ub10_color);
	fail_ub10_color:
		vkk_buffer_delete(&self->ub00_mvp);
	fail_ub00_mvp:
		vkk_graphicsPipeline_delete(&self->gp_tricolor);
	fail_gp_tricolor:
		vkk_graphicsPipeline_delete(&self->gp_text);
	fail_gp_text:
		vkk_graphicsPipeline_delete(&self->gp_image);
	fail_gp_image:
		vkk_graphicsPipeline_delete(&self->gp_color);
	fail_gp_color:
		vkk_pipelineLayout_delete(&self->pl);
	fail_pl:
		vkk_uniformSetFactory_delete(&self->usf3_tricolor);
	fail_usf3_tricolor:
		vkk_uniformSetFactory_delete(&self->usf2_multiplyImage);
	fail_usf2_multiplyImage:
		vkk_uniformSetFactory_delete(&self->usf1_color);
	fail_usf1_color:
		vkk_uniformSetFactory_delete(&self->usf0_mvp);
	fail_usf0_mvp:
		FREE(self);
	return NULL;
}

void vkk_uiScreen_delete(vkk_uiScreen_t** _self)
{
	ASSERT(_self);

	vkk_uiScreen_t* self = *_self;
	if(self)
	{
		cc_multimapIter_t  mmiterator;
		cc_multimapIter_t* mmiter;
		mmiter = cc_multimap_head(self->map_text_vb, &mmiterator);
		while(mmiter)
		{
			vkk_buffer_t* vb;
			vb = (vkk_buffer_t*)
			     cc_multimap_remove(self->map_text_vb, &mmiter);
			vkk_buffer_delete(&vb);
		}
		cc_multimap_delete(&self->map_text_vb);

		vkk_uiFont_delete(&self->font_array[2]);
		vkk_uiFont_delete(&self->font_array[1]);
		vkk_uiFont_delete(&self->font_array[0]);

		cc_mapIter_t* miter;
		miter = cc_map_head(self->sprite_map);
		while(miter)
		{
			vkk_image_t* image;
			image = (vkk_image_t*)
			        cc_map_remove(self->sprite_map, &miter);
			vkk_image_delete(&image);
		}
		cc_map_delete(&self->sprite_map);

		cc_list_discard(self->depth_dirty);
		cc_list_delete(&self->depth_dirty);
		cc_list_discard(self->window_stack);
		cc_list_delete(&self->window_stack);
		vkk_uniformSet_delete(&self->us3_tricolor);
		vkk_uniformSet_delete(&self->us2_multiplyImage);
		vkk_uniformSet_delete(&self->us1_color);
		vkk_uniformSet_delete(&self->us0_mvp);
		vkk_buffer_delete(&self->ub33_ab);
		vkk_buffer_delete(&self->ub32_color2);
		vkk_buffer_delete(&self->ub31_color1);
		vkk_buffer_delete(&self->ub30_color0);
		vkk_image_delete(&self->img21);
		vkk_buffer_delete(&self->ub20_multiply);
		vkk_buffer_delete(&self->ub10_color);
		vkk_buffer_delete(&self->ub00_mvp);
		vkk_graphicsPipeline_delete(&self->gp_tricolor);
		vkk_graphicsPipeline_delete(&self->gp_text);
		vkk_graphicsPipeline_delete(&self->gp_image);
		vkk_graphicsPipeline_delete(&self->gp_color);
		vkk_pipelineLayout_delete(&self->pl);
		vkk_uniformSetFactory_delete(&self->usf3_tricolor);
		vkk_uniformSetFactory_delete(&self->usf2_multiplyImage);
		vkk_uniformSetFactory_delete(&self->usf1_color);
		vkk_uniformSetFactory_delete(&self->usf0_mvp);
		FREE(self);
		*_self = NULL;
	}
}

#ifdef LOG_DEBUG
static const char*
vkk_uiScreen_actionStateString(int action_state)
{
	if(action_state == VKK_UI_SCREEN_ACTION_STATE_UP)
	{
		return "UP";
	}
	else if(action_state == VKK_UI_SCREEN_ACTION_STATE_DOWN)
	{
		return "DOWN";
	}
	else if(action_state == VKK_UI_SCREEN_ACTION_STATE_DRAG)
	{
		return "DRAG";
	}
	else if(action_state == VKK_UI_SCREEN_ACTION_STATE_ROTATE)
	{
		return "ROTATE";
	}
	else if(action_state == VKK_UI_SCREEN_ACTION_STATE_SCALE)
	{
		return "SCALE";
	}

	return "NULL";
}
#endif

static int
vkk_uiScreen_actionDetect1(vkk_uiScreen_t* self,
                           vkk_platformEvent_t* event)
{
	ASSERT(self);
	ASSERT(event);

	vkk_platformEventAction_t* e = &event->action;

	cc_vec2f_t* acoord0 = &self->action_coord0;
	cc_vec2f_t* ecoord0 = &e->coord[0];

	// check for a sizable motion
	float dx0    = ecoord0->x - acoord0->x;
	float dy0    = ecoord0->y - acoord0->y;
	float delta  = fabsf(dx0) + fabsf(dy0);
	int   sz     = (self->w > self->h) ? self->w : self->h;
	float thresh = 0.03f*((float) sz);
	if(delta < thresh)
	{
		return VKK_UI_SCREEN_ACTION_STATE_DOWN;
	}

	return VKK_UI_SCREEN_ACTION_STATE_DRAG;
}

static int
vkk_uiScreen_actionDetect2(vkk_uiScreen_t* self,
                           vkk_platformEvent_t* event)
{
	ASSERT(self);
	ASSERT(event);

	vkk_platformEventAction_t* e = &event->action;

	cc_vec2f_t* acoord0 = &self->action_coord0;
	cc_vec2f_t* acoord1 = &self->action_coord1;
	cc_vec2f_t* ecoord0 = &e->coord[0];
	cc_vec2f_t* ecoord1 = &e->coord[1];

	// check for a sizable motion
	float dx0    = ecoord0->x - acoord0->x;
	float dy0    = ecoord0->y - acoord0->y;
	float dx1    = ecoord1->x - acoord1->x;
	float dy1    = ecoord1->y - acoord1->y;
	float delta  = fabsf(dx0) + fabsf(dy0) +
	               fabsf(dx1) + fabsf(dy1);
	int   sz     = (self->w > self->h) ? self->w : self->h;
	float thresh = 0.03f*((float) sz);
	if(delta < thresh)
	{
		return VKK_UI_SCREEN_ACTION_STATE_DOWN;
	}

	// compute vectors
	cc_vec2f_t va;
	cc_vec2f_t ve;
	cc_vec2f_subv_copy(acoord1, acoord0, &va);
	cc_vec2f_subv_copy(ecoord1, ecoord0, &ve);

	// compute vector properties
	// a.e = |a||e|cos(rotation)
	float dot  = cc_vec2f_dot(&va, &ve);
	float axe  = cc_vec2f_cross(&va, &ve);
	float sign = (axe > 0.0f) ? -1.0f : 1.0f;

	// compute rotation
	// clamp for numerical stability
	float angle = 0.0f;
	float ma    = cc_vec2f_mag(&va);
	float me    = cc_vec2f_mag(&ve);
	if((ma < 1.0f) || (me < 1.0f))
	{
		// ignore
	}
	else
	{
		float cosang;
		cosang = cc_clamp(dot/(ma*me), -1.0f, 1.0f);
		angle  = sign*cc_rad2deg(acosf(cosang));
	}

	// compute scale
	float scale = cc_vec2f_mag(&ve)/cc_vec2f_mag(&va);

	// compute centers
	cc_vec2f_t ca;
	cc_vec2f_t ce;
	cc_vec2f_muls(&va, 0.5f);
	cc_vec2f_muls(&ve, 0.5f);
	cc_vec2f_addv_copy(acoord0, &va, &ca);
	cc_vec2f_addv_copy(ecoord0, &ve, &ce);

	// compute drag
	cc_vec2f_t drag;
	cc_vec2f_subv_copy(&ce, &ca, &drag);

	// determine the action state
	if(fabs(angle) >= 5.0f)
	{
		return VKK_UI_SCREEN_ACTION_STATE_ROTATE;
	}
	else if((scale > 1.05f) || (scale < 1.0f/1.05f))
	{
		return VKK_UI_SCREEN_ACTION_STATE_SCALE;
	}
	else if(cc_vec2f_mag(&drag) > thresh)
	{
		return VKK_UI_SCREEN_ACTION_STATE_DRAG;
	}

	return VKK_UI_SCREEN_ACTION_STATE_DOWN;
}

void
vkk_uiScreen_eventAction(vkk_uiScreen_t* self,
                         vkk_platformEvent_t* event)
{
	ASSERT(self);
	ASSERT(event);

	vkk_platformEventAction_t* e = &event->action;

	vkk_uiWindow_t* top = vkk_uiScreen_windowPeek(self);
	if(top == NULL)
	{
		// ignore
		return;
	}

	int         astate  = self->action_state;
	int         acount  = self->action_count;
	double      ats     = self->action_ts;
	cc_vec2f_t* acoord0 = &self->action_coord0;
	cc_vec2f_t* acoord1 = &self->action_coord1;
	int         ecount  = e->count;
	double      ets     = event->ts;
	cc_vec2f_t* ecoord0 = &e->coord[0];
	cc_vec2f_t* ecoord1 = &e->coord[1];
	int         nstate  = astate;
	int         ncount  = acount;
	double      nts     = ats;

	vkk_uiWidget_t* awidget = self->action_widget;
	vkk_uiWidget_t* nwidget = awidget;

	vkk_platformEventType_e etype = event->type;

	vkk_uiWidgetActionInfo_t info =
	{
		.action = VKK_UI_WIDGET_ACTION_UP,
		.count  = ecount,
		.ts     = ets,
		.dt     = ets - ats,
		.coord0 =
		{
			.x = ecoord0->x,
			.y = ecoord0->y,
		},
		.coord1 =
		{
			.x = ecoord1->x,
			.y = ecoord1->y,
		},
	};

	if((astate == VKK_UI_SCREEN_ACTION_STATE_UP)     &&
	   (etype == VKK_PLATFORM_EVENTTYPE_ACTION_DOWN) &&
	   (ecount <= 2))
	{
		// <UP:UP>
		// <UP:DOWN>

		info.action = VKK_UI_WIDGET_ACTION_DOWN;

		// try to direct actions to action_bar popup
		// or fall back to the top window
		if(self->action_bar)
		{
			vkk_uiWidget_t* ab;
			ab = (vkk_uiWidget_t*) self->action_bar;
			nwidget = vkk_uiWidget_action(ab, &info);
			if(nwidget == NULL)
			{
				// dismiss the action_bar popup
				vkk_uiScreen_popupSet(self, NULL, NULL);
				nwidget = vkk_uiWidget_action(&top->base, &info);
			}
		}
		else
		{
			nwidget = vkk_uiWidget_action(&top->base, &info);
		}

		if(nwidget)
		{
			// update state
			nstate = VKK_UI_SCREEN_ACTION_STATE_DOWN;
			ncount = ecount;
			nts    = ets;
			LOGD("<%s:%s> ncount=%i",
			     vkk_uiScreen_actionStateString(astate),
			     vkk_uiScreen_actionStateString(nstate),
			     ncount);
			LOGD("acount=%i, acoord0=%f,%f, acoord1=%f,%f",
			     acount, acoord0->x, acoord0->y,
			     acoord1->x, acoord1->y);
			LOGD("ecount=%i, ecoord0=%f,%f, ecoord1=%f,%f",
			     ecount, ecoord0->x, ecoord0->y,
			     ecoord1->x, ecoord1->y);
		}
	}
	else if((astate == VKK_UI_SCREEN_ACTION_STATE_DOWN) &&
	        ((etype  == VKK_PLATFORM_EVENTTYPE_ACTION_DOWN) ||
	         (etype  == VKK_PLATFORM_EVENTTYPE_ACTION_MOVE)) &&
	        (ecount <= 2) && (ecount != acount))
	{
		// <DOWN:DOWN>

		// update state
		ncount = ecount;
		nts    = ets;
		LOGD("<%s:%s> ncount=%i",
		     vkk_uiScreen_actionStateString(astate),
		     vkk_uiScreen_actionStateString(nstate),
		     ncount);
		LOGD("acount=%i, acoord0=%f,%f, acoord1=%f,%f",
		     acount, acoord0->x, acoord0->y,
		     acoord1->x, acoord1->y);
		LOGD("ecount=%i, ecoord0=%f,%f, ecoord1=%f,%f",
		     ecount, ecoord0->x, ecoord0->y,
		     ecoord1->x, ecoord1->y);
	}
	else if((astate == VKK_UI_SCREEN_ACTION_STATE_DOWN)    &&
	        (etype  == VKK_PLATFORM_EVENTTYPE_ACTION_MOVE) &&
	        (ecount == 1))
	{
		// <DOWN:DOWN>
		// <DOWN:DRAG>

		// determine and update state
		int next = vkk_uiScreen_actionDetect1(self, event);
		if(next != VKK_UI_SCREEN_ACTION_STATE_DOWN)
		{
			nstate = next;
		}
		ncount = ecount;
		nts    = ets;
		LOGD("<%s:%s> ncount=%i",
		     vkk_uiScreen_actionStateString(astate),
		     vkk_uiScreen_actionStateString(nstate),
		     ncount);
		LOGD("acount=%i, acoord0=%f,%f, acoord1=%f,%f",
		     acount, acoord0->x, acoord0->y,
		     acoord1->x, acoord1->y);
		LOGD("ecount=%i, ecoord0=%f,%f, ecoord1=%f,%f",
		     ecount, ecoord0->x, ecoord0->y,
		     ecoord1->x, ecoord1->y);
	}
	else if((astate == VKK_UI_SCREEN_ACTION_STATE_DOWN)    &&
	        (etype  == VKK_PLATFORM_EVENTTYPE_ACTION_MOVE) &&
	        (ecount == 2))
	{
		// <DOWN:DOWN>
		// <DOWN:DRAG>
		// <DOWN:ROTATE>
		// <DOWN:SCALE>

		// determine and update state
		int next = vkk_uiScreen_actionDetect2(self, event);
		if(next != VKK_UI_SCREEN_ACTION_STATE_DOWN)
		{
			nstate = next;
		}
		ncount = ecount;
		nts    = ets;
		LOGD("<%s:%s> ncount=%i",
		     vkk_uiScreen_actionStateString(astate),
		     vkk_uiScreen_actionStateString(nstate),
		     ncount);
		LOGD("acount=%i, acoord0=%f,%f, acoord1=%f,%f",
		     acount, acoord0->x, acoord0->y,
		     acoord1->x, acoord1->y);
		LOGD("ecount=%i, ecoord0=%f,%f, ecoord1=%f,%f",
		     ecount, ecoord0->x, ecoord0->y,
		     ecoord1->x, ecoord1->y);
	}
	else if((astate == VKK_UI_SCREEN_ACTION_STATE_DRAG) &&
	        (etype  == VKK_PLATFORM_EVENTTYPE_ACTION_MOVE) &&
	        (acount == ecount) && (acount == 1))
	{
		// <DRAG:DRAG>

		info.action = VKK_UI_WIDGET_ACTION_DRAG;
		cc_vec2f_subv_copy(ecoord0, acoord0, &info.drag);

		double dt = ets - ats;
		if(awidget && vkk_uiWidget_action(awidget, &info))
		{
			// skip dragging/scrolling
		}
		else if(dt > 0.001)
		{
			// dragging/scrolling
			self->action_dragv.x = info.drag.x/((float) dt);
			self->action_dragv.y = info.drag.y/((float) dt);
			self->layout_dirty   = 1;
		}

		// update state
		nts = ets;
		LOGD("<%s:%s> ncount=%i, drag=%f,%f",
		     vkk_uiScreen_actionStateString(astate),
		     vkk_uiScreen_actionStateString(nstate),
		     ncount, info.drag.x, info.drag.y);
		LOGD("acount=%i, acoord0=%f,%f, acoord1=%f,%f",
		     acount, acoord0->x, acoord0->y,
		     acoord1->x, acoord1->y);
		LOGD("ecount=%i, ecoord0=%f,%f, ecoord1=%f,%f",
		     ecount, ecoord0->x, ecoord0->y,
		     ecoord1->x, ecoord1->y);
	}
	else if((astate == VKK_UI_SCREEN_ACTION_STATE_DRAG)    &&
	        (etype  == VKK_PLATFORM_EVENTTYPE_ACTION_MOVE) &&
	        (acount == ecount) && (acount == 2))
	{
		// <DRAG:DRAG>

		// compute vectors
		cc_vec2f_t va;
		cc_vec2f_t ve;
		cc_vec2f_subv_copy(acoord1, acoord0, &va);
		cc_vec2f_subv_copy(ecoord1, ecoord0, &ve);

		// compute centers
		cc_vec2f_t ca;
		cc_vec2f_t ce;
		cc_vec2f_muls(&va, 0.5f);
		cc_vec2f_muls(&ve, 0.5f);
		cc_vec2f_addv_copy(acoord0, &va, &ca);
		cc_vec2f_addv_copy(ecoord0, &ve, &ce);

		info.action = VKK_UI_WIDGET_ACTION_DRAG;
		cc_vec2f_subv_copy(&ce, &ca, &info.drag);

		// ignore return value
		if(awidget)
		{
			vkk_uiWidget_action(awidget, &info);
		}

		// update state
		nts = ets;
		LOGD("<%s:%s> ncount=%i, drag=%f,%f",
		     vkk_uiScreen_actionStateString(astate),
		     vkk_uiScreen_actionStateString(nstate),
		     ncount, info.drag.x, info.drag.y);
		LOGD("acount=%i, acoord0=%f,%f, acoord1=%f,%f",
		     acount, acoord0->x, acoord0->y,
		     acoord1->x, acoord1->y);
		LOGD("ecount=%i, ecoord0=%f,%f, ecoord1=%f,%f",
		     ecount, ecoord0->x, ecoord0->y,
		     ecoord1->x, ecoord1->y);
	}
	else if((astate == VKK_UI_SCREEN_ACTION_STATE_ROTATE)  &&
	        (etype  == VKK_PLATFORM_EVENTTYPE_ACTION_MOVE) &&
	        (acount == ecount) && (acount == 2))
	{
		// <ROTATE:ROTATE>

		// compute vectors
		cc_vec2f_t va;
		cc_vec2f_t ve;
		cc_vec2f_subv_copy(acoord1, acoord0, &va);
		cc_vec2f_subv_copy(ecoord1, ecoord0, &ve);

		// compute vector properties
		// a.e = |a||e|cos(rotation)
		float dot  = cc_vec2f_dot(&va, &ve);
		float axe  = cc_vec2f_cross(&va, &ve);
		float sign = (axe > 0.0f) ? -1.0f : 1.0f;

		// compute rotation
		// clamp for numerical stability
		float angle = 0.0f;
		float ma    = cc_vec2f_mag(&va);
		float me    = cc_vec2f_mag(&ve);
		if((ma < 1.0f) || (me < 1.0f))
		{
			// ignore
		}
		else
		{
			float cosang;
			cosang = cc_clamp(dot/(ma*me), -1.0f, 1.0f);
			angle  = sign*cc_rad2deg(acosf(cosang));
		}

		info.action = VKK_UI_WIDGET_ACTION_ROTATE;
		info.angle  = angle;

		// ignore return value
		if(awidget)
		{
			vkk_uiWidget_action(awidget, &info);
		}

		// update state
		nts = ets;
		LOGD("<%s:%s> ncount=%i, angle=%f",
		     vkk_uiScreen_actionStateString(astate),
		     vkk_uiScreen_actionStateString(nstate),
		     ncount, info.angle);
		LOGD("acount=%i, acoord0=%f,%f, acoord1=%f,%f",
		     acount, acoord0->x, acoord0->y,
		     acoord1->x, acoord1->y);
		LOGD("ecount=%i, ecoord0=%f,%f, ecoord1=%f,%f",
		     ecount, ecoord0->x, ecoord0->y,
		     ecoord1->x, ecoord1->y);
	}
	else if((astate == VKK_UI_SCREEN_ACTION_STATE_SCALE)   &&
	        (etype  == VKK_PLATFORM_EVENTTYPE_ACTION_MOVE) &&
	        (acount == ecount) && (acount == 2))
	{
		// <SCALE:SCALE>

		// compute vectors
		cc_vec2f_t va;
		cc_vec2f_t ve;
		cc_vec2f_subv_copy(acoord1, acoord0, &va);
		cc_vec2f_subv_copy(ecoord1, ecoord0, &ve);

		info.action = VKK_UI_WIDGET_ACTION_SCALE;
		info.scale  = cc_vec2f_mag(&ve)/cc_vec2f_mag(&va);

		// ignore return value
		if(awidget)
		{
			vkk_uiWidget_action(awidget, &info);
		}

		// update state
		nts = ets;
		LOGD("<%s:%s> ncount=%i, scale=%f",
		     vkk_uiScreen_actionStateString(astate),
		     vkk_uiScreen_actionStateString(nstate),
		     ncount, info.scale);
		LOGD("acount=%i, acoord0=%f,%f, acoord1=%f,%f",
		     acount, acoord0->x, acoord0->y,
		     acoord1->x, acoord1->y);
		LOGD("ecount=%i, ecoord0=%f,%f, ecoord1=%f,%f",
		     ecount, ecoord0->x, ecoord0->y,
		     ecoord1->x, ecoord1->y);
	}
	else if(astate != VKK_UI_SCREEN_ACTION_STATE_UP)
	{
		// <DOWN:CLICK> (count == 1)
		// <ELSE:UP>

		info.action = VKK_UI_WIDGET_ACTION_UP;
		if((astate == VKK_UI_SCREEN_ACTION_STATE_DOWN) &&
		   (acount == 1))
		{
			info.action = VKK_UI_WIDGET_ACTION_CLICK;
		}

		if(awidget &&
		   vkk_uiWidget_action(awidget, &info) &&
		   (info.action == VKK_UI_WIDGET_ACTION_CLICK))
		{
			vkk_engine_platformCmd(self->engine,
			                       VKK_PLATFORM_CMD_PLAY_CLICK);
		}

		// update state
		nstate  = VKK_UI_SCREEN_ACTION_STATE_UP;
		ncount  = 0;
		nts     = ets;
		nwidget = NULL;
		LOGD("<%s:%s> ncount=%i",
		     vkk_uiScreen_actionStateString(astate),
		     vkk_uiScreen_actionStateString(nstate),
		     ncount);
		LOGD("acount=%i, acoord0=%f,%f, acoord1=%f,%f",
		     acount, acoord0->x, acoord0->y,
		     acoord1->x, acoord1->y);
		LOGD("ecount=%i, ecoord0=%f,%f, ecoord1=%f,%f",
		     ecount, ecoord0->x, ecoord0->y,
		     ecoord1->x, ecoord1->y);
	}

	// update coords
	// DRAG/ROTATE/SCALE transitions require a certain thresh
	// to detect a state change so we don't update the coords
	// unless the count changes
	if((nstate != VKK_UI_SCREEN_ACTION_STATE_DOWN) ||
	   (acount != ncount))
	{
		acoord0->x = ecoord0->x;
		acoord0->y = ecoord0->y;
		acoord1->x = ecoord1->x;
		acoord1->y = ecoord1->y;
	}

	// update state
	self->action_state  = nstate;
	self->action_count  = ncount;
	self->action_ts     = nts;
	self->action_widget = nwidget;
}

void
vkk_uiScreen_eventContentRect(vkk_uiScreen_t* self,
                              vkk_platformEventContentRect_t* e)
{
	ASSERT(self);
	ASSERT(e);

	self->content_rect_top    = e->t;
	self->content_rect_left   = e->l;
	self->content_rect_width  = e->r - e->l;
	self->content_rect_height = e->b - e->t;
	self->layout_dirty        = 1;
}

void
vkk_uiScreen_eventDensity(vkk_uiScreen_t* self,
                          float density)
{
	ASSERT(self);

	if(self->density == density)
	{
		return;
	}

	self->density      = density;
	self->layout_dirty = 1;
}

int vkk_uiScreen_eventKey(vkk_uiScreen_t* self,
                          vkk_platformEventKey_t* e)
{
	ASSERT(self);
	ASSERT(e);

	if(e->keycode == VKK_PLATFORM_KEYCODE_ESCAPE)
	{
		if(self->action_bar || self->action_popup)
		{
			vkk_uiScreen_popupSet(self, NULL, NULL);
			return 1;
		}

		return vkk_uiScreen_windowPop(self);
	}

	if(self->focus_widget == NULL)
	{
		return 0;
	}

	return vkk_uiWidget_keyPress(self->focus_widget,
	                             e->keycode, e->meta);
}

vkk_uiWindow_t*
vkk_uiScreen_windowPeek(vkk_uiScreen_t* self)
{
	ASSERT(self);

	return (vkk_uiWindow_t*)
	       cc_list_peekTail(self->window_stack);
}

void vkk_uiScreen_windowPush(vkk_uiScreen_t* self,
                             vkk_uiWindow_t* window)
{
	ASSERT(self);
	ASSERT(window);

	vkk_uiScreen_popupSet(self, NULL, NULL);

	if(vkk_uiScreen_windowPeek(self) == window)
	{
		return;
	}

	self->layout_dirty = 1;

	if(window)
	{
		cc_list_append(self->window_stack, NULL,
		               (const void*) window);

		// reset scroll bar
		vkk_uiWidget_scrollTop((vkk_uiWidget_t*) window);

		if(window->focus)
		{
			vkk_engine_platformCmd(self->engine,
			                       VKK_PLATFORM_CMD_SOFTKEY_SHOW);
			vkk_uiScreen_focus(self, window->focus);
			return;
		}
	}

	// default focus state
	vkk_engine_platformCmd(self->engine,
	                       VKK_PLATFORM_CMD_SOFTKEY_HIDE);
	vkk_uiScreen_focus(self, NULL);
}

int vkk_uiScreen_windowPop(vkk_uiScreen_t* self)
{
	ASSERT(self);

	cc_listIter_t* iter;
	iter = cc_list_tail(self->window_stack);
	if((iter == NULL) ||
	   (cc_list_size(self->window_stack) <= 1))
	{
		return 0;
	}

	self->layout_dirty = 1;

	cc_list_remove(self->window_stack, &iter);

	// default focus state
	vkk_engine_platformCmd(self->engine,
	                       VKK_PLATFORM_CMD_SOFTKEY_HIDE);
	vkk_uiScreen_focus(self, NULL);

	return 1;
}

void vkk_uiScreen_windowReset(vkk_uiScreen_t* self,
                              vkk_uiWindow_t* window)
{
	// window may be NULL
	ASSERT(self);

	vkk_uiScreen_popupSet(self, NULL, NULL);

	// check if window already active
	if(vkk_uiScreen_windowPeek(self) == window)
	{
		return;
	}

	self->layout_dirty = 1;

	// reset window stack
	cc_list_discard(self->window_stack);

	if(window)
	{
		vkk_uiScreen_windowPush(self, window);
	}
}

void
vkk_uiScreen_popupGet(vkk_uiScreen_t* self,
                      vkk_uiActionBar_t** _action_bar,
                      vkk_uiActionPopup_t** _action_popup)
{
	ASSERT(self);
	ASSERT(_action_bar);
	ASSERT(_action_popup);

	*_action_bar   = self->action_bar;
	*_action_popup = self->action_popup;
}

void
vkk_uiScreen_popupSet(vkk_uiScreen_t* self,
                      vkk_uiActionBar_t* action_bar,
                      vkk_uiActionPopup_t* action_popup)
{
	// action_bar and action_popup may be NULL
	ASSERT(self);

	if((self->action_bar   == action_bar) &&
	   (self->action_popup == action_popup))
	{
		self->action_bar   = NULL;
		self->action_popup = NULL;
	}
	else
	{
		self->action_bar   = action_bar;
		self->action_popup = action_popup;
	}
}

void vkk_uiScreen_detach(vkk_uiScreen_t* self,
                         vkk_uiWidget_t* widget)
{
	ASSERT(self);
	ASSERT(widget);

	if(self->focus_widget == widget)
	{
		self->focus_widget = NULL;
	}
	else if(self->action_widget == widget)
	{
		self->action_widget = NULL;
	}
}

void
vkk_uiScreen_focus(vkk_uiScreen_t* self,
                   vkk_uiWidget_t* focus)
{
	// focus may be NULL
	ASSERT(self);

	self->focus_widget = focus;
}

void vkk_uiScreen_rescale(vkk_uiScreen_t* self, int scale)
{
	ASSERT(self);

	if(self->scale == scale)
	{
		return;
	}

	if((scale >= VKK_UI_SCREEN_SCALE_XSMALL) &&
	   (scale <= VKK_UI_SCREEN_SCALE_XLARGE))
	{
		self->scale        = scale;
		self->layout_dirty = 1;
	}
}

void vkk_uiScreen_draw(vkk_uiScreen_t* self)
{
	ASSERT(self);

	uint32_t uw;
	uint32_t uh;
	vkk_renderer_surfaceSize(self->renderer,
	                         &uw, &uh);
	if((uw != self->w) || (uh != self->h))
	{
		self->w = uw;
		self->h = uh;

		self->layout_dirty = 1;
	}

	vkk_uiWindow_t* top = vkk_uiScreen_windowPeek(self);
	if(top == NULL)
	{
		// ignore
		return;
	}

	vkk_uiScreen_depthReset(self);
	vkk_uiWidget_refresh(&top->base);

	// dragging/scrolling
	float w  = (float) self->w;
	float h  = (float) self->h;
	float dt = 1.0f/60.0f;
	if((self->action_dragv.x != 0.0f) ||
	   (self->action_dragv.y != 0.0f))
	{
		float x  = self->action_coord0.x;
		float y  = self->action_coord0.y;
		float vx = self->action_dragv.x;
		float vy = self->action_dragv.y;

		// clamp the speed to be proportional to the range
		float range  = 4.0f*sqrtf(w*w + h*h);
		float speed1 = sqrtf(vx*vx + vy*vy);
		float drag   = 1.0f*range*dt;
		if(speed1 > range)
		{
			vx *= range/speed1;
			vy *= range/speed1;
			speed1 = range;
		}

		vkk_uiWidget_drag(&top->base,
		                  x, y, vx*dt, vy*dt);

		// update the speed
		if((speed1 > drag) && (speed1 > 0.1f))
		{
			float speed2 = speed1 - drag;
			float coef   = speed2/speed1;
			self->action_dragv.x *= coef;
			self->action_dragv.y *= coef;
		}
		else
		{
			self->action_dragv.x = 0.0f;
			self->action_dragv.y = 0.0f;
		}

		self->layout_dirty = 1;
	}

	if(self->layout_dirty)
	{
		cc_rect1f_t clip =
		{
			.t = 0.0f,
			.l = 0.0f,
			.w = w,
			.h = h,
		};

		// override the clip with content_rect
		if((self->content_rect_width  > 0)  &&
		   (self->content_rect_height > 0)  &&
		   (self->content_rect_width  <= w) &&
		   (self->content_rect_height <= h))
		{
			clip.t = self->content_rect_top;
			clip.l = self->content_rect_left;
			clip.w = self->content_rect_width;
			clip.h = self->content_rect_height;
		}

		float layout_w = clip.w;
		float layout_h = clip.h;
		vkk_uiWidget_layoutSize(&top->base,
		                        &layout_w, &layout_h);
		vkk_uiWidget_layoutXYClip(&top->base, clip.l,
		                          clip.t, &clip, 1, 1);
		self->layout_dirty = 0;
	}

	cc_mat4f_t mvp;
	cc_mat4f_orthoVK(&mvp, 1, 0.0f, w, h, 0.0f, 0.0f, 2.0f);
	vkk_renderer_updateBuffer(self->renderer, self->ub00_mvp,
	                          sizeof(cc_mat4f_t),
	                          (const void*) &mvp);

	vkk_renderer_viewport(self->renderer,
	                      0.0f, 0.0f, (float) w, (float) h);
	vkk_renderer_scissor(self->renderer,
	                     0, 0, w, h);
	vkk_uiWidget_draw(&top->base);
	vkk_uiScreen_bind(self, VKK_UI_SCREEN_BIND_NONE);
}

void vkk_uiScreen_colorPageItem(vkk_uiScreen_t* self,
                                cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	// https://material.io/design/color/dark-theme.html
	vkk_uiWidgetStyle_t* s = &self->widget_style;
	cc_vec4f_t*          c = &s->color_text;
	cc_vec4f_load(color, c->r, c->g, c->b, 0.6f);
}

void vkk_uiScreen_colorPageHeading(vkk_uiScreen_t* self,
                                   cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	// https://material.io/design/color/dark-theme.html
	vkk_uiWidgetStyle_t* s = &self->widget_style;
	cc_vec4f_t*          c = &s->color_text;
	cc_vec4f_load(color, c->r, c->g, c->b, 0.87f);
}

void vkk_uiScreen_colorPageImage(vkk_uiScreen_t* self,
                                 cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	cc_vec4f_load(color, 1.0f, 1.0f, 1.0f, 1.0f);
}

void vkk_uiScreen_colorPageLink(vkk_uiScreen_t* self,
                                cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	// https://material.io/design/color/dark-theme.html
	vkk_uiWidgetStyle_t* s = &self->widget_style;
	cc_vec4f_t*          c = &s->color_primary;
	cc_vec4f_load(color, c->r, c->g, c->b, 0.6f);
}

void vkk_uiScreen_colorPageEntry(vkk_uiScreen_t* self,
                                 cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	// https://material.io/design/color/dark-theme.html
	vkk_uiWidgetStyle_t* s = &self->widget_style;
	cc_vec4f_t*          c = &s->color_secondary;
	cc_vec4f_load(color, c->r, c->g, c->b, 0.38f);
}

void vkk_uiScreen_colorBanner(vkk_uiScreen_t* self,
                              cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	// https://material.io/design/color/dark-theme.html
	vkk_uiWidgetStyle_t* s = &self->widget_style;
	cc_vec4f_t*          c = &s->color_secondary;
	cc_vec4f_load(color, c->r, c->g, c->b, 1.0f);
}

void vkk_uiScreen_colorBannerText(vkk_uiScreen_t* self,
                                  cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	// https://material.io/design/color/dark-theme.html
	vkk_uiWidgetStyle_t* s = &self->widget_style;
	cc_vec4f_t*          c = &s->color_primary;
	cc_vec4f_load(color, c->r, c->g, c->b, 0.87f);
}

void vkk_uiScreen_colorActionIcon0(vkk_uiScreen_t* self,
                                   cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	// https://material.io/design/color/dark-theme.html
	vkk_uiWidgetStyle_t* s = &self->widget_style;
	cc_vec4f_t*          c = &s->color_text;
	cc_vec4f_load(color, c->r, c->g, c->b, 0.87f);
}

void vkk_uiScreen_colorActionIcon1(vkk_uiScreen_t* self,
                                   cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	// https://material.io/design/color/dark-theme.html
	vkk_uiWidgetStyle_t* s = &self->widget_style;
	cc_vec4f_t*          c = &s->color_primary;
	cc_vec4f_load(color, c->r, c->g, c->b, 0.87f);
}

void vkk_uiScreen_colorStatusIcon(vkk_uiScreen_t* self,
                                  cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	// https://material.io/design/color/dark-theme.html
	vkk_uiWidgetStyle_t* s = &self->widget_style;
	cc_vec4f_t*          c = &s->color_text;
	cc_vec4f_load(color, c->r, c->g, c->b, 0.87f);
}

void vkk_uiScreen_colorFooterItem(vkk_uiScreen_t* self,
                                  cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	// https://material.io/design/color/dark-theme.html
	vkk_uiWidgetStyle_t* s = &self->widget_style;
	cc_vec4f_t*          c = &s->color_primary;
	cc_vec4f_load(color, c->r, c->g, c->b, 0.87f);
}

void vkk_uiScreen_colorBackground(vkk_uiScreen_t* self,
                                  cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	// https://material.io/design/color/dark-theme.html
	vkk_uiWidgetStyle_t* s = &self->widget_style;
	cc_vec4f_t*          c = &s->color_background;
	cc_vec4f_load(color, c->r, c->g, c->b, 1.0f);
}

void vkk_uiScreen_colorScroll0(vkk_uiScreen_t* self,
                               cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	vkk_uiScreen_colorBackground(self, color);
}

void vkk_uiScreen_colorScroll1(vkk_uiScreen_t* self,
                               cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	// https://material.io/design/color/dark-theme.html
	vkk_uiWidgetStyle_t* s = &self->widget_style;
	cc_vec4f_t*          c = &s->color_primary;
	cc_vec4f_load(color, c->r, c->g, c->b, 0.87f);
}

vkk_uiFont_t*
vkk_uiScreen_font(vkk_uiScreen_t* self, int font_type)
{
	ASSERT(self);

	return self->font_array[font_type];
}

vkk_buffer_t*
vkk_uiScreen_textVb(vkk_uiScreen_t* self, size_t size,
                    vkk_buffer_t* vb)
{
	// vb may be NULL
	ASSERT(self);

	// return vb to the pool
	if(vb)
	{
		size_t sz = vkk_buffer_size(vb);
		if(cc_multimap_addp(self->map_text_vb,
		                    (const void*) vb,
		                    sizeof(size_t), &sz) == 0)
		{
			LOGE("buffer size=%i leaked", (int) sz);
			return NULL;
		}
	}

	// check for realloc buffer
	if(size == 0)
	{
		return NULL;
	}

	// reuse an existing buffer
	cc_multimapIter_t  miterator;
	cc_multimapIter_t* miter = &miterator;
	if(cc_multimap_findp(self->map_text_vb, miter,
	                     sizeof(size_t), &size))
	{
		vb = (vkk_buffer_t*)
		     cc_multimap_remove(self->map_text_vb, &miter);
		return vb;
	}

	// create a new buffer
	return vkk_buffer_new(self->engine,
	                      VKK_UPDATE_MODE_ASYNCHRONOUS,
	                      VKK_BUFFER_USAGE_VERTEX,
	                      size, NULL);
}

vkk_image_t*
vkk_uiScreen_spriteImage(vkk_uiScreen_t* self,
                         const char* name,
                         texgz_tex_t** _tex)
{
	// _tex may be NULL
	ASSERT(self);
	ASSERT(name);

	cc_mapIter_t* miter = cc_map_find(self->sprite_map, name);
	if(miter)
	{
		return (vkk_image_t*) cc_map_val(miter);;
	}

	bfs_file_t* bfs;
	bfs = bfs_file_open(self->resource, 1, BFS_MODE_RDONLY);
	if(bfs == NULL)
	{
		return NULL;
	}

	size_t size = 0;
	void*  data = NULL;
	if(bfs_file_blobGet(bfs, 0, name,
	                    &size, &data) == 0)
	{
		goto fail_get;
	}

	// check for empty data
	if(size == 0)
	{
		LOGE("invalid %s", name);
		goto fail_empty;
	}

	texgz_tex_t* tex = NULL;
	if(strstr(name, ".png"))
	{
		tex = texgz_png_importd(size, data);
	}
	else if(strstr(name, ".jpg"))
	{
		tex = texgz_jpeg_importd(size, data, TEXGZ_RGB);
	}
	else if(strstr(name, ".texz"))
	{
		tex = texgz_tex_importd(size, data);
	}

	if(tex == NULL)
	{
		goto fail_tex;
	}

	if((tex->type == TEXGZ_UNSIGNED_SHORT_4_4_4_4) &&
	   (tex->format == TEXGZ_RGBA))
	{
		texgz_tex_convert(tex, TEXGZ_UNSIGNED_BYTE, TEXGZ_RGBA);
	}
	else if((tex->type == TEXGZ_UNSIGNED_SHORT_5_6_5) &&
	        (tex->format == TEXGZ_RGB))
	{
		texgz_tex_convert(tex, TEXGZ_UNSIGNED_BYTE, TEXGZ_RGBA);
	}

	int image_format = -1;
	if(tex->type == TEXGZ_UNSIGNED_BYTE)
	{
		if((tex->format == TEXGZ_ALPHA) ||
		   (tex->format == TEXGZ_LUMINANCE))
		{
			image_format = VKK_IMAGE_FORMAT_R8;
		}
		else if(tex->format == TEXGZ_LUMINANCE_ALPHA)
		{
			image_format = VKK_IMAGE_FORMAT_RG88;
		}
		else if(tex->format == TEXGZ_RGB)
		{
			image_format = VKK_IMAGE_FORMAT_RGB888;
		}
		else if(tex->format == TEXGZ_RGBA)
		{
			image_format = VKK_IMAGE_FORMAT_RGBA8888;
		}
	}

	if(image_format < 0)
	{
		LOGE("invalid name=%s, type=0x%X, format=0x%X",
		     name, tex->type, tex->format);
		goto fail_format;
	}

	vkk_imageCaps_t caps;
	vkk_engine_imageCaps(self->engine, image_format, &caps);
	if((caps.texture == 0) || (caps.filter_linear == 0))
	{
		// try to fall back to RGBA8888
		image_format = VKK_IMAGE_FORMAT_RGBA8888;
		vkk_engine_imageCaps(self->engine, image_format, &caps);
		if((caps.texture == 0) ||
		   (caps.filter_linear == 0))
		{
			LOGW("unsupported format=%i, texture=%i, filter_linear=%i",
			     (int) image_format, (int) caps.texture,
			     (int) caps.filter_linear);
			goto fail_format;
		}

		int format_rgba = TEXGZ_RGBA;
		if(tex->format == TEXGZ_LUMINANCE_ALPHA)
		{
			format_rgba = TEXGZ_RG00;
		}

		if(texgz_tex_convert(tex, TEXGZ_UNSIGNED_BYTE,
		                     format_rgba) == 0)
		{
			goto fail_format;
		}
	}

	if((tex->width  != tex->stride) ||
	   (tex->height != tex->vstride))
	{
		LOGE("invalid name=%s, width=%i, height=%i, stride=%i, vstride=%i",
		     name, tex->width, tex->height, tex->stride, tex->vstride);
		goto fail_size;
	}

	vkk_image_t* image;
	image = vkk_image_new(self->engine,
	                      tex->width, tex->height, 1,
	                      image_format, 0, VKK_STAGE_FS,
	                      tex->pixels);
	if(image == NULL)
	{
		goto fail_image;
	}

	if(cc_map_add(self->sprite_map, (const void*) image,
	              name) == NULL)
	{
		goto fail_add;
	}

	if(_tex)
	{
		*_tex = tex;
	}
	else
	{
		texgz_tex_delete(&tex);
	}

	FREE(data);
	bfs_file_close(&bfs);

	// success
	return image;

	// failure
	fail_add:
		vkk_image_delete(&image);
	fail_image:
	fail_size:
	fail_format:
		texgz_tex_delete(&tex);
	fail_tex:
	fail_empty:
		FREE(data);
	fail_get:
		bfs_file_close(&bfs);
	return NULL;
}
