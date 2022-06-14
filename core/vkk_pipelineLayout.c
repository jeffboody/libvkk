/*
 * Copyright (c) 2019 Jeff Boody
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
#include "vkk_pipelineLayout.h"
#include "vkk_uniformSetFactory.h"

/***********************************************************
* public                                                   *
***********************************************************/

vkk_pipelineLayout_t*
vkk_pipelineLayout_new(vkk_engine_t* engine,
                       uint32_t usf_count,
                       vkk_uniformSetFactory_t** usf_array)
{
	ASSERT(engine);
	ASSERT(usf_array);

	if(usf_count > VKK_ENGINE_MAX_USF_COUNT)
	{
		LOGE("invalid usf_count=%i", usf_count);
		return NULL;
	}

	vkk_pipelineLayout_t* self;
	self = (vkk_pipelineLayout_t*)
	       CALLOC(1, sizeof(vkk_pipelineLayout_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->engine    = engine;
	self->usf_count = usf_count;

	VkDescriptorSetLayout* dsl_array;
	dsl_array = (VkDescriptorSetLayout*)
	            CALLOC(usf_count, sizeof(VkDescriptorSetLayout));
	if(dsl_array == NULL)
	{
		goto fail_dsl_array;
	}

	int i;
	for(i = 0; i < usf_count; ++i)
	{
		dsl_array[i] = usf_array[i]->ds_layout;
	}

	VkPipelineLayoutCreateInfo pl_info =
	{
		.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext                  = NULL,
		.flags                  = 0,
		.setLayoutCount         = usf_count,
		.pSetLayouts            = dsl_array,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges    = NULL
	};

	if(vkCreatePipelineLayout(engine->device, &pl_info, NULL,
	                          &self->pl) != VK_SUCCESS)
	{
		LOGE("vkCreatePipelineLayout failed");
		goto fail_create_pl;
	}

	FREE(dsl_array);

	// success
	return self;

	// failure
	fail_create_pl:
		FREE(dsl_array);
	fail_dsl_array:
		FREE(self);
	return NULL;
}

void vkk_pipelineLayout_delete(vkk_pipelineLayout_t** _self)
{
	ASSERT(_self);

	vkk_pipelineLayout_t* self = *_self;
	if(self)
	{
		vkk_engine_deleteObject(self->engine,
		                        VKK_OBJECT_TYPE_PIPELINELAYOUT,
		                        (void*) self);
		*_self = NULL;
	}
}
