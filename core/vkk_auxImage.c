/*
 * Copyright (c) 2025 Jeff Boody
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
#include "vkk_auxImage.h"
#include "vkk_memoryManager.h"

/***********************************************************
* public                                                   *
***********************************************************/

vkk_auxImage_t*
vkk_auxImage_newDepth(vkk_engine_t* engine,
                      uint32_t width,
                      uint32_t height,
                      int use_msaa)
{
	ASSERT(engine);

	vkk_auxImage_t* self;
	self = (vkk_auxImage_t*) CALLOC(1, sizeof(vkk_auxImage_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->engine = engine;

	VkImageUsageFlags     usage;
	VkImageAspectFlags    aspectMask;
	VkSampleCountFlagBits samples;
	usage      = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
		         VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT |
		         VK_IMAGE_ASPECT_STENCIL_BIT;
	samples    = use_msaa ? VK_SAMPLE_COUNT_4_BIT :
	                        VK_SAMPLE_COUNT_1_BIT;
	VkImageCreateInfo i_info =
	{
		.sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext       = NULL,
		.flags       = 0,
		.imageType   = VK_IMAGE_TYPE_2D,
		.format      = VK_FORMAT_D24_UNORM_S8_UINT,
		.extent      =
		{
			.width  = width,
			.height = height,
			.depth  = 1
		},
		.mipLevels   = 1,
		.arrayLayers = 1,
		.samples     = samples,
		.tiling      = VK_IMAGE_TILING_OPTIMAL,
		.usage       = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices   = &engine->queue_family_index,
		.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED
	};

	if(vkCreateImage(engine->device, &i_info, NULL,
	                 &self->image) != VK_SUCCESS)
	{
		LOGE("vkCreateImage failed");
		return 0;
	}

	// memory is uninitialized
	self->memory = vkk_memoryManager_allocImage(engine->mm,
	                                            self->image,
	                                            1, 1);
	if(self->memory == NULL)
	{
		goto fail_alloc;
	}

	VkImageViewCreateInfo iv_info =
	{
		.sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext      = NULL,
		.flags      = 0,
		.image      = self->image,
		.viewType   = VK_IMAGE_VIEW_TYPE_2D,
		.format     = VK_FORMAT_D24_UNORM_S8_UINT,
		.components =
		{
			.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.a = VK_COMPONENT_SWIZZLE_IDENTITY
		},
		.subresourceRange =
		{
			.aspectMask     = aspectMask,
			.baseMipLevel   = 0,
			.levelCount     = 1,
			.baseArrayLayer = 0,
			.layerCount     = 1
		}
	};

	if(vkCreateImageView(engine->device, &iv_info, NULL,
	                     &self->image_view) != VK_SUCCESS)
	{
		LOGE("vkCreateImageView failed");
		goto fail_image_view;
	}

	// success
	return self;

	// failure
	fail_image_view:
		vkk_memoryManager_free(engine->mm, &self->memory);
	fail_alloc:
		vkDestroyImage(engine->device, self->image, NULL);
	return NULL;
}

vkk_auxImage_t*
vkk_auxImage_newMSAA(vkk_engine_t* engine,
                     uint32_t width,
                     uint32_t height,
                     VkFormat format)
{
	ASSERT(engine);

	vkk_auxImage_t* self;
	self = (vkk_auxImage_t*) CALLOC(1, sizeof(vkk_auxImage_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->engine = engine;

	// when MSAA is enabled
	// 1. create a transient MS image with 4x samples
	// 2. it is important to note that the MS image
	//    sets the local_memory flag which allows the
	//    allocation to be performed in tiled memory
	// 3. the MS image only requires a single backing image
	//    since only one frame is rendered at a time
	VkImageUsageFlags  usage      = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
	                                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	VkImageCreateInfo i_info =
	{
		.sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext       = NULL,
		.flags       = 0,
		.imageType   = VK_IMAGE_TYPE_2D,
		.format      = format,
		.extent      =
		{
			.width  = width,
			.height = height,
			.depth  = 1
		},
		.mipLevels   = 1,
		.arrayLayers = 1,
		.samples     = VK_SAMPLE_COUNT_4_BIT,
		.tiling      = VK_IMAGE_TILING_OPTIMAL,
		.usage       = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices   = &engine->queue_family_index,
		.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED
	};

	if(vkCreateImage(engine->device, &i_info, NULL,
	                 &self->image) != VK_SUCCESS)
	{
		LOGE("vkCreateImage failed");
		return 0;
	}

	// memory is uninitialized
	self->memory = vkk_memoryManager_allocImage(engine->mm,
	                                            self->image,
	                                            1, 1);
	if(self->memory == NULL)
	{
		goto fail_alloc;
	}

	VkImageViewCreateInfo iv_info =
	{
		.sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext      = NULL,
		.flags      = 0,
		.image      = self->image,
		.viewType   = VK_IMAGE_VIEW_TYPE_2D,
		.format     = format,
		.components =
		{
			.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.a = VK_COMPONENT_SWIZZLE_IDENTITY
		},
		.subresourceRange =
		{
			.aspectMask     = aspectMask,
			.baseMipLevel   = 0,
			.levelCount     = 1,
			.baseArrayLayer = 0,
			.layerCount     = 1
		}
	};

	if(vkCreateImageView(engine->device, &iv_info, NULL,
	                     &self->image_view) != VK_SUCCESS)
	{
		LOGE("vkCreateImageView failed");
		goto fail_image_view;
	}

	// success
	return self;

	// failure
	fail_image_view:
		vkk_memoryManager_free(engine->mm, &self->memory);
	fail_alloc:
		vkDestroyImage(engine->device, self->image, NULL);
	return NULL;
}

void vkk_auxImage_delete(vkk_auxImage_t** _self)
{
	ASSERT(_self);

	vkk_auxImage_t* self = *_self;
	if(self)
	{
		vkk_engine_t* engine = self->engine;

		vkDestroyImageView(engine->device, self->image_view,
		                   NULL);
		vkk_memoryManager_free(engine->mm, &self->memory);
		vkDestroyImage(engine->device, self->image, NULL);
		FREE(self);
		*_self = NULL;
	}
}
