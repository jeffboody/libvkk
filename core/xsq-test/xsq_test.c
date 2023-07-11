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

#define LOG_TAG "xsq-test"
#include "libcc/cc_log.h"
#include "libcc/rng/cc_rngUniform.h"
#include "libcc/cc_memory.h"
#include "xsq_test.h"

#define XSQ_TEST_COUNT 64

/***********************************************************
* public                                                   *
***********************************************************/

xsq_test_t* xsq_test_new(vkk_engine_t* engine)
{
	ASSERT(engine);

	xsq_test_t* self;
	self = (xsq_test_t*)
	       CALLOC(1, sizeof(xsq_test_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->engine = engine;

	vkk_uniformBinding_t ub_array[] =
	{
		// layout(std140, set=0, binding=0) readonly buffer bufferIn
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_STORAGE,
			.stage   = VKK_STAGE_COMPUTE,
		},
		// layout(std140, set=0, binding=1) writeonly buffer bufferOut
		{
			.binding = 1,
			.type    = VKK_UNIFORM_TYPE_STORAGE,
			.stage   = VKK_STAGE_COMPUTE,
		},
	};

	self->usf = vkk_uniformSetFactory_new(engine,
	                                      VKK_UPDATE_MODE_SYNCHRONOUS,
	                                      2, ub_array);
	if(self->usf == NULL)
	{
		goto fail_usf;
	}

	size_t size = XSQ_TEST_COUNT*sizeof(float);
	self->bufx = vkk_buffer_new(engine,
	                            VKK_UPDATE_MODE_SYNCHRONOUS,
	                            VKK_BUFFER_USAGE_STORAGE,
	                            size, NULL);
	if(self->bufx == NULL)
	{
		goto fail_bufx;
	}

	self->bufxx = vkk_buffer_new(engine,
	                             VKK_UPDATE_MODE_SYNCHRONOUS,
	                             VKK_BUFFER_USAGE_STORAGE,
	                             size, NULL);
	if(self->bufxx == NULL)
	{
		goto fail_bufxx;
	}

	vkk_uniformAttachment_t ua_array[] =
	{
		// layout(std140, set=0, binding=0) readonly buffer bufferIn
		{
			.binding = 0,
			.type    = VKK_UNIFORM_TYPE_STORAGE,
			.buffer  = self->bufx,
		},
		// layout(std140, set=0, binding=1) writeonly buffer bufferOut
		{
			.binding = 1,
			.type    = VKK_UNIFORM_TYPE_STORAGE,
			.buffer  = self->bufxx,
		},
	};

	self->us = vkk_uniformSet_new(engine, 0, 2, ua_array,
	                              self->usf);
	if(self->us == NULL)
	{
		goto fail_us;
	}

	self->pl = vkk_pipelineLayout_new(engine, 1, &self->usf);
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
		.cs      = "shaders/xsq.spv",
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
		vkk_uniformSet_delete(&self->us);
	fail_us:
		vkk_buffer_delete(&self->bufxx);
	fail_bufxx:
		vkk_buffer_delete(&self->bufx);
	fail_bufx:
		vkk_uniformSetFactory_delete(&self->usf);
	fail_usf:
		FREE(self);
	return NULL;
}

void xsq_test_delete(xsq_test_t** _self)
{
	ASSERT(_self);

	xsq_test_t* self = *_self;
	if(self)
	{
		vkk_computePipeline_delete(&self->cp);
		vkk_compute_delete(&self->compute);
		vkk_pipelineLayout_delete(&self->pl);
		vkk_uniformSet_delete(&self->us);
		vkk_buffer_delete(&self->bufxx);
		vkk_buffer_delete(&self->bufx);
		vkk_uniformSetFactory_delete(&self->usf);
		FREE(self);
		*_self = NULL;
	}
}

void xsq_test_main(xsq_test_t* self,
                   int argc, char** argv)
{
	ASSERT(self);
	ASSERT(argv);

	if(vkk_compute_begin(self->compute) == 0)
	{
		return;
	}

	// create rng
	cc_rngUniform_t rng;
	cc_rngUniform_init(&rng);

	// initialize data
	int    i;
	size_t size = XSQ_TEST_COUNT*sizeof(float);
	float  x[XSQ_TEST_COUNT];
	float  xx1[XSQ_TEST_COUNT];
	for(i = 0; i < XSQ_TEST_COUNT; ++i)
	{
		x[i]   = cc_rngUniform_rand2F(&rng, -1.0f, 1.0f);
		xx1[i] = x[i]*x[i];
	}
	vkk_compute_updateBuffer(self->compute, self->bufx,
	                         size, x);

	// compute xsq
	// groupCountX is determined by xsq.comp where local_size_x
	// is 4 so we must divide the count by 4
	vkk_compute_bindComputePipeline(self->compute, self->cp);
	vkk_compute_bindUniformSets(self->compute, 1, &self->us);
	vkk_compute_dispatch(self->compute,
	                     XSQ_TEST_COUNT/4, 1, 1);
	vkk_compute_end(self->compute);

	// read buffer
	float xx2[XSQ_TEST_COUNT];
	vkk_compute_readBuffer(self->compute, self->bufxx,
	                       size, xx2);

	// output results
	for(i = 0; i < XSQ_TEST_COUNT; ++i)
	{
		LOGI("i=%i, x=%f, xx1=%f, xx2=%f",
		     i, x[i], xx1[i], xx2[i]);
	}
}
