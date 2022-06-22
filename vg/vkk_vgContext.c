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
	vkk_buffer_t*     ub10_color;
	vkk_uniformSet_t* us1;
} vkk_vgContextUs1_t;

typedef struct
{
	vkk_buffer_t*     ub20_dist;
	vkk_uniformSet_t* us2;
} vkk_vgContextUs2_t;

static vkk_vgContextUs1_t*
vkk_vgContextUs1_new(vkk_vgContext_t* ctx,
                     cc_vec4f_t* color)
{
	ASSERT(ctx);
	ASSERT(color);

	vkk_renderer_t* rend   = ctx->rend;
	vkk_engine_t*   engine = rend->engine;

	vkk_vgContextUs1_t* self;
	self = (vkk_vgContextUs1_t*)
	       CALLOC(1, sizeof(vkk_vgContextUs1_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->ub10_color = vkk_buffer_new(engine,
	                                  VKK_UPDATE_MODE_STATIC,
	                                  VKK_BUFFER_USAGE_UNIFORM,
	                                  sizeof(cc_vec4f_t),
	                                  color);
	if(self->ub10_color == NULL)
	{
		goto fail_ub10;
	}

	vkk_uniformAttachment_t ua_array[] =
	{
		// layout(std140, set=1, binding=0) uniform uniformColor
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.buffer  = self->ub10_color
		},
	};

	self->us1 = vkk_uniformSet_new(rend->engine, 1, 1,
	                               ua_array, ctx->usf1);
	if(self->us1 == NULL)
	{
		goto fail_us1;
	}

	// success
	return self;

	// failure
	fail_us1:
		vkk_buffer_delete(&self->ub10_color);
	fail_ub10:
		FREE(self);
	return NULL;
}

static void
vkk_vgContextUs1_delete(vkk_vgContextUs1_t** _self)
{
	ASSERT(_self);

	vkk_vgContextUs1_t* self = *_self;
	if(self)
	{
		vkk_uniformSet_delete(&self->us1);
		vkk_buffer_delete(&self->ub10_color);
		FREE(self);
		*_self = NULL;
	}
}

static vkk_vgContextUs2_t*
vkk_vgContextUs2_new(vkk_vgContext_t* ctx, float dist)
{
	ASSERT(ctx);
	ASSERT(style);

	vkk_renderer_t* rend   = ctx->rend;
	vkk_engine_t*   engine = rend->engine;

	vkk_vgContextUs2_t* self;
	self = (vkk_vgContextUs2_t*)
	       CALLOC(1, sizeof(vkk_vgContextUs2_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->ub20_dist = vkk_buffer_new(engine,
	                                 vkk_renderer_updateMode(rend),
	                                 VKK_BUFFER_USAGE_UNIFORM,
	                                 sizeof(float),
	                                 &dist);
	if(self->ub20_dist == NULL)
	{
		goto fail_ub20;
	}

	vkk_uniformAttachment_t ua_array[] =
	{
		// layout(std140, set=2, binding=0) uniform uniformDist
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.buffer  = self->ub20_dist
		},
	};

	self->us2 = vkk_uniformSet_new(engine, 2, 1,
	                               ua_array, ctx->usf2);
	if(self->us2 == NULL)
	{
		goto fail_us2;
	}

	// success
	return self;

	// failure
	fail_us2:
		vkk_buffer_delete(&self->ub20_dist);
	fail_ub20:
		FREE(self);
	return NULL;
}

static void
vkk_vgContextUs2_delete(vkk_vgContextUs2_t** _self)
{
	ASSERT(_self);

	vkk_vgContextUs2_t* self = *_self;
	if(self)
	{
		vkk_uniformSet_delete(&self->us2);
		vkk_buffer_delete(&self->ub20_dist);
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

	vkk_renderer_t* rend   = self->rend;
	vkk_engine_t*   engine = rend->engine;

	vkk_vgContextUs1_t* cus1;
	vkk_vgContextUs2_t* cus2;

	// get us1
	cc_mapIter_t* miter;
	miter = cc_map_findp(self->map_us1_color,
	                     sizeof(cc_vec4f_t),
	                     &style->color);
	if(miter)
	{
		cus1 = (vkk_vgContextUs1_t*) cc_map_val(miter);
	}
	else
	{
		cus1 = vkk_vgContextUs1_new(self, &style->color);
		if(cus1 == NULL)
		{
			return 0;
		}

		if(cc_map_addp(self->map_us1_color, cus1,
		               sizeof(cc_vec4f_t),
		               &style->color) == NULL)
		{
			vkk_vgContextUs1_delete(&cus1);
			return 0;
		}
	}

	// get ub21_brush12WidthCap
	cc_vec4f_t key_ub21 =
	{
		.x = style->brush1,
		.y = style->brush2,
		.z = style->width,
		.w = (float) style->cap
	};
	vkk_buffer_t* ub21_brush12WidthCap;
	miter = cc_map_findp(self->map_ub21_brush12WidthCap,
	                     sizeof(cc_vec4f_t), &key_ub21);
	if(miter)
	{
		ub21_brush12WidthCap = (vkk_buffer_t*)
		                       cc_map_val(miter);
	}
	else
	{
		ub21_brush12WidthCap = vkk_buffer_new(engine,
		                                      VKK_UPDATE_MODE_STATIC,
		                                      VKK_BUFFER_USAGE_UNIFORM,
		                                      sizeof(cc_vec4f_t),
		                                      &key_ub21);
		if(ub21_brush12WidthCap == NULL)
		{
			return 0;
		}

		if(cc_map_addp(self->map_ub21_brush12WidthCap, ub21_brush12WidthCap,
		               sizeof(cc_vec4f_t),
		               &key_ub21) == NULL)
		{
			vkk_buffer_delete(&ub21_brush12WidthCap);
			return 0;
		}
	}

	// get us2
	cc_listIter_t* iter;
	iter = cc_list_head(self->list_us2[0]);
	if(iter)
	{
		cus2 = (vkk_vgContextUs2_t*)
		       cc_list_peekIter(iter);
		cc_list_swapn(self->list_us2[0], self->list_us2[1],
		              iter, NULL);
		vkk_renderer_updateBuffer(rend, cus2->ub20_dist,
		                          sizeof(float), &dist);
	}
	else
	{
		cus2 = vkk_vgContextUs2_new(self, dist);
		if(cus2 == NULL)
		{
			return 0;
		}

		if(cc_list_append(self->list_us2[1], NULL,
		                  (const void*) cus2) == NULL)
		{
			vkk_vgContextUs2_delete(&cus2);
			return 0;
		}
	}

	// update ub21_brush12WidthCap reference
	vkk_uniformAttachment_t ua_array[] =
	{
		// layout(std140, set=2, binding=1) uniform uniformBrush12WidthCap
		{
			.binding = 1,
			.type    = VKK_UNIFORM_TYPE_BUFFER_REF,
			.buffer  = ub21_brush12WidthCap
		}
	};
	vkk_renderer_updateUniformSetRefs(rend, cus2->us2, 1,
	                                  ua_array);

	// bind uniform sets
	vkk_uniformSet_t* us_array[] =
	{
		cus1->us1,
		cus2->us2,
	};
	vkk_renderer_bindUniformSets(rend, 2, us_array);

	return 1;
}

int vkk_vgContext_bindPolygon(vkk_vgContext_t* self,
                              vkk_vgPolygonStyle_t* style)
{
	ASSERT(self);
	ASSERT(style);

	vkk_renderer_t* rend = self->rend;

	vkk_vgContextUs1_t* cus1;

	cc_mapIter_t* miter;
	miter = cc_map_findp(self->map_us1_color,
	                     sizeof(cc_vec4f_t),
	                     &style->color);
	if(miter)
	{
		cus1 = (vkk_vgContextUs1_t*) cc_map_val(miter);
	}
	else
	{
		cus1 = vkk_vgContextUs1_new(self, &style->color);
		if(cus1 == NULL)
		{
			return 0;
		}

		if(cc_map_addp(self->map_us1_color, cus1,
		               sizeof(cc_vec4f_t),
		               &style->color) == NULL)
		{
			vkk_vgContextUs1_delete(&cus1);
			return 0;
		}
	}

	vkk_renderer_bindUniformSets(rend, 1, &cus1->us1);

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
		// layout(std140, set=0, binding=0) uniform uniformMvp
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
		// layout(std140, set=1, binding=0) uniform uniformColor
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.stage   = VKK_STAGE_FS,
		},
	};

	self->usf1 = vkk_uniformSetFactory_new(engine,
	                                       VKK_UPDATE_MODE_STATIC,
	                                       1, ub_array1);
	if(self->usf1 == NULL)
	{
		goto fail_usf1;
	}

	vkk_uniformBinding_t ub_array2[] =
	{
		// layout(std140, set=2, binding=0) uniform uniformDist
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_BUFFER,
			.stage   = VKK_STAGE_FS,
		},
		// layout(std140, set=2, binding=1) uniform uniformBrush12WidthCap
		{
			.binding = 1,
			.type    = VKK_UNIFORM_TYPE_BUFFER_REF,
			.stage   = VKK_STAGE_VSFS,
		},
	};

	self->usf2 = vkk_uniformSetFactory_new(engine,
	                                       vkk_renderer_updateMode(rend),
	                                       2, ub_array2);
	if(self->usf2 == NULL)
	{
		goto fail_usf2;
	}

	vkk_uniformSetFactory_t* usf_array[] =
	{
		self->usf0,
		self->usf1,
		self->usf2,
	};

	self->pl = vkk_pipelineLayout_new(engine, 3, usf_array);
	if(self->pl == NULL)
	{
		goto fail_pl;
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
		.pl         = self->pl,
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
		.pl         = self->pl,
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

	self->ub00_mvp = vkk_buffer_new(engine,
	                                vkk_renderer_updateMode(rend),
	                                VKK_BUFFER_USAGE_UNIFORM,
	                                sizeof(cc_mat4f_t), NULL);
	if(self->ub00_mvp == NULL)
	{
		goto fail_ub00_mvp;
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

	self->us0 = vkk_uniformSet_new(engine, 0, 1,
	                               ua_array0, self->usf0);
	if(self->us0 == NULL)
	{
		goto fail_us0;
	}

	self->list_us2[0] = cc_list_new();
	self->list_us2[1] = cc_list_new();
	if((self->list_us2[0] == NULL) ||
	   (self->list_us2[1] == NULL))
	{
		goto fail_list_us2;
	}

	self->map_us1_color = cc_map_new();
	if(self->map_us1_color == NULL)
	{
		goto fail_map_us1_color;
	}

	self->map_ub21_brush12WidthCap = cc_map_new();
	if(self->map_ub21_brush12WidthCap == NULL)
	{
		goto fail_map_ub21_brush12WidthCap;
	}

	// success
	return self;

	// failure
	fail_map_ub21_brush12WidthCap:
		cc_map_delete(&self->map_us1_color);
	fail_map_us1_color:
		cc_list_delete(&self->list_us2[1]);
		cc_list_delete(&self->list_us2[0]);
	fail_list_us2:
		vkk_uniformSet_delete(&self->us0);
	fail_us0:
		vkk_buffer_delete(&self->ub00_mvp);
	fail_ub00_mvp:
		vkk_graphicsPipeline_delete(&self->gp_poly);
	fail_gp_poly:
		vkk_graphicsPipeline_delete(&self->gp_line);
	fail_gp_line:
		vkk_pipelineLayout_delete(&self->pl);
	fail_pl:
		vkk_uniformSetFactory_delete(&self->usf2);
	fail_usf2:
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
		cc_list_appendList(self->list_us2[0],
		                   self->list_us2[1]);

		cc_mapIter_t* miter;
		miter = cc_map_head(self->map_ub21_brush12WidthCap);
		while(miter)
		{
			vkk_buffer_t* tmp;
			tmp = (vkk_buffer_t*)
			      cc_map_remove(self->map_ub21_brush12WidthCap,
			                    &miter);
			vkk_buffer_delete(&tmp);
		}

		miter = cc_map_head(self->map_us1_color);
		while(miter)
		{
			vkk_vgContextUs1_t* tmp;
			tmp = (vkk_vgContextUs1_t*)
			      cc_map_remove(self->map_us1_color,
			                    &miter);
			vkk_vgContextUs1_delete(&tmp);
		}

		cc_listIter_t* iter;
		iter = cc_list_head(self->list_us2[0]);
		while(iter)
		{
			vkk_vgContextUs2_t* tmp;
			tmp = (vkk_vgContextUs2_t*)
			      cc_list_remove(self->list_us2[0], &iter);
			vkk_vgContextUs2_delete(&tmp);
		}

		cc_map_delete(&self->map_ub21_brush12WidthCap);
		cc_map_delete(&self->map_us1_color);
		cc_list_delete(&self->list_us2[1]);
		cc_list_delete(&self->list_us2[0]);

		vkk_uniformSet_delete(&self->us0);
		vkk_buffer_delete(&self->ub00_mvp);
		vkk_graphicsPipeline_delete(&self->gp_poly);
		vkk_graphicsPipeline_delete(&self->gp_line);
		vkk_pipelineLayout_delete(&self->pl);
		vkk_uniformSetFactory_delete(&self->usf2);
		vkk_uniformSetFactory_delete(&self->usf1);
		vkk_uniformSetFactory_delete(&self->usf0);
		FREE(self);
		*_self = NULL;
	}
}

void vkk_vgContext_reset(vkk_vgContext_t* self,
                         cc_mat4f_t* mvp)
{
	ASSERT(self);
	ASSERT(mvp);

	// update mvp
	vkk_renderer_updateBuffer(self->rend, self->ub00_mvp,
	                          sizeof(cc_mat4f_t), mvp);

	// make us2 available to the current frame
	cc_list_appendList(self->list_us2[0],
	                   self->list_us2[1]);
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
