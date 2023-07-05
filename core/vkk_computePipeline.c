/*
 * Copyright (c) 2023 Jeff Boody
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
#include "vkk_engine.h"
#include "vkk_computePipeline.h"
#include "vkk_pipelineLayout.h"
#include "vkk_renderer.h"

/***********************************************************
* public                                                   *
***********************************************************/

vkk_computePipeline_t*
vkk_computePipeline_new(vkk_engine_t* engine,
                        vkk_computePipelineInfo_t* cpi)
{
	ASSERT(engine);
	ASSERT(cpi);

	VkShaderModule cs;
	cs = vkk_engine_getShaderModule(engine, cpi->cs);
	if(cs == VK_NULL_HANDLE)
	{
		return NULL;
	}

	vkk_computePipeline_t* self;
	self = (vkk_computePipeline_t*)
	       CALLOC(1, sizeof(vkk_computePipeline_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}
	self->engine  = engine;
	self->compute = cpi->compute;
	self->pl      = cpi->pl;

	VkComputePipelineCreateInfo cp_info =
	{
		.sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext              = NULL,
		.flags              = 0,
		.stage              =
		{
			.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext               = NULL,
			.flags               = 0,
			.stage               = VK_SHADER_STAGE_COMPUTE_BIT,
			.module              = cs,
			.pName               = "main",
			.pSpecializationInfo = NULL,
		},
		.layout             = cpi->pl->pl,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex  = -1,
	};

	if(vkCreateComputePipelines(engine->device,
	                            engine->pipeline_cache,
	                            1, &cp_info, NULL,
	                            &self->pipeline) != VK_SUCCESS)
	{
		LOGE("vkCreateComputePipelines failed");
		goto fail_create;
	}

	// success
	return self;

	// failure
	fail_create:
		FREE(self);
	return NULL;
}

void vkk_computePipeline_delete(vkk_computePipeline_t** _self)
{
	ASSERT(_self);

	vkk_computePipeline_t* self = *_self;
	if(self)
	{
		vkk_engine_deleteObject(self->engine,
		                        VKK_OBJECT_TYPE_COMPUTEPIPELINE,
		                        (void*) self);
		*_self = NULL;
	}
}
