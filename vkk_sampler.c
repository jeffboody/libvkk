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
#include "../libcc/cc_log.h"
#include "../libcc/cc_memory.h"
#include "vkk_engine.h"
#include "vkk_sampler.h"

/***********************************************************
* public                                                   *
***********************************************************/

vkk_sampler_t*
vkk_sampler_new(vkk_engine_t* engine, int min_filter,
                int mag_filter, int mipmap_mode)
{
	ASSERT(engine);

	vkk_sampler_t* self;
	self = (vkk_sampler_t*)
	       CALLOC(1, sizeof(vkk_sampler_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->engine = engine;

	VkFilter filter_map[VKK_SAMPLER_FILTER_COUNT] =
	{
		VK_FILTER_NEAREST,
		VK_FILTER_LINEAR
	};

	VkSamplerMipmapMode mipmap_map[VKK_SAMPLER_MIPMAP_MODE_COUNT] =
	{
		VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_MIPMAP_MODE_LINEAR
	};

	// Note: the maxLod represents the maximum number of mip
	// levels that can be supported and is just used to clamp
	// the computed maxLod for a particular texture.
	// A large value for maxLod effectively allows all mip
	// levels to be used for mipmapped textures.
	VkSamplerCreateInfo si =
	{
		.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.pNext                   = NULL,
		.flags                   = 0,
		.magFilter               = filter_map[mag_filter],
		.minFilter               = filter_map[min_filter],
		.mipmapMode              = mipmap_map[mipmap_mode],
		.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.mipLodBias              = 0.0f,
		.anisotropyEnable        = VK_FALSE,
		.maxAnisotropy           = 0.0f,
		.compareEnable           = VK_FALSE,
		.compareOp               = VK_COMPARE_OP_NEVER,
		.minLod                  = 0.0f,
		.maxLod                  = 1024.0f,
		.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
		.unnormalizedCoordinates = VK_FALSE
	};

	if(vkCreateSampler(engine->device, &si, NULL,
	                   &self->sampler) != VK_SUCCESS)
	{
		LOGE("vkCreateSampler failed");
		goto fail_create;
	}

	// success
	return self;

	// failure
	fail_create:
		FREE(self);
	return NULL;
}

void vkk_sampler_delete(vkk_sampler_t** _self)
{
	ASSERT(_self);

	vkk_sampler_t* self = *_self;
	if(self)
	{
		vkk_engine_deleteObject(self->engine,
		                        VKK_OBJECT_TYPE_SAMPLER,
		                        (void*) self);
		*_self = NULL;
	}
}
