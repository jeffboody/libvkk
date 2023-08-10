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

#define LOG_TAG "xsum-test"
#include "libcc/cc_log.h"
#include "libcc/rng/cc_rngUniform.h"
#include "libcc/cc_memory.h"
#include "xsum_test.h"

// see xsum.comp
#define XSUM_TEST_COUNT 1000
#define XSUM_TEST_IN    64

/***********************************************************
* public                                                   *
***********************************************************/

xsum_test_t* xsum_test_new(vkk_engine_t* engine)
{
	ASSERT(engine);

	xsum_test_t* self;
	self = (xsum_test_t*)
	       CALLOC(1, sizeof(xsum_test_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->engine = engine;

	vkk_uniformBinding_t ub_array0[] =
	{
		// layout(std430, set=0, binding=0) readonly buffer sb00
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_STORAGE,
			.stage   = VKK_STAGE_COMPUTE,
		},
		// layout(std430, set=0, binding=1) writeonly buffer sb01
		{
			.binding = 1,
			.type    = VKK_UNIFORM_TYPE_STORAGE,
			.stage   = VKK_STAGE_COMPUTE,
		},
		// layout(std430, set=0, binding=2) readonly buffer sb02
		{
			.binding = 2,
			.type    = VKK_UNIFORM_TYPE_STORAGE,
			.stage   = VKK_STAGE_COMPUTE,
		},
	};

	self->usf0 = vkk_uniformSetFactory_new(engine,
	                                       VKK_UPDATE_MODE_SYNCHRONOUS,
	                                       3, ub_array0);
	if(self->usf0 == NULL)
	{
		goto fail_usf0;
	}

	size_t size = XSUM_TEST_COUNT*sizeof(float);
	self->sb00_x = vkk_buffer_new(engine,
	                              VKK_UPDATE_MODE_SYNCHRONOUS,
	                              VKK_BUFFER_USAGE_STORAGE,
	                              size, NULL);
	if(self->sb00_x == NULL)
	{
		goto fail_sb00_x;
	}

	self->sb01_xsum = vkk_buffer_new(engine,
	                                 VKK_UPDATE_MODE_SYNCHRONOUS,
	                                 VKK_BUFFER_USAGE_STORAGE,
	                                 sizeof(float), NULL);
	if(self->sb01_xsum == NULL)
	{
		goto fail_sb01_xsum;
	}

	self->sb02_count = vkk_buffer_new(engine,
	                                  VKK_UPDATE_MODE_SYNCHRONOUS,
	                                  VKK_BUFFER_USAGE_STORAGE,
	                                  sizeof(uint32_t), NULL);
	if(self->sb02_count == NULL)
	{
		goto fail_sb02_count;
	}

	vkk_uniformAttachment_t ua_array0[] =
	{
		// layout(std430, set=0, binding=0) readonly buffer sb00
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_STORAGE,
			.buffer  = self->sb00_x,
		},
		// layout(std430, set=0, binding=1) writeonly buffer sb01
		{
			.binding = 1,
			.type    = VKK_UNIFORM_TYPE_STORAGE,
			.buffer  = self->sb01_xsum,
		},
		// layout(std430, set=0, binding=2) readonly buffer sb02
		{
			.binding = 2,
			.type    = VKK_UNIFORM_TYPE_STORAGE,
			.buffer  = self->sb02_count,
		},
	};

	self->us0 = vkk_uniformSet_new(engine, 0, 3, ua_array0,
	                               self->usf0);
	if(self->us0 == NULL)
	{
		goto fail_us;
	}

	self->pl = vkk_pipelineLayout_new(engine, 1, &self->usf0);
	if(self->pl == NULL)
	{
		goto fail_pl;
	}

	self->compute = vkk_compute_new(engine);
	if(self->compute == NULL)
	{
		goto fail_compute;
	}

	vkk_computePipelineInfo_t cpi =
	{
		.compute = self->compute,
		.pl      = self->pl,
		.cs      = "shaders/xsum.spv",
	};

	self->cp = vkk_computePipeline_new(engine, &cpi);
	if(self->cp == NULL)
	{
		goto fail_cp;
	}

	// success
	return self;

	// failure
	fail_cp:
		vkk_compute_delete(&self->compute);
	fail_compute:
		vkk_pipelineLayout_delete(&self->pl);
	fail_pl:
		vkk_uniformSet_delete(&self->us0);
	fail_us:
		vkk_buffer_delete(&self->sb02_count);
	fail_sb02_count:
		vkk_buffer_delete(&self->sb01_xsum);
	fail_sb01_xsum:
		vkk_buffer_delete(&self->sb00_x);
	fail_sb00_x:
		vkk_uniformSetFactory_delete(&self->usf0);
	fail_usf0:
		FREE(self);
	return NULL;
}

void xsum_test_delete(xsum_test_t** _self)
{
	ASSERT(_self);

	xsum_test_t* self = *_self;
	if(self)
	{
		vkk_computePipeline_delete(&self->cp);
		vkk_compute_delete(&self->compute);
		vkk_pipelineLayout_delete(&self->pl);
		vkk_uniformSet_delete(&self->us0);
		vkk_buffer_delete(&self->sb02_count);
		vkk_buffer_delete(&self->sb01_xsum);
		vkk_buffer_delete(&self->sb00_x);
		vkk_uniformSetFactory_delete(&self->usf0);
		FREE(self);
		*_self = NULL;
	}
}

int xsum_test_main(xsum_test_t* self,
                   int argc, char** argv)
{
	ASSERT(self);
	ASSERT(argv);

	if(vkk_compute_begin(self->compute) == 0)
	{
		return EXIT_FAILURE;
	}

	// create rng
	cc_rngUniform_t rng;
	cc_rngUniform_init(&rng);

	// initialize data
	int      i;
	size_t   size = XSUM_TEST_COUNT*sizeof(float);
	float    x[XSUM_TEST_COUNT];
	float    xsum1 = 0.0f;
	for(i = 0; i < XSUM_TEST_COUNT; ++i)
	{
		x[i]   = cc_rngUniform_rand2F(&rng, -1.0f, 1.0f);
		xsum1 += x[i];
	}

	vkk_compute_writeBuffer(self->compute, self->sb00_x,
	                        size, 0, x);

	uint32_t count = XSUM_TEST_COUNT;
	vkk_compute_writeBuffer(self->compute, self->sb02_count,
	                        sizeof(uint32_t), 0, &count);

	// compute xsum
	// only dispatch one workgroup to compute the xsum since
	// the invocations must be synchronized using barrier()
	//
	// See "Modern OpenGL Tutorial - Compute Shaders" for a
	// detailed explaination of workgroups and invocations
	// https://www.youtube.com/watch?v=nF4X9BIUzx0
	vkk_compute_bindComputePipeline(self->compute, self->cp);
	vkk_compute_bindUniformSets(self->compute, 1, &self->us0);
	vkk_compute_dispatch(self->compute, VKK_HAZZARD_NONE,
	                     1, 1, 1, XSUM_TEST_IN, 1, 1);
	vkk_compute_end(self->compute);

	// read buffer
	float xsum2;
	vkk_compute_readBuffer(self->compute, self->sb01_xsum,
	                       sizeof(float), 0, &xsum2);

	// output results
	LOGI("xsum1=%f, xsum2=%f", xsum1, xsum2);

	return EXIT_SUCCESS;
}
