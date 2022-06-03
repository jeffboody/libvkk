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

#define LOG_TAG "vkui"
#include "../../libcc/math/cc_mat4f.h"
#include "../../libcc/math/cc_vec2f.h"
#include "../../libcc/cc_memory.h"
#include "../../libcc/cc_log.h"
#include "../../libbfs/bfs_file.h"
#include "../../texgz/texgz_png.h"
#include "../../texgz/texgz_jpeg.h"
#include "vkui_screen.h"
#include "vkui_widget.h"

/***********************************************************
* protected                                                *
***********************************************************/

void vkui_screen_sizei(vkui_screen_t* self, int* w, int* h)
{
	ASSERT(self);

	*w = self->w;
	*h = self->h;
}

void
vkui_screen_sizef(vkui_screen_t* self, float* w, float* h)
{
	ASSERT(self);

	*w = (float) self->w;
	*h = (float) self->h;
}

int vkui_screen_scalei(vkui_screen_t* self)
{
	ASSERT(self);

	return self->scale;
}

float vkui_screen_scalef(vkui_screen_t* self)
{
	ASSERT(self);

	if(self->scale == VKUI_SCREEN_SCALE_XSMALL)
	{
		return 0.79f;
	}
	else if(self->scale == VKUI_SCREEN_SCALE_SMALL)
	{
		return 0.89f;
	}
	else if(self->scale == VKUI_SCREEN_SCALE_LARGE)
	{
		return 1.13f;
	}
	else if(self->scale == VKUI_SCREEN_SCALE_XLARGE)
	{
		return 1.27f;
	}
	return 1.0f;
}

void vkui_screen_dirty(vkui_screen_t* self)
{
	ASSERT(self);

	self->dirty = 1;
}

void
vkui_screen_layoutBorder(vkui_screen_t* self, int border,
                         float* hborder, float* vborder)
{
	ASSERT(self);
	ASSERT(hborder);
	ASSERT(vborder);

	*hborder = 0.0f;
	*vborder = 0.0f;

	float size = 0.25f*vkui_screen_layoutText(self,
	                                          VKUI_TEXT_SIZE_MEDIUM);

	// horizontal
	if(border & VKUI_WIDGET_BORDER_HSMALL)
	{
		*hborder = 0.66f*size;
	}
	else if(border & VKUI_WIDGET_BORDER_HMEDIUM)
	{
		*hborder = size;
	}
	else if(border & VKUI_WIDGET_BORDER_HLARGE)
	{
		*hborder = 1.5f*size;
	}

	// vertical
	if(border & VKUI_WIDGET_BORDER_VSMALL)
	{
		*vborder = 0.66f*size;
	}
	else if(border & VKUI_WIDGET_BORDER_VMEDIUM)
	{
		*vborder = size;
	}
	else if(border & VKUI_WIDGET_BORDER_VLARGE)
	{
		*vborder = 1.5f*size;
	}
}

float vkui_screen_layoutText(vkui_screen_t* self, int size)
{
	ASSERT(self);

	// default size is 24 px at density 1.0
	float sizef = 24.0f*self->density*vkui_screen_scalef(self);
	if(size == VKUI_TEXT_SIZE_XSMALL)
	{
		return 0.45f*sizef;
	}
	else if(size == VKUI_TEXT_SIZE_SMALL)
	{
		return 0.66f*sizef;
	}
	else if(size == VKUI_TEXT_SIZE_LARGE)
	{
		return 1.5f*sizef;
	}
	return sizef;
}

void vkui_screen_bind(vkui_screen_t* self, int bind)
{
	ASSERT(self);

	if(bind == self->gp_bound)
	{
		return;
	}

	if(bind == VKUI_SCREEN_BIND_COLOR)
	{
		vkk_renderer_bindGraphicsPipeline(self->renderer,
		                                  self->gp_color);
	}
	else if(bind == VKUI_SCREEN_BIND_IMAGE)
	{
		vkk_renderer_bindGraphicsPipeline(self->renderer,
		                                  self->gp_image);
	}
	else if(bind == VKUI_SCREEN_BIND_TEXT)
	{
		vkk_renderer_bindGraphicsPipeline(self->renderer,
		                                  self->gp_text);
	}
	else if(bind == VKUI_SCREEN_BIND_TRICOLOR)
	{
		vkk_renderer_bindGraphicsPipeline(self->renderer,
		                                  self->gp_tricolor);
	}

	self->gp_bound = bind;
}

void
vkui_screen_scissor(vkui_screen_t* self, cc_rect1f_t* rect)
{
	ASSERT(self);
	ASSERT(rect);

	vkk_renderer_scissor(self->renderer,
	                     (uint32_t) (rect->l + 0.5f),
	                     (uint32_t) (rect->t + 0.5f),
	                     (uint32_t) (rect->w + 0.5f),
	                     (uint32_t) (rect->h + 0.5f));
}

void vkui_screen_playClick(vkui_screen_t* self)
{
	ASSERT(self);

	self->clicked = 1;
}

/***********************************************************
* public                                                   *
***********************************************************/

vkui_screen_t*
vkui_screen_new(vkk_engine_t* engine,
                vkk_renderer_t* renderer,
                const char* resource,
                void* sound_fx,
                vkui_screen_playClickFn playClick,
                vkui_widgetStyle_t* widget_style)
{
	ASSERT(engine);
	ASSERT(resource);
	ASSERT(sound_fx);
	ASSERT(playClick);
	ASSERT(widget_style);

	vkui_screen_t* self;
	self = (vkui_screen_t*) CALLOC(1, sizeof(vkui_screen_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->engine        = engine;
	self->density       = 1.0f;
	self->scale         = VKUI_SCREEN_SCALE_MEDIUM;
	self->dirty         = 1;
	self->pointer_state = VKUI_WIDGET_POINTER_UP;
	self->sound_fx      = sound_fx;
	self->playClick     = playClick;

	memcpy(&self->widget_style, widget_style,
	       sizeof(vkui_widgetStyle_t));
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
		.vs                = "vkui/shaders/default_vert.spv",
		.fs                = "vkui/shaders/color_frag.spv",
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
		.vs                = "vkui/shaders/default_vert.spv",
		.fs                = "vkui/shaders/image_frag.spv",
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
		.vs                = "vkui/shaders/default_vert.spv",
		.fs                = "vkui/shaders/tricolor_frag.spv",
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

	self->sprite_map = cc_map_new();
	if(self->sprite_map == NULL)
	{
		goto fail_sprite_map;
	}

	self->font_array[0] = vkui_font_new(self, resource,
	                                    "vkui/fonts/BarlowSemiCondensed-Regular-64.png",
	                                    "vkui/fonts/BarlowSemiCondensed-Regular-64.xml");
	if(self->font_array[0] == NULL)
	{
		goto fail_font_array0;
	}

	self->font_array[1] = vkui_font_new(self, resource,
	                                    "vkui/fonts/BarlowSemiCondensed-Bold-64.png",
	                                    "vkui/fonts/BarlowSemiCondensed-Bold-64.xml");
	if(self->font_array[1] == NULL)
	{
		goto fail_font_array1;
	}

	self->font_array[2] = vkui_font_new(self, resource,
	                                    "vkui/fonts/BarlowSemiCondensed-Medium-32.png",
	                                    "vkui/fonts/BarlowSemiCondensed-Medium-32.xml");
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
		vkui_font_delete(&self->font_array[2]);
	fail_font_array2:
		vkui_font_delete(&self->font_array[1]);
	fail_font_array1:
		vkui_font_delete(&self->font_array[0]);
	fail_font_array0:
		cc_map_delete(&self->sprite_map);
	fail_sprite_map:
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

void vkui_screen_delete(vkui_screen_t** _self)
{
	ASSERT(_self);

	vkui_screen_t* self = *_self;
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

		vkui_font_delete(&self->font_array[2]);
		vkui_font_delete(&self->font_array[1]);
		vkui_font_delete(&self->font_array[0]);

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

vkui_widget_t*
vkui_screen_top(vkui_screen_t* self, vkui_widget_t* top)
{
	ASSERT(self);

	if(self->top_widget == top)
	{
		return NULL;
	}

	vkui_widget_t* prev = self->top_widget;
	self->top_widget    = top;
	self->dirty         = 1;
	return prev;
}

void vkui_screen_contentRect(vkui_screen_t* self,
                             int t, int l,
                             int b, int r)
{
	ASSERT(self);

	self->content_rect_top    = t;
	self->content_rect_left   = l;
	self->content_rect_width  = r - l;
	self->content_rect_height = b - t;

	self->dirty = 1;
}

void
vkui_screen_focus(vkui_screen_t* self, vkui_widget_t* focus)
{
	// focus may be NULL
	ASSERT(self);

	self->focus_widget = focus;
}

void
vkui_screen_move(vkui_screen_t* self, vkui_widget_t* move)
{
	// move may be NULL
	ASSERT(self);

	self->move_widget = move;
}

void vkui_screen_resize(vkui_screen_t* self, int w, int h)
{
	ASSERT(self);

	if((self->w == w) && (self->h == h))
	{
		return;
	}

	self->w     = w;
	self->h     = h;
	self->dirty = 1;
}

void vkui_screen_density(vkui_screen_t* self, float density)
{
	ASSERT(self);

	if(self->density == density)
	{
		return;
	}

	self->density = density;
	self->dirty   = 1;
}

void vkui_screen_rescale(vkui_screen_t* self, int scale)
{
	ASSERT(self);

	if(self->scale == scale)
	{
		return;
	}

	if((scale >= VKUI_SCREEN_SCALE_XSMALL) &&
	   (scale <= VKUI_SCREEN_SCALE_XLARGE))
	{
		self->scale = scale;
		self->dirty = 1;
	}
}

int vkui_screen_pointerDown(vkui_screen_t* self,
                            float x, float y, double t0)
{
	ASSERT(self);

	if((self->top_widget == NULL) ||
	   (self->pointer_state != VKUI_WIDGET_POINTER_UP))
	{
		// ignore
		return 0;
	}

	if(vkui_widget_click(self->top_widget,
	                     VKUI_WIDGET_POINTER_DOWN, x, y))
	{
		self->pointer_state = VKUI_WIDGET_POINTER_DOWN;
		self->pointer_x0    = x;
		self->pointer_y0    = y;
		self->pointer_t0    = t0;
		self->pointer_vx    = 0.0f;
		self->pointer_vy    = 0.0f;
		return 1;
	}

	return 0;
}

int vkui_screen_pointerUp(vkui_screen_t* self,
                          float x, float y, double t0)
{
	ASSERT(self);

	int touch = self->pointer_state != VKUI_WIDGET_POINTER_UP;
	if(self->move_widget)
	{
		vkui_widget_click(self->move_widget,
		                  VKUI_WIDGET_POINTER_UP, x, y);
	}
	else if(self->top_widget &&
	        (self->pointer_state == VKUI_WIDGET_POINTER_DOWN))
	{
		vkui_widget_click(self->top_widget,
		                  VKUI_WIDGET_POINTER_UP, x, y);
	}
	self->pointer_state = VKUI_WIDGET_POINTER_UP;

	vkui_screen_move(self, NULL);

	return touch;
}

int vkui_screen_pointerMove(vkui_screen_t* self,
                            float x, float y, double t0)
{
	ASSERT(self);

	if((self->top_widget == NULL) ||
	   (self->pointer_state == VKUI_WIDGET_POINTER_UP))
	{
		// ignore
		return 0;
	}

	float dx = x - self->pointer_x0;
	float dy = y - self->pointer_y0;
	if(self->pointer_state == VKUI_WIDGET_POINTER_DOWN)
	{
		// reject small motions
		float d = sqrtf(dx*dx + dy*dy);
		float s = 0.2f*vkui_screen_layoutText(self,
		                                      VKUI_TEXT_SIZE_MEDIUM);
		if(d < s)
		{
			// ignore
			return 1;
		}

		// initialize move state
		self->pointer_state = VKUI_WIDGET_POINTER_MOVE;
		self->pointer_x0    = x;
		self->pointer_y0    = y;
		self->pointer_t0    = t0;
		self->pointer_vx    = 0.0f;
		self->pointer_vy    = 0.0f;
		return 1;
	}

	// ignore events with less than 8ms time delta
	float dt = (float) (t0 - self->pointer_t0);
	if(self->move_widget)
	{
		vkui_widget_click(self->move_widget,
		                  VKUI_WIDGET_POINTER_MOVE, x, y);
	}
	else if(dt >= 0.008f)
	{
		// update the move state
		self->pointer_x0 = x;
		self->pointer_y0 = y;
		self->pointer_t0 = t0;
		self->pointer_vx = dx/dt;
		self->pointer_vy = dy/dt;
		self->dirty      = 1;
	}

	return 1;
}

int vkui_screen_keyPress(vkui_screen_t* self,
                         int keycode, int meta)
{
	ASSERT(self);

	if(self->focus_widget == NULL)
	{
		return 0;
	}

	return vkui_widget_keyPress(self->focus_widget,
	                            keycode, meta);
}

void vkui_screen_draw(vkui_screen_t* self)
{
	ASSERT(self);

	uint32_t uw;
	uint32_t uh;
	vkk_renderer_surfaceSize(self->renderer,
	                         &uw, &uh);
	if((uw != self->w) || (uh != self->h))
	{
		self->w     = uw;
		self->h     = uh;
		self->dirty = 1;
	}

	vkui_widget_t* top = self->top_widget;
	if(top == NULL)
	{
		// ignore
		return;
	}

	vkui_widget_refresh(top);

	// dragging
	float w  = (float) self->w;
	float h  = (float) self->h;
	float dt = 1.0f/60.0f;
	if((self->pointer_vx != 0.0f) ||
	   (self->pointer_vy != 0.0f))
	{
		float x  = self->pointer_x0;
		float y  = self->pointer_y0;
		float vx = self->pointer_vx;
		float vy = self->pointer_vy;

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

		// TODO - change drag to return status for
		// bump animation and to minimize dirty updates
		vkui_widget_drag(self->top_widget,
		                 x, y, vx*dt, vy*dt);

		// update the speed
		if((speed1 > drag) && (speed1 > 0.1f))
		{
			float speed2 = speed1 - drag;
			float coef   = speed2/speed1;
			self->pointer_vx *= coef;
			self->pointer_vy *= coef;
		}
		else
		{
			self->pointer_vx = 0.0f;
			self->pointer_vy = 0.0f;
		}

		self->dirty = 1;
	}

	if(self->dirty)
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
		vkui_widget_layoutSize(top, &layout_w, &layout_h);
		vkui_widget_layoutXYClip(top, clip.l, clip.t, &clip, 1, 1);
		self->dirty = 0;
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
	vkui_widget_draw(self->top_widget);
	vkui_screen_bind(self, VKUI_SCREEN_BIND_NONE);

	// play sound fx
	if(self->clicked)
	{
		vkui_screen_playClickFn playClick = self->playClick;
		(*playClick)(self->sound_fx);
		self->clicked = 0;
	}
}

void vkui_screen_colorPageItem(vkui_screen_t* self,
                               cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	// https://material.io/design/color/dark-theme.html
	vkui_widgetStyle_t* s = &self->widget_style;
	cc_vec4f_t*         c = &s->color_text;
	cc_vec4f_load(color, c->r, c->g, c->b, 0.6f);
}

void vkui_screen_colorPageHeading(vkui_screen_t* self,
                                  cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	// https://material.io/design/color/dark-theme.html
	vkui_widgetStyle_t* s = &self->widget_style;
	cc_vec4f_t*         c = &s->color_text;
	cc_vec4f_load(color, c->r, c->g, c->b, 0.87f);
}

void vkui_screen_colorPageBanner(vkui_screen_t* self,
                                 cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	cc_vec4f_load(color, 1.0f, 1.0f, 1.0f, 1.0f);
}

void vkui_screen_colorPageEntry(vkui_screen_t* self,
                                cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	// https://material.io/design/color/dark-theme.html
	vkui_widgetStyle_t* s = &self->widget_style;
	cc_vec4f_t*         c = &s->color_secondary;
	cc_vec4f_load(color, c->r, c->g, c->b, 0.38f);
}

void vkui_screen_colorStatusIcon(vkui_screen_t* self,
                                 cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	// https://material.io/design/color/dark-theme.html
	vkui_widgetStyle_t* s = &self->widget_style;
	cc_vec4f_t*         c = &s->color_text;
	cc_vec4f_load(color, c->r, c->g, c->b, 0.87f);
}

void vkui_screen_colorFooterItem(vkui_screen_t* self,
                                 cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	// https://material.io/design/color/dark-theme.html
	vkui_widgetStyle_t* s = &self->widget_style;
	cc_vec4f_t*         c = &s->color_primary;
	cc_vec4f_load(color, c->r, c->g, c->b, 0.87f);
}

void vkui_screen_colorBackground(vkui_screen_t* self,
                                 cc_vec4f_t* color)
{
	ASSERT(self);
	ASSERT(color);

	// https://material.io/design/color/dark-theme.html
	vkui_widgetStyle_t* s = &self->widget_style;
	cc_vec4f_t*         c = &s->color_background;
	cc_vec4f_load(color, c->r, c->g, c->b, 1.0f);
}

vkui_font_t*
vkui_screen_font(vkui_screen_t* self, int font_type)
{
	ASSERT(self);

	return self->font_array[font_type];
}

vkk_buffer_t*
vkui_screen_textVb(vkui_screen_t* self, size_t size,
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
vkui_screen_spriteImage(vkui_screen_t* self,
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
	else
	{
		LOGE("invalid name=%s", name);
	}

	if(tex == NULL)
	{
		LOGE("invalid name=%s", name);
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
