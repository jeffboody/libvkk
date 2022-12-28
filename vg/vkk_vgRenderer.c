/*
 * Copyright (c) 2022 Jeff Boody
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

#define LOG_TAG "vkk"
#include "../../libcc/cc_log.h"
#include "../../libcc/cc_memory.h"
#include "../core/vkk_renderer.h"
#include "vkk_vgRenderer.h"
#include "vkk_vgLine.h"
#include "vkk_vgPolygonIdx.h"
#include "vkk_vgPolygon.h"

/***********************************************************
* private                                                  *
***********************************************************/

typedef struct
{
	cc_mat4f_t        mvm;
	vkk_buffer_t*     ub10_mvm;
	vkk_uniformSet_t* us1;
} vkk_vgRendererUSB1_t;

static vkk_vgRendererUSB1_t*
vkk_vgRendererUSB1_new(vkk_vgRenderer_t* vg_rend)
{
	ASSERT(vg_rend);

	vkk_renderer_t* rend   = vg_rend->rend;
	vkk_engine_t*   engine = rend->engine;

	vkk_vgRendererUSB1_t* self;
	self = (vkk_vgRendererUSB1_t*)
	       CALLOC(1, sizeof(vkk_vgRendererUSB1_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->ub10_mvm = vkk_buffer_new(engine,
	                                vkk_renderer_updateMode(rend),
	                                VKK_BUFFER_USAGE_UNIFORM,
	                                sizeof(cc_mat4f_t),
	                                NULL);
	if(self->ub10_mvm == NULL)
	{
		goto fail_ub10_mvm;
	}

	vkk_uniformAttachment_t ua_array[] =
	{
		// layout(std140, set=1, binding=0) uniform uniformMvm
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.buffer  = self->ub10_mvm
		},
	};

	self->us1 = vkk_uniformSet_new(engine, 1, 1,
	                               ua_array, vg_rend->usf1);
	if(self->us1 == NULL)
	{
		goto fail_us1;
	}

	// success
	return self;

	// failure
	fail_us1:
		vkk_buffer_delete(&self->ub10_mvm);
	fail_ub10_mvm:
		FREE(self);
	return NULL;
}

static void
vkk_vgRendererUSB1_delete(vkk_vgRendererUSB1_t** _self)
{
	ASSERT(_self);

	vkk_vgRendererUSB1_t* self = *_self;
	if(self)
	{
		vkk_uniformSet_delete(&self->us1);
		vkk_buffer_delete(&self->ub10_mvm);
		FREE(self);
		*_self = NULL;
	}
}

typedef struct
{
	vkk_buffer_t*     ub20_color;
	vkk_buffer_t*     ub21_brush12WidthCap;
	vkk_uniformSet_t* us2;
} vkk_vgRendererUSB2Line_t;

static vkk_vgRendererUSB2Line_t*
vkk_vgRendererUSB2Line_new(vkk_vgRenderer_t* vg_rend,
                           vkk_vgLineStyle_t* style)
{
	ASSERT(vg_rend);
	ASSERT(style);

	vkk_renderer_t* rend   = vg_rend->rend;
	vkk_engine_t*   engine = rend->engine;

	vkk_vgRendererUSB2Line_t* self;
	self = (vkk_vgRendererUSB2Line_t*)
	       CALLOC(1, sizeof(vkk_vgRendererUSB2Line_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->ub20_color = vkk_buffer_new(engine,
	                                  VKK_UPDATE_MODE_STATIC,
	                                  VKK_BUFFER_USAGE_UNIFORM,
	                                  sizeof(cc_vec4f_t),
	                                  &style->color);
	if(self->ub20_color == NULL)
	{
		goto fail_ub20_color;
	}

	cc_vec4f_t brush12WidthCap =
	{
		.x = style->brush1,
		.y = style->brush2,
		.z = style->width,
		.w = (float) style->cap,
	};
	self->ub21_brush12WidthCap = vkk_buffer_new(engine,
	                                            VKK_UPDATE_MODE_STATIC,
	                                            VKK_BUFFER_USAGE_UNIFORM,
	                                            sizeof(cc_vec4f_t),
	                                            &brush12WidthCap);
	if(self->ub21_brush12WidthCap == NULL)
	{
		goto fail_ub21_brush12WidthCap;
	}

	vkk_uniformAttachment_t ua_array[] =
	{
		// layout(std140, set=2, binding=0) uniform uniformColor
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.buffer  = self->ub20_color,
		},
		// layout(std140, set=2, binding=1) uniform uniformBrush12WidthCap
		{
			.binding = 1,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.buffer  = self->ub21_brush12WidthCap,
		},
	};

	self->us2 = vkk_uniformSet_new(rend->engine, 2, 2,
	                               ua_array,
	                               vg_rend->usf2_line);
	if(self->us2 == NULL)
	{
		goto fail_us2;
	}

	// success
	return self;

	// failure
	fail_us2:
		vkk_buffer_delete(&self->ub21_brush12WidthCap);
	fail_ub21_brush12WidthCap:
		vkk_buffer_delete(&self->ub20_color);
	fail_ub20_color:
		FREE(self);
	return NULL;
}

static void
vkk_vgRendererUSB2Line_delete(vkk_vgRendererUSB2Line_t** _self)
{
	ASSERT(_self);

	vkk_vgRendererUSB2Line_t* self = *_self;
	if(self)
	{
		vkk_uniformSet_delete(&self->us2);
		vkk_buffer_delete(&self->ub21_brush12WidthCap);
		vkk_buffer_delete(&self->ub20_color);
		FREE(self);
		*_self = NULL;
	}
}

typedef struct
{
	vkk_buffer_t*     ub20_color;
	vkk_uniformSet_t* us2;
} vkk_vgRendererUSB2Poly_t;

static vkk_vgRendererUSB2Poly_t*
vkk_vgRendererUSB2Poly_new(vkk_vgRenderer_t* vg_rend,
                           vkk_vgPolygonStyle_t* style)
{
	ASSERT(vg_rend);
	ASSERT(style);

	vkk_renderer_t* rend   = vg_rend->rend;
	vkk_engine_t*   engine = rend->engine;

	vkk_vgRendererUSB2Poly_t* self;
	self = (vkk_vgRendererUSB2Poly_t*)
	       CALLOC(1, sizeof(vkk_vgRendererUSB2Poly_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->ub20_color = vkk_buffer_new(engine,
	                                  VKK_UPDATE_MODE_STATIC,
	                                  VKK_BUFFER_USAGE_UNIFORM,
	                                  sizeof(cc_vec4f_t),
	                                  &style->color);
	if(self->ub20_color == NULL)
	{
		goto fail_ub20_color;
	}

	vkk_uniformAttachment_t ua_array[] =
	{
		// layout(std140, set=2, binding=0) uniform uniformColor
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.buffer  = self->ub20_color,
		},
	};

	self->us2 = vkk_uniformSet_new(rend->engine, 2, 1,
	                               ua_array,
	                               vg_rend->usf2_poly);
	if(self->us2 == NULL)
	{
		goto fail_us2;
	}

	// success
	return self;

	// failure
	fail_us2:
		vkk_buffer_delete(&self->ub20_color);
	fail_ub20_color:
		FREE(self);
	return NULL;
}

static void
vkk_vgRendererUSB2Poly_delete(vkk_vgRendererUSB2Poly_t** _self)
{
	ASSERT(_self);

	vkk_vgRendererUSB2Poly_t* self = *_self;
	if(self)
	{
		vkk_uniformSet_delete(&self->us2);
		vkk_buffer_delete(&self->ub20_color);
		FREE(self);
		*_self = NULL;
	}
}

typedef struct vkk_vgRendererImageState_s
{
	vkk_uniformSetFactory_t* usf2_image;
	vkk_pipelineLayout_t*    pl_image;
	vkk_graphicsPipeline_t*  gp_image;

	// image state updated per draw
	cc_list_t* list_us2_image[2];
} vkk_vgRendererImageState_t;

static vkk_vgRendererImageState_t*
vkk_vgRendererImageState_new(vkk_vgRenderer_t* vg_rend,
                             vkk_samplerInfo_t* si)
{
	ASSERT(vg_rend);
	ASSERT(si);

	vkk_engine_t* engine = vg_rend->rend->engine;

	vkk_vgRendererImageState_t* self;
	self = (vkk_vgRendererImageState_t*)
	       CALLOC(1, sizeof(vkk_vgRendererImageState_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	vkk_uniformBinding_t ub_array2_image[] =
	{
		// layout(set=2, binding=0) uniform sampler2D image;
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_IMAGE_REF,
			.stage   = VKK_STAGE_FS,
			.si      = *si,
		},
	};

	self->usf2_image = vkk_uniformSetFactory_new(engine,
	                                             VKK_UPDATE_MODE_ASYNCHRONOUS,
	                                             1, ub_array2_image);
	if(self->usf2_image == NULL)
	{
		goto fail_usf2_image;
	}

	vkk_uniformSetFactory_t* usf_array_image[] =
	{
		vg_rend->usf0,
		vg_rend->usf1,
		self->usf2_image,
	};

	self->pl_image = vkk_pipelineLayout_new(engine, 3,
	                                        usf_array_image);
	if(self->pl_image == NULL)
	{
		goto fail_pl_image;
	}

	vkk_vertexBufferInfo_t vbi_image =
	{
		// layout(location=0) in vec4 xyuv;
		.location   = 0,
		.components = 4,
		.format     = VKK_VERTEX_FORMAT_FLOAT
	};

	vkk_graphicsPipelineInfo_t gpi_image =
	{
		.renderer   = vg_rend->rend,
		.pl         = self->pl_image,
		.vs         = "vkk/vg/shaders/image_vert.spv",
		.fs         = "vkk/vg/shaders/image_frag.spv",
		.vb_count   = 1,
		.vbi        = &vbi_image,
		.cull_back  = 1,
		.primitive  = VKK_PRIMITIVE_TRIANGLE_STRIP,
		.blend_mode = VKK_BLEND_MODE_TRANSPARENCY,
	};

	self->gp_image = vkk_graphicsPipeline_new(engine,
	                                          &gpi_image);
	if(self->gp_image == NULL)
	{
		goto fail_gp_image;
	}

	self->list_us2_image[0] = cc_list_new();
	if(self->list_us2_image[0] == NULL)
	{
		goto fail_list_us2_image0;
	}

	self->list_us2_image[1] = cc_list_new();
	if(self->list_us2_image[1] == NULL)
	{
		goto fail_list_us2_image1;
	}

	// success
	return self;

	// failure
	fail_list_us2_image1:
		cc_list_delete(&self->list_us2_image[0]);
	fail_list_us2_image0:
		vkk_graphicsPipeline_delete(&self->gp_image);
	fail_gp_image:
		vkk_pipelineLayout_delete(&self->pl_image);
	fail_pl_image:
		vkk_uniformSetFactory_delete(&self->usf2_image);
	fail_usf2_image:
		FREE(self);
	return NULL;
}

static void
vkk_vgRendererImageState_delete(vkk_vgRendererImageState_t** _self)
{
	ASSERT(_self);

	vkk_vgRendererImageState_t* self = *_self;
	if(self)
	{
		cc_list_appendList(self->list_us2_image[0],
		                   self->list_us2_image[1]);

		cc_listIter_t* iter;
		iter = cc_list_head(self->list_us2_image[0]);
		while(iter)
		{
			vkk_uniformSet_t* us2;
			us2 = (vkk_uniformSet_t*)
			      cc_list_remove(self->list_us2_image[0],
			                     &iter);
			vkk_uniformSet_delete(&us2);
		}

		cc_list_delete(&self->list_us2_image[1]);
		cc_list_delete(&self->list_us2_image[0]);
		vkk_graphicsPipeline_delete(&self->gp_image);
		vkk_pipelineLayout_delete(&self->pl_image);
		vkk_uniformSetFactory_delete(&self->usf2_image);
		FREE(self);
		*_self = NULL;
	}
}

static void
vkk_vgRendererImageState_reset(vkk_vgRendererImageState_t* self)
{
	ASSERT(self);

	cc_list_appendList(self->list_us2_image[0],
	                   self->list_us2_image[1]);
}

static vkk_uniformSet_t*
vkk_vgRendererImageState_us2(vkk_vgRendererImageState_t* self,
                             vkk_engine_t* engine)
{
	ASSERT(self);
	ASSERT(engine);

	vkk_uniformSet_t* us2;
	cc_listIter_t*    iter;
	iter = cc_list_head(self->list_us2_image[0]);
	if(iter)
	{
		us2 = (vkk_uniformSet_t*)
		      cc_list_peekIter(iter);
		cc_list_swapn(self->list_us2_image[0],
		              self->list_us2_image[1],
		              iter, NULL);
	}
	else
	{
		us2 = vkk_uniformSet_new(engine, 2, 0, NULL,
		                         self->usf2_image);
		if(us2 == NULL)
		{
			return NULL;
		}

		if(cc_list_append(self->list_us2_image[1], NULL,
		                  (const void*) us2) == NULL)
		{
			vkk_uniformSet_delete(&us2);
			return NULL;
		}
	}

	return us2;
}

typedef struct
{
	vkk_buffer_t*     ub30_dist;
	vkk_uniformSet_t* us3;
} vkk_vgRendererUSB3Line_t;

static vkk_vgRendererUSB3Line_t*
vkk_vgRendererUSB3Line_new(vkk_vgRenderer_t* vg_rend, float dist)
{
	ASSERT(vg_rend);

	vkk_renderer_t* rend   = vg_rend->rend;
	vkk_engine_t*   engine = rend->engine;

	vkk_vgRendererUSB3Line_t* self;
	self = (vkk_vgRendererUSB3Line_t*)
	       CALLOC(1, sizeof(vkk_vgRendererUSB3Line_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->ub30_dist = vkk_buffer_new(engine,
	                                 vkk_renderer_updateMode(rend),
	                                 VKK_BUFFER_USAGE_UNIFORM,
	                                 sizeof(float),
	                                 &dist);
	if(self->ub30_dist == NULL)
	{
		goto fail_ub30_dist;
	}

	vkk_uniformAttachment_t ua_array[] =
	{
		// layout(std140, set=3, binding=0) uniform uniformDist
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.buffer  = self->ub30_dist
		},
	};

	self->us3 = vkk_uniformSet_new(engine, 3, 1,
	                               ua_array,
	                               vg_rend->usf3_line);
	if(self->us3 == NULL)
	{
		goto fail_us3;
	}

	// success
	return self;

	// failure
	fail_us3:
		vkk_buffer_delete(&self->ub30_dist);
	fail_ub30_dist:
		FREE(self);
	return NULL;
}

static void
vkk_vgRendererUSB3Line_delete(vkk_vgRendererUSB3Line_t** _self)
{
	ASSERT(_self);

	vkk_vgRendererUSB3Line_t* self = *_self;
	if(self)
	{
		vkk_uniformSet_delete(&self->us3);
		vkk_buffer_delete(&self->ub30_dist);
		FREE(self);
		*_self = NULL;
	}
}

static int
vkk_vgRenderer_bindLine(vkk_vgRenderer_t* self,
                        float dist,
                        vkk_vgLineStyle_t* style)
{
	ASSERT(self);
	ASSERT(style);

	vkk_renderer_t* rend = self->rend;

	// get us1
	vkk_uniformSet_t*    us1 = self->us1;
	vkk_vgRendererUSB1_t* usb1;
	usb1 = (vkk_vgRendererUSB1_t*)
	       cc_list_peekHead(self->stack_usb1);
	if(usb1)
	{
		us1 = usb1->us1;
	}

	// get us2
	vkk_vgRendererUSB2Line_t* usb2_line;
	cc_mapIter_t*             miter;
	miter = cc_map_findp(self->map_usb2_line,
	                     sizeof(vkk_vgLineStyle_t), style);
	if(miter)
	{
		usb2_line = (vkk_vgRendererUSB2Line_t*)
		            cc_map_val(miter);
	}
	else
	{
		usb2_line = vkk_vgRendererUSB2Line_new(self, style);
		if(usb2_line == NULL)
		{
			return 0;
		}

		if(cc_map_addp(self->map_usb2_line, usb2_line,
		               sizeof(vkk_vgLineStyle_t),
		               style) == NULL)
		{
			vkk_vgRendererUSB2Line_delete(&usb2_line);
			return 0;
		}
	}

	// get us3
	vkk_vgRendererUSB3Line_t* usb3_line;
	cc_listIter_t*            iter;
	iter = cc_list_head(self->list_usb3_line[0]);
	if(iter)
	{
		usb3_line = (vkk_vgRendererUSB3Line_t*)
		            cc_list_peekIter(iter);
		cc_list_swapn(self->list_usb3_line[0],
		              self->list_usb3_line[1],
		              iter, NULL);
		vkk_renderer_updateBuffer(rend, usb3_line->ub30_dist,
		                          sizeof(float), &dist);
	}
	else
	{
		usb3_line = vkk_vgRendererUSB3Line_new(self, dist);
		if(usb3_line == NULL)
		{
			return 0;
		}

		if(cc_list_append(self->list_usb3_line[1], NULL,
		                  (const void*) usb3_line) == NULL)
		{
			vkk_vgRendererUSB3Line_delete(&usb3_line);
			return 0;
		}
	}

	// bind uniform sets
	vkk_uniformSet_t* us_array[] =
	{
		us1,
		usb2_line->us2,
		usb3_line->us3,
	};
	vkk_renderer_bindUniformSets(rend, 3, us_array);

	return 1;
}

static int
vkk_vgRenderer_bindPolygon(vkk_vgRenderer_t* self,
                           vkk_vgPolygonStyle_t* style)
{
	ASSERT(self);
	ASSERT(style);

	vkk_renderer_t* rend = self->rend;

	// get us1
	vkk_uniformSet_t*     us1 = self->us1;
	vkk_vgRendererUSB1_t* usb1;
	usb1 = (vkk_vgRendererUSB1_t*)
	       cc_list_peekHead(self->stack_usb1);
	if(usb1)
	{
		us1 = usb1->us1;
	}

	// get us2
	vkk_vgRendererUSB2Poly_t* usb2_poly;
	cc_mapIter_t*             miter;
	miter = cc_map_findp(self->map_usb2_poly,
	                     sizeof(vkk_vgPolygonStyle_t), style);
	if(miter)
	{
		usb2_poly = (vkk_vgRendererUSB2Poly_t*)
		            cc_map_val(miter);
	}
	else
	{
		usb2_poly = vkk_vgRendererUSB2Poly_new(self, style);
		if(usb2_poly == NULL)
		{
			return 0;
		}

		if(cc_map_addp(self->map_usb2_poly, usb2_poly,
		               sizeof(vkk_vgPolygonStyle_t),
		               style) == NULL)
		{
			vkk_vgRendererUSB2Poly_delete(&usb2_poly);
			return 0;
		}
	}

	// bind uniform sets
	vkk_uniformSet_t* us_array[] =
	{
		us1,
		usb2_poly->us2,
	};
	vkk_renderer_bindUniformSets(rend, 2, us_array);

	return 1;
}

static int
vkk_vgRenderer_bindImage(vkk_vgRenderer_t* self,
                         vkk_image_t* image)
{
	ASSERT(self);
	ASSERT(image);

	vkk_renderer_t* rend = self->rend;

	if(self->image_state == NULL)
	{
		return 0;
	}

	// get us1
	vkk_uniformSet_t*     us1 = self->us1;
	vkk_vgRendererUSB1_t* usb1;
	usb1 = (vkk_vgRendererUSB1_t*)
	       cc_list_peekHead(self->stack_usb1);
	if(usb1)
	{
		us1 = usb1->us1;
	}

	// get us2
	vkk_uniformSet_t* us2;
	us2 = vkk_vgRendererImageState_us2(self->image_state,
	                                   rend->engine);
	if(us2 == NULL)
	{
		return 0;
	}

	vkk_uniformAttachment_t ua_array2[] =
	{
		// layout(set=2, binding=0) uniform sampler2D image;
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_IMAGE_REF,
			.image   = image,
		},
	};

	vkk_renderer_updateUniformSetRefs(rend, us2,
	                                  1, ua_array2);

	// bind uniform sets
	vkk_uniformSet_t* us_array[] =
	{
		us1,
		us2,
	};
	vkk_renderer_bindUniformSets(rend, 2, us_array);

	return 1;
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_vgRenderer_t* vkk_vgRenderer_new(vkk_renderer_t* rend)
{
	ASSERT(rend);

	vkk_engine_t* engine = rend->engine;

	vkk_vgRenderer_t* self;
	self = (vkk_vgRenderer_t*)
	       CALLOC(1, sizeof(vkk_vgRenderer_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->rend = rend;

	vkk_uniformBinding_t ub_array0[] =
	{
		// layout(std140, set=0, binding=0) uniform uniformPm
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.stage   = VKK_STAGE_VS,
		},
	};

	self->usf0 = vkk_uniformSetFactory_new(engine,
	                                       vkk_renderer_updateMode(rend),
	                                       1, ub_array0);
	if(self->usf0 == NULL)
	{
		goto fail_usf0;
	}

	vkk_uniformBinding_t ub_array1[] =
	{
		// layout(std140, set=1, binding=0) uniform uniformMvm
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.stage   = VKK_STAGE_VS,
		},
	};

	self->usf1 = vkk_uniformSetFactory_new(engine,
	                                       vkk_renderer_updateMode(rend),
	                                       1, ub_array1);
	if(self->usf1 == NULL)
	{
		goto fail_usf1;
	}

	vkk_uniformBinding_t ub_array2_line[] =
	{
		// layout(std140, set=2, binding=0) uniform uniformColor
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.stage   = VKK_STAGE_FS,
		},
		// layout(std140, set=2, binding=1) uniform uniformBrush12WidthCap
		{
			.binding = 1,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.stage   = VKK_STAGE_VSFS,
		},
	};

	self->usf2_line = vkk_uniformSetFactory_new(engine,
	                                            VKK_UPDATE_MODE_STATIC,
	                                            2, ub_array2_line);
	if(self->usf2_line == NULL)
	{
		goto fail_usf2_line;
	}

	vkk_uniformBinding_t ub_array2_poly[] =
	{
		// layout(std140, set=2, binding=0) uniform uniformColor
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.stage   = VKK_STAGE_FS,
		},
	};

	self->usf2_poly = vkk_uniformSetFactory_new(engine,
	                                            VKK_UPDATE_MODE_STATIC,
	                                            1, ub_array2_poly);
	if(self->usf2_poly == NULL)
	{
		goto fail_usf2_poly;
	}

	vkk_uniformBinding_t ub_array3_line[] =
	{
		// layout(std140, set=3, binding=0) uniform uniformDist
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.stage   = VKK_STAGE_FS,
		},
	};

	self->usf3_line = vkk_uniformSetFactory_new(engine,
	                                            vkk_renderer_updateMode(rend),
	                                            1, ub_array3_line);
	if(self->usf3_line == NULL)
	{
		goto fail_usf3_line;
	}

	vkk_uniformSetFactory_t* usf_array_line[] =
	{
		self->usf0,
		self->usf1,
		self->usf2_line,
		self->usf3_line,
	};

	self->pl_line = vkk_pipelineLayout_new(engine, 4,
	                                       usf_array_line);
	if(self->pl_line == NULL)
	{
		goto fail_pl_line;
	}

	vkk_uniformSetFactory_t* usf_array_poly[] =
	{
		self->usf0,
		self->usf1,
		self->usf2_poly,
	};

	self->pl_poly = vkk_pipelineLayout_new(engine, 3,
	                                       usf_array_poly);
	if(self->pl_poly == NULL)
	{
		goto fail_pl_poly;
	}

	vkk_vertexBufferInfo_t vbi_line[] =
	{
		{
			// layout(location=0) in vec4 xyst;
			.location   = 0,
			.components = 4,
			.format     = VKK_VERTEX_FORMAT_FLOAT
		},
		{
			// layout(location=1) in vec2 dxdy;
			.location   = 1,
			.components = 2,
			.format     = VKK_VERTEX_FORMAT_FLOAT
		},
	};

	vkk_graphicsPipelineInfo_t gpi_line =
	{
		.renderer   = rend,
		.pl         = self->pl_line,
		.vs         = "vkk/vg/shaders/line_vert.spv",
		.fs         = "vkk/vg/shaders/line_frag.spv",
		.vb_count   = 2,
		.vbi        = vbi_line,
		.primitive  = VKK_PRIMITIVE_TRIANGLE_STRIP,
		.blend_mode = VKK_BLEND_MODE_TRANSPARENCY,
	};

	self->gp_line = vkk_graphicsPipeline_new(engine, &gpi_line);
	if(self->gp_line == NULL)
	{
		goto fail_gp_line;
	}

	vkk_vertexBufferInfo_t vbi_poly =
	{
		// layout(location=0) in vec2 xy;
		.location   = 0,
		.components = 2,
		.format     = VKK_VERTEX_FORMAT_FLOAT
	};

	vkk_graphicsPipelineInfo_t gpi_poly =
	{
		.renderer   = rend,
		.pl         = self->pl_poly,
		.vs         = "vkk/vg/shaders/polygon_vert.spv",
		.fs         = "vkk/vg/shaders/polygon_frag.spv",
		.vb_count   = 1,
		.vbi        = &vbi_poly,
		.primitive  = VKK_PRIMITIVE_TRIANGLE_FAN,
		.blend_mode = VKK_BLEND_MODE_TRANSPARENCY,
	};

	self->gp_poly = vkk_graphicsPipeline_new(engine, &gpi_poly);
	if(self->gp_poly == NULL)
	{
		goto fail_gp_poly;
	}

	self->ub00_pm = vkk_buffer_new(engine,
	                               vkk_renderer_updateMode(rend),
	                               VKK_BUFFER_USAGE_UNIFORM,
	                               sizeof(cc_mat4f_t), NULL);
	if(self->ub00_pm == NULL)
	{
		goto fail_ub00_pm;
	}

	vkk_uniformAttachment_t ua_array0[] =
	{
		// layout(std140, set=0, binding=0) uniform uniformPm
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.buffer  = self->ub00_pm
		}
	};

	self->us0 = vkk_uniformSet_new(engine, 0, 1,
	                               ua_array0, self->usf0);
	if(self->us0 == NULL)
	{
		goto fail_us0;
	}

	cc_mat4f_t identity;
	cc_mat4f_identity(&identity);
	self->ub10_mvm_identity = vkk_buffer_new(engine,
	                                         vkk_renderer_updateMode(rend),
	                                         VKK_BUFFER_USAGE_UNIFORM,
	                                         sizeof(cc_mat4f_t),
	                                         &identity);
	if(self->ub10_mvm_identity == NULL)
	{
		goto fail_ub10_mvm_identity;
	}

	vkk_uniformAttachment_t ua_array1[] =
	{
		// layout(std140, set=1, binding=0) uniform uniformMvm
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.buffer  = self->ub10_mvm_identity
		}
	};

	self->us1 = vkk_uniformSet_new(engine, 1, 1,
	                               ua_array1, self->usf1);
	if(self->us1 == NULL)
	{
		goto fail_us1;
	}

	self->list_usb1[0] = cc_list_new();
	if(self->list_usb1[0] == NULL)
	{
		goto fail_list_usb10;
	}

	self->list_usb1[1] = cc_list_new();
	if(self->list_usb1[1] == NULL)
	{
		goto fail_list_usb11;
	}

	self->stack_usb1 = cc_list_new();
	if(self->stack_usb1 == NULL)
	{
		goto fail_stack_usb1;
	}

	self->map_usb2_line = cc_map_new();
	if(self->map_usb2_line == NULL)
	{
		goto fail_map_usb2_line;
	}

	self->map_usb2_poly = cc_map_new();
	if(self->map_usb2_poly == NULL)
	{
		goto fail_map_usb2_poly;
	}

	self->map_image = cc_map_new();
	if(self->map_image == NULL)
	{
		goto fail_map_image;
	}

	self->list_usb3_line[0] = cc_list_new();
	if(self->list_usb3_line[0] == NULL)
	{
		goto fail_list_usb3_line0;
	}

	self->list_usb3_line[1] = cc_list_new();
	if(self->list_usb3_line[1] == NULL)
	{
		goto fail_list_usb3_line1;
	}

	float vb_xyuv_image0[] =
	{
		-0.5f, -0.5f, 0.0f, 0.0f,
		-0.5f,  0.5f, 0.0f, 1.0f,
		 0.5f, -0.5f, 1.0f, 0.0f,
		 0.5f,  0.5f, 1.0f, 1.0f,
	};

	self->vb_xyuv_image[0] = vkk_buffer_new(engine,
	                                        VKK_UPDATE_MODE_STATIC,
	                                        VKK_BUFFER_USAGE_VERTEX,
	                                        4*sizeof(cc_vec4f_t),
	                                        vb_xyuv_image0);
	if(self->vb_xyuv_image[0] == NULL)
	{
		goto fail_vb_xyuv_image0;
	}

	float vb_xyuv_image1[] =
	{
		-0.5f,  0.5f, 0.0f, 0.0f,
		-0.5f, -0.5f, 0.0f, 1.0f,
		 0.5f,  0.5f, 1.0f, 0.0f,
		 0.5f, -0.5f, 1.0f, 1.0f,
	};

	self->vb_xyuv_image[1] = vkk_buffer_new(engine,
	                                        VKK_UPDATE_MODE_STATIC,
	                                        VKK_BUFFER_USAGE_VERTEX,
	                                        4*sizeof(cc_vec4f_t),
	                                        vb_xyuv_image1);
	if(self->vb_xyuv_image[1] == NULL)
	{
		goto fail_vb_xyuv_image1;
	}

	// success
	return self;

	// failure
	fail_vb_xyuv_image1:
		vkk_buffer_delete(&self->vb_xyuv_image[0]);
	fail_vb_xyuv_image0:
		cc_list_delete(&self->list_usb3_line[1]);
	fail_list_usb3_line1:
		cc_list_delete(&self->list_usb3_line[0]);
	fail_list_usb3_line0:
		cc_map_delete(&self->map_image);
	fail_map_image:
		cc_map_delete(&self->map_usb2_poly);
	fail_map_usb2_poly:
		cc_map_delete(&self->map_usb2_line);
	fail_map_usb2_line:
		cc_list_delete(&self->stack_usb1);
	fail_stack_usb1:
		cc_list_delete(&self->list_usb1[1]);
	fail_list_usb11:
		cc_list_delete(&self->list_usb1[0]);
	fail_list_usb10:
		vkk_uniformSet_delete(&self->us1);
	fail_us1:
		vkk_buffer_delete(&self->ub10_mvm_identity);
	fail_ub10_mvm_identity:
		vkk_uniformSet_delete(&self->us0);
	fail_us0:
		vkk_buffer_delete(&self->ub00_pm);
	fail_ub00_pm:
		vkk_graphicsPipeline_delete(&self->gp_poly);
	fail_gp_poly:
		vkk_graphicsPipeline_delete(&self->gp_line);
	fail_gp_line:
		vkk_pipelineLayout_delete(&self->pl_poly);
	fail_pl_poly:
		vkk_pipelineLayout_delete(&self->pl_line);
	fail_pl_line:
		vkk_uniformSetFactory_delete(&self->usf3_line);
	fail_usf3_line:
		vkk_uniformSetFactory_delete(&self->usf2_poly);
	fail_usf2_poly:
		vkk_uniformSetFactory_delete(&self->usf2_line);
	fail_usf2_line:
		vkk_uniformSetFactory_delete(&self->usf1);
	fail_usf1:
		vkk_uniformSetFactory_delete(&self->usf0);
	fail_usf0:
		FREE(self);
	return NULL;
}

void vkk_vgRenderer_delete(vkk_vgRenderer_t** _self)
{
	ASSERT(_self);

	vkk_vgRenderer_t* self = *_self;
	if(self)
	{
		cc_list_appendList(self->list_usb1[0],
		                   self->list_usb1[1]);
		cc_list_discard(self->stack_usb1);
		cc_list_appendList(self->list_usb3_line[0],
		                   self->list_usb3_line[1]);

		cc_listIter_t* iter;
		iter = cc_list_head(self->list_usb3_line[0]);
		while(iter)
		{
			vkk_vgRendererUSB3Line_t* usb3_line;
			usb3_line = (vkk_vgRendererUSB3Line_t*)
			            cc_list_remove(self->list_usb3_line[0],
			                           &iter);
			vkk_vgRendererUSB3Line_delete(&usb3_line);
		}

		cc_mapIter_t* miter;
		miter = cc_map_head(self->map_image);
		while(miter)
		{
			vkk_vgRendererImageState_t* image_state;
			image_state = (vkk_vgRendererImageState_t*)
			              cc_map_remove(self->map_image, &miter);
			vkk_vgRendererImageState_delete(&image_state);
		}

		miter = cc_map_head(self->map_usb2_poly);
		while(miter)
		{
			vkk_vgRendererUSB2Poly_t* usb2_poly;
			usb2_poly = (vkk_vgRendererUSB2Poly_t*)
			            cc_map_remove(self->map_usb2_poly, &miter);
			vkk_vgRendererUSB2Poly_delete(&usb2_poly);
		}

		miter = cc_map_head(self->map_usb2_line);
		while(miter)
		{
			vkk_vgRendererUSB2Line_t* usb2_line;
			usb2_line = (vkk_vgRendererUSB2Line_t*)
			            cc_map_remove(self->map_usb2_line, &miter);
			vkk_vgRendererUSB2Line_delete(&usb2_line);
		}

		iter = cc_list_head(self->list_usb1[0]);
		while(iter)
		{
			vkk_vgRendererUSB1_t* usb1;
			usb1 = (vkk_vgRendererUSB1_t*)
			       cc_list_remove(self->list_usb1[0], &iter);
			vkk_vgRendererUSB1_delete(&usb1);
		}

		vkk_buffer_delete(&self->vb_xyuv_image[1]);
		vkk_buffer_delete(&self->vb_xyuv_image[0]);
		cc_list_delete(&self->list_usb3_line[1]);
		cc_list_delete(&self->list_usb3_line[0]);
		cc_map_delete(&self->map_image);
		cc_map_delete(&self->map_usb2_poly);
		cc_map_delete(&self->map_usb2_line);
		cc_list_delete(&self->stack_usb1);
		cc_list_delete(&self->list_usb1[1]);
		cc_list_delete(&self->list_usb1[0]);
		vkk_uniformSet_delete(&self->us1);
		vkk_buffer_delete(&self->ub10_mvm_identity);
		vkk_uniformSet_delete(&self->us0);
		vkk_buffer_delete(&self->ub00_pm);
		vkk_graphicsPipeline_delete(&self->gp_poly);
		vkk_graphicsPipeline_delete(&self->gp_line);
		vkk_pipelineLayout_delete(&self->pl_poly);
		vkk_pipelineLayout_delete(&self->pl_line);
		vkk_uniformSetFactory_delete(&self->usf3_line);
		vkk_uniformSetFactory_delete(&self->usf2_poly);
		vkk_uniformSetFactory_delete(&self->usf2_line);
		vkk_uniformSetFactory_delete(&self->usf1);
		vkk_uniformSetFactory_delete(&self->usf0);
		FREE(self);
		*_self = NULL;
	}
}

void vkk_vgRenderer_reset(vkk_vgRenderer_t* self,
                          cc_mat4f_t* pm)
{
	ASSERT(self);
	ASSERT(pm);

	vkk_renderer_updateBuffer(self->rend, self->ub00_pm,
	                          sizeof(cc_mat4f_t), pm);
	cc_list_appendList(self->list_usb1[0],
	                   self->list_usb1[1]);
	cc_list_discard(self->stack_usb1);
	cc_list_appendList(self->list_usb3_line[0],
	                   self->list_usb3_line[1]);

	cc_mapIter_t* miter;
	miter = cc_map_head(self->map_image);
	while(miter)
	{
		vkk_vgRendererImageState_t* image_state;
		image_state = (vkk_vgRendererImageState_t*)
		              cc_map_val(miter);
		vkk_vgRendererImageState_reset(image_state);
		miter = cc_map_next(miter);
	}
	self->image_state = NULL;

	// check y axis up sign
	// by using sign of m11 component
	// of the orthographic projection
	if(pm->m11 > 0.0f)
	{
		self->image_yup = 0;
	}
	else
	{
		self->image_yup = 1;
	}
}

void vkk_vgRenderer_bindLines(vkk_vgRenderer_t* self)
{
	ASSERT(self);

	vkk_renderer_bindGraphicsPipeline(self->rend,
	                                  self->gp_line);
	vkk_renderer_bindUniformSets(self->rend, 1,
	                             &self->us0);
}

void vkk_vgRenderer_bindPolygons(vkk_vgRenderer_t* self)
{
	ASSERT(self);

	vkk_renderer_bindGraphicsPipeline(self->rend,
	                                  self->gp_poly);
	vkk_renderer_bindUniformSets(self->rend, 1,
	                             &self->us0);
}

void vkk_vgRenderer_bindImages(vkk_vgRenderer_t* self,
                               vkk_samplerInfo_t* si)
{
	ASSERT(self);
	ASSERT(si);

	cc_mapIter_t* miter;
	vkk_vgRendererImageState_t* image_state;
	miter = cc_map_findp(self->map_image,
	                     sizeof(vkk_samplerInfo_t), si);
	if(miter)
	{
		image_state = (vkk_vgRendererImageState_t*)
		              cc_map_val(miter);
	}
	else
	{
		image_state = vkk_vgRendererImageState_new(self, si);
		if(image_state == NULL)
		{
			self->image_state = NULL;
			return;
		}

		if(cc_map_addp(self->map_image, image_state,
		               sizeof(vkk_samplerInfo_t), si) == NULL)
		{
			vkk_vgRendererImageState_delete(&image_state);
			self->image_state = NULL;
			return;
		}
	}

	vkk_renderer_bindGraphicsPipeline(self->rend,
	                                  image_state->gp_image);
	vkk_renderer_bindUniformSets(self->rend, 1,
	                             &self->us0);
	self->image_state = image_state;
}

int vkk_vgRenderer_pushMatrix(vkk_vgRenderer_t* self,
                              cc_mat4f_t* mvm)
{
	ASSERT(self);
	ASSERT(mvm);

	// try to use an existing mvm
	// or fall back to create a new mvm
	vkk_vgRendererUSB1_t* usb1;
	cc_listIter_t*        iter;
	cc_listIter_t*        siter;
	iter = cc_list_head(self->list_usb1[0]);
	if(iter)
	{
		usb1 = (vkk_vgRendererUSB1_t*) cc_list_peekIter(iter);

		siter = cc_list_insert(self->stack_usb1, NULL, usb1);
		if(siter == NULL)
		{
			return 0;
		}

		cc_list_swapn(self->list_usb1[0], self->list_usb1[1],
		              iter, NULL);
	}
	else
	{
		usb1 = vkk_vgRendererUSB1_new(self);
		if(usb1 == NULL)
		{
			return 0;
		}

		siter = cc_list_insert(self->stack_usb1, NULL, usb1);
		if(siter == NULL)
		{
			goto fail_stack;
		}

		if(cc_list_append(self->list_usb1[1], NULL, usb1) == NULL)
		{
			goto fail_list_usb1;
		}
	}

	// update mvm
	cc_mat4f_copy(mvm, &usb1->mvm);
	vkk_renderer_updateBuffer(self->rend, usb1->ub10_mvm,
	                          sizeof(cc_mat4f_t),
	                          &usb1->mvm);

	// success
	return 1;

	// failure
	fail_list_usb1:
		cc_list_remove(self->stack_usb1, &siter);
	fail_stack:
		vkk_vgRendererUSB1_delete(&usb1);
	return 0;
}

void vkk_vgRenderer_popMatrix(vkk_vgRenderer_t* self)
{
	ASSERT(self);

	cc_listIter_t* iter = cc_list_head(self->stack_usb1);
	if(iter)
	{
		cc_list_remove(self->stack_usb1, &iter);
	}
}

void vkk_vgRenderer_drawLine(vkk_vgRenderer_t* self,
                             vkk_vgLineStyle_t* style,
                             vkk_vgLine_t* line)
{
	ASSERT(self);
	ASSERT(style);
	ASSERT(line);

	if(vkk_vgRenderer_bindLine(self, line->dist, style) == 0)
	{
		return;
	}

	vkk_buffer_t* vb_array[] =
	{
		line->vb_xyst,
		line->vb_dxdy,
	};
	vkk_renderer_draw(self->rend, line->vc, 2, vb_array);
}

void vkk_vgRenderer_drawPolygon(vkk_vgRenderer_t* self,
                                vkk_vgPolygonStyle_t* style,
                                vkk_vgPolygon_t* polygon)
{
	ASSERT(self);
	ASSERT(style);
	ASSERT(polygon);

	if(vkk_vgRenderer_bindPolygon(self, style) == 0)
	{
		return;
	}

	cc_listIter_t* iter = cc_list_head(polygon->list_idx);
	while(iter)
	{
		vkk_vgPolygonIdx_t* pi;
		pi = (vkk_vgPolygonIdx_t*) cc_list_peekIter(iter);

		vkk_renderer_drawIndexed(self->rend,
		                         pi->count, 1,
		                         VKK_INDEX_TYPE_USHORT,
		                         pi->ib, &polygon->vb_xy);
		iter = cc_list_next(iter);
	}
}

void vkk_vgRenderer_drawImage(vkk_vgRenderer_t* self,
                              vkk_image_t* image)
{
	ASSERT(self);
	ASSERT(image);

	if(vkk_vgRenderer_bindImage(self, image) == 0)
	{
		return;
	}

	vkk_buffer_t* vb_xyuv_image;
	vb_xyuv_image = self->vb_xyuv_image[self->image_yup];
	vkk_renderer_draw(self->rend, 4, 1, &vb_xyuv_image);
}
