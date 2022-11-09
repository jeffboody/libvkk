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
#include "vkk_vgContext.h"

/***********************************************************
* private                                                  *
***********************************************************/

typedef struct
{
	cc_mat4f_t        mvm;
	vkk_buffer_t*     ub10_mvm;
	vkk_uniformSet_t* us1;
} vkk_vgContextUSB1_t;

static vkk_vgContextUSB1_t*
vkk_vgContextUSB1_new(vkk_vgContext_t* ctx)
{
	ASSERT(ctx);

	vkk_renderer_t* rend   = ctx->rend;
	vkk_engine_t*   engine = rend->engine;

	vkk_vgContextUSB1_t* self;
	self = (vkk_vgContextUSB1_t*)
	       CALLOC(1, sizeof(vkk_vgContextUSB1_t));
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
	                               ua_array, ctx->usf1);
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
vkk_vgContextUSB1_delete(vkk_vgContextUSB1_t** _self)
{
	ASSERT(_self);

	vkk_vgContextUSB1_t* self = *_self;
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
} vkk_vgContextUSB2Line_t;

static vkk_vgContextUSB2Line_t*
vkk_vgContextUSB2Line_new(vkk_vgContext_t* ctx,
                          vkk_vgLineStyle_t* style)
{
	ASSERT(ctx);
	ASSERT(style);

	vkk_renderer_t* rend   = ctx->rend;
	vkk_engine_t*   engine = rend->engine;

	vkk_vgContextUSB2Line_t* self;
	self = (vkk_vgContextUSB2Line_t*)
	       CALLOC(1, sizeof(vkk_vgContextUSB2Line_t));
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
	                               ua_array, ctx->usf2_line);
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
vkk_vgContextUSB2Line_delete(vkk_vgContextUSB2Line_t** _self)
{
	ASSERT(_self);

	vkk_vgContextUSB2Line_t* self = *_self;
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
} vkk_vgContextUSB2Poly_t;

static vkk_vgContextUSB2Poly_t*
vkk_vgContextUSB2Poly_new(vkk_vgContext_t* ctx,
                          vkk_vgPolygonStyle_t* style)
{
	ASSERT(ctx);
	ASSERT(style);

	vkk_renderer_t* rend   = ctx->rend;
	vkk_engine_t*   engine = rend->engine;

	vkk_vgContextUSB2Poly_t* self;
	self = (vkk_vgContextUSB2Poly_t*)
	       CALLOC(1, sizeof(vkk_vgContextUSB2Poly_t));
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
	                               ua_array, ctx->usf2_poly);
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
vkk_vgContextUSB2Poly_delete(vkk_vgContextUSB2Poly_t** _self)
{
	ASSERT(_self);

	vkk_vgContextUSB2Poly_t* self = *_self;
	if(self)
	{
		vkk_uniformSet_delete(&self->us2);
		vkk_buffer_delete(&self->ub20_color);
		FREE(self);
		*_self = NULL;
	}
}

typedef struct
{
	vkk_buffer_t*     ub30_dist;
	vkk_uniformSet_t* us3;
} vkk_vgContextUSB3Line_t;

static vkk_vgContextUSB3Line_t*
vkk_vgContextUSB3Line_new(vkk_vgContext_t* ctx, float dist)
{
	ASSERT(ctx);

	vkk_renderer_t* rend   = ctx->rend;
	vkk_engine_t*   engine = rend->engine;

	vkk_vgContextUSB3Line_t* self;
	self = (vkk_vgContextUSB3Line_t*)
	       CALLOC(1, sizeof(vkk_vgContextUSB3Line_t));
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
	                               ua_array, ctx->usf3_line);
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
vkk_vgContextUSB3Line_delete(vkk_vgContextUSB3Line_t** _self)
{
	ASSERT(_self);

	vkk_vgContextUSB3Line_t* self = *_self;
	if(self)
	{
		vkk_uniformSet_delete(&self->us3);
		vkk_buffer_delete(&self->ub30_dist);
		FREE(self);
		*_self = NULL;
	}
}

/***********************************************************
* protected                                                *
***********************************************************/

int vkk_vgContext_bindLine(vkk_vgContext_t* self,
                           float dist,
                           vkk_vgLineStyle_t* style)
{
	ASSERT(self);
	ASSERT(style);

	vkk_renderer_t* rend = self->rend;

	// get us1
	vkk_uniformSet_t*    us1 = self->us1;
	vkk_vgContextUSB1_t* usb1;
	usb1 = (vkk_vgContextUSB1_t*)
	       cc_list_peekHead(self->stack_usb1);
	if(usb1)
	{
		us1 = usb1->us1;
	}

	// get us2
	vkk_vgContextUSB2Line_t* usb2_line;
	cc_mapIter_t*            miter;
	miter = cc_map_findp(self->map_usb2_line,
	                     sizeof(vkk_vgLineStyle_t), style);
	if(miter)
	{
		usb2_line = (vkk_vgContextUSB2Line_t*) cc_map_val(miter);
	}
	else
	{
		usb2_line = vkk_vgContextUSB2Line_new(self, style);
		if(usb2_line == NULL)
		{
			return 0;
		}

		if(cc_map_addp(self->map_usb2_line, usb2_line,
		               sizeof(vkk_vgLineStyle_t),
		               style) == NULL)
		{
			vkk_vgContextUSB2Line_delete(&usb2_line);
			return 0;
		}
	}

	// get us3
	vkk_vgContextUSB3Line_t* usb3_line;
	cc_listIter_t*           iter;
	iter = cc_list_head(self->list_usb3_line[0]);
	if(iter)
	{
		usb3_line = (vkk_vgContextUSB3Line_t*)
		            cc_list_peekIter(iter);
		cc_list_swapn(self->list_usb3_line[0],
		              self->list_usb3_line[1],
		              iter, NULL);
		vkk_renderer_updateBuffer(rend, usb3_line->ub30_dist,
		                          sizeof(float), &dist);
	}
	else
	{
		usb3_line = vkk_vgContextUSB3Line_new(self, dist);
		if(usb3_line == NULL)
		{
			return 0;
		}

		if(cc_list_append(self->list_usb3_line[1], NULL,
		                  (const void*) usb3_line) == NULL)
		{
			vkk_vgContextUSB3Line_delete(&usb3_line);
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

int vkk_vgContext_bindPolygon(vkk_vgContext_t* self,
                              vkk_vgPolygonStyle_t* style)
{
	ASSERT(self);
	ASSERT(style);

	vkk_renderer_t* rend = self->rend;

	// get us1
	vkk_uniformSet_t*    us1 = self->us1;
	vkk_vgContextUSB1_t* usb1;
	usb1 = (vkk_vgContextUSB1_t*)
	       cc_list_peekHead(self->stack_usb1);
	if(usb1)
	{
		us1 = usb1->us1;
	}

	// get us2
	vkk_vgContextUSB2Poly_t* usb2_poly;
	cc_mapIter_t*            miter;
	miter = cc_map_findp(self->map_usb2_poly,
	                     sizeof(vkk_vgPolygonStyle_t), style);
	if(miter)
	{
		usb2_poly = (vkk_vgContextUSB2Poly_t*) cc_map_val(miter);
	}
	else
	{
		usb2_poly = vkk_vgContextUSB2Poly_new(self, style);
		if(usb2_poly == NULL)
		{
			return 0;
		}

		if(cc_map_addp(self->map_usb2_poly, usb2_poly,
		               sizeof(vkk_vgPolygonStyle_t),
		               style) == NULL)
		{
			vkk_vgContextUSB2Poly_delete(&usb2_poly);
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

/***********************************************************
* public                                                   *
***********************************************************/

vkk_vgContext_t* vkk_vgContext_new(vkk_renderer_t* rend)
{
	ASSERT(rend);

	vkk_engine_t* engine = rend->engine;

	vkk_vgContext_t* self;
	self = (vkk_vgContext_t*)
	       CALLOC(1, sizeof(vkk_vgContext_t));
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
		// layout(location=0) in vec4 xy;
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
	self->list_usb1[1] = cc_list_new();
	self->stack_usb1   = cc_list_new();
	if((self->list_usb1[0] == NULL) ||
	   (self->list_usb1[1] == NULL) ||
	   (self->stack_usb1   == NULL))
	{
		goto fail_list_usb1;
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

	self->list_usb3_line[0] = cc_list_new();
	self->list_usb3_line[1] = cc_list_new();
	if((self->list_usb3_line[0] == NULL) ||
	   (self->list_usb3_line[1] == NULL))
	{
		goto fail_list_usb3_line;
	}

	// success
	return self;

	// failure
	fail_list_usb3_line:
		cc_map_delete(&self->map_usb2_poly);
	fail_map_usb2_poly:
		cc_map_delete(&self->map_usb2_line);
	fail_map_usb2_line:
		cc_list_delete(&self->stack_usb1);
		cc_list_delete(&self->list_usb1[1]);
		cc_list_delete(&self->list_usb1[0]);
	fail_list_usb1:
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

void vkk_vgContext_delete(vkk_vgContext_t** _self)
{
	ASSERT(_self);

	vkk_vgContext_t* self = *_self;
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
			vkk_vgContextUSB3Line_t* usb3_line;
			usb3_line = (vkk_vgContextUSB3Line_t*)
			            cc_list_remove(self->list_usb3_line[0],
			                           &iter);
			vkk_vgContextUSB3Line_delete(&usb3_line);
		}

		cc_mapIter_t* miter;
		miter = cc_map_head(self->map_usb2_poly);
		while(miter)
		{
			vkk_vgContextUSB2Poly_t* usb2_poly;
			usb2_poly = (vkk_vgContextUSB2Poly_t*)
			            cc_map_remove(self->map_usb2_poly, &miter);
			vkk_vgContextUSB2Poly_delete(&usb2_poly);
		}

		miter = cc_map_head(self->map_usb2_line);
		while(miter)
		{
			vkk_vgContextUSB2Line_t* usb2_line;
			usb2_line = (vkk_vgContextUSB2Line_t*)
			            cc_map_remove(self->map_usb2_line, &miter);
			vkk_vgContextUSB2Line_delete(&usb2_line);
		}

		iter = cc_list_head(self->list_usb1[0]);
		while(iter)
		{
			vkk_vgContextUSB1_t* usb1;
			usb1 = (vkk_vgContextUSB1_t*)
			       cc_list_remove(self->list_usb1[0], &iter);
			vkk_vgContextUSB1_delete(&usb1);
		}

		cc_list_delete(&self->list_usb3_line[1]);
		cc_list_delete(&self->list_usb3_line[0]);
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

void vkk_vgContext_reset(vkk_vgContext_t* self,
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
}

void vkk_vgContext_bindLines(vkk_vgContext_t* self)
{
	ASSERT(self);

	vkk_renderer_bindGraphicsPipeline(self->rend,
	                                  self->gp_line);
	vkk_renderer_bindUniformSets(self->rend, 1,
	                             &self->us0);
}

void vkk_vgContext_bindPolygons(vkk_vgContext_t* self)
{
	ASSERT(self);

	vkk_renderer_bindGraphicsPipeline(self->rend,
	                                  self->gp_poly);
	vkk_renderer_bindUniformSets(self->rend, 1,
	                             &self->us0);
}

int vkk_vgContext_pushMatrix(vkk_vgContext_t* self,
                             cc_mat4f_t* mvm)
{
	ASSERT(self);
	ASSERT(mvm);

	// try to use an existing mvm
	// or fall back to create a new mvm
	vkk_vgContextUSB1_t* usb1;
	cc_listIter_t*       iter;
	cc_listIter_t*       siter;
	iter = cc_list_head(self->list_usb1[0]);
	if(iter)
	{
		usb1 = (vkk_vgContextUSB1_t*) cc_list_peekIter(iter);

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
		usb1 = vkk_vgContextUSB1_new(self);
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
		vkk_vgContextUSB1_delete(&usb1);
	return 0;
}

void vkk_vgContext_popMatrix(vkk_vgContext_t* self)
{
	ASSERT(self);

	cc_listIter_t* iter = cc_list_head(self->stack_usb1);
	if(iter)
	{
		cc_list_remove(self->stack_usb1, &iter);
	}
}
