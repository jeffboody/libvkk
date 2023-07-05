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
#include <string.h>

#define LOG_TAG "vkk"
#include "../../libcc/cc_log.h"
#include "../../libcc/cc_memory.h"
#include "vkk_engine.h"
#include "vkk_uniformSetFactory.h"

/***********************************************************
* public                                                   *
***********************************************************/

vkk_uniformSetFactory_t*
vkk_uniformSetFactory_new(vkk_engine_t* engine,
                          vkk_updateMode_e update,
                          uint32_t ub_count,
                          vkk_uniformBinding_t* ub_array)
{
	ASSERT(engine);
	ASSERT(ub_array);

	VkDescriptorType dt_map[VKK_UNIFORM_TYPE_COUNT] =
	{
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	};

	VkShaderStageFlags ss_map[VKK_STAGE_COUNT] =
	{
		0,
		VK_SHADER_STAGE_VERTEX_BIT,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		VK_SHADER_STAGE_COMPUTE_BIT,
	};

	vkk_uniformSetFactory_t* self;
	self = (vkk_uniformSetFactory_t*)
	       CALLOC(1, sizeof(vkk_uniformSetFactory_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->engine   = engine;
	self->update   = update;
	self->ub_count = ub_count;

	// copy the ub_array
	self->ub_array = (vkk_uniformBinding_t*)
	                 CALLOC(ub_count,
	                        sizeof(vkk_uniformBinding_t));
	if(self->ub_array == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_ub_array;
	}
	memcpy(self->ub_array, ub_array,
	       ub_count*sizeof(vkk_uniformBinding_t));

	// create temportary descriptor set layout bindings
	VkDescriptorSetLayoutBinding* bindings;
	bindings = (VkDescriptorSetLayoutBinding*)
	           CALLOC(ub_count,
	                  sizeof(VkDescriptorSetLayoutBinding));
	if(bindings == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_bindings;
	}

	// fill in bindings
	int i;
	VkSampler* samplerp;
	for(i = 0; i < ub_count; ++i)
	{
		vkk_uniformBinding_t* ub = &(ub_array[i]);

		if((ub->type == VKK_UNIFORM_TYPE_IMAGE) ||
		   (ub->type == VKK_UNIFORM_TYPE_IMAGE_REF))
		{
			samplerp = vkk_engine_getSamplerp(engine, &ub->si);
			if(samplerp == NULL)
			{
				goto fail_sampler;
			}
		}
		else
		{
			samplerp = NULL;
		}

		VkDescriptorSetLayoutBinding* b = &(bindings[i]);
		b->binding            = ub->binding;
		b->descriptorType     = dt_map[ub->type];
		b->descriptorCount    = 1;
		b->stageFlags         = ss_map[ub->stage];
		b->pImmutableSamplers = samplerp;
	}

	VkDescriptorSetLayoutCreateInfo dsl_info =
	{
		.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext        = NULL,
		.flags        = 0,
		.bindingCount = ub_count,
		.pBindings    = bindings,
	};

	if(vkCreateDescriptorSetLayout(engine->device,
	                               &dsl_info, NULL,
	                               &self->ds_layout) != VK_SUCCESS)
	{
		LOGE("vkCreateDescriptorSetLayout failed");
		goto fail_create_dsl;
	}

	self->dp_list = cc_list_new();
	if(self->dp_list == NULL)
	{
		goto fail_dp_list;
	}

	self->us_list = cc_list_new();
	if(self->us_list == NULL)
	{
		goto fail_us_list;
	}

	// increment type counter
	for(i = 0; i < ub_count; ++i)
	{
		++self->type_count[ub_array[i].type];
	}

	FREE(bindings);

	vkk_uniformSetFactory_incRef(self);

	// success
	return self;

	// failure
	fail_us_list:
		cc_list_delete(&self->dp_list);
	fail_dp_list:
		vkDestroyDescriptorSetLayout(engine->device,
		                             self->ds_layout, NULL);
	fail_create_dsl:
	fail_sampler:
		FREE(bindings);
	fail_bindings:
		FREE(self->ub_array);
	fail_ub_array:
		FREE(self);
	return NULL;
}

void
vkk_uniformSetFactory_delete(vkk_uniformSetFactory_t** _self)
{
	ASSERT(_self);

	vkk_uniformSetFactory_t* self = *_self;
	if(self)
	{
		vkk_uniformSetFactory_decRef(self);
		*_self = NULL;
	}
}

/***********************************************************
* protected                                                *
***********************************************************/

void vkk_uniformSetFactory_incRef(vkk_uniformSetFactory_t* self)
{
	ASSERT(self);

	vkk_engine_usfLock(self->engine);
	++self->ref_count;
	vkk_engine_usfUnlock(self->engine);
}

void vkk_uniformSetFactory_decRef(vkk_uniformSetFactory_t* self)
{
	ASSERT(self);

	vkk_engine_usfLock(self->engine);
	int ref_count = --self->ref_count;
	vkk_engine_usfUnlock(self->engine);

	if(ref_count == 0)
	{
		vkk_engine_deleteObject(self->engine,
		                        VKK_OBJECT_TYPE_UNIFORMSETFACTORY,
		                        (void*) self);
	}
}
