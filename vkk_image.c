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

#include <assert.h>
#include <stdlib.h>

#define LOG_TAG "vkk"
#include "../libcc/cc_log.h"
#include "../libcc/cc_memory.h"
#include "vkk_engine.h"
#include "vkk_image.h"
#include "vkk_memoryManager.h"
#include "vkk_util.h"

/***********************************************************
* public                                                   *
***********************************************************/

vkk_image_t* vkk_image_new(vkk_engine_t* engine,
                           uint32_t width,
                           uint32_t height,
                           int format,
                           int mipmap,
                           int stage,
                           const void* pixels)
{
	// pixels may be NULL for depth image or
	// offscreen rendering
	assert(engine);

	// check if mipmapped images are a power-of-two
	// and compute the mip_levels
	uint32_t mip_levels = 1;
	if(mipmap)
	{
		// mipmap does not apply to the depth image
		assert(format != VKK_IMAGE_FORMAT_DEPTH);

		uint32_t w = 1;
		uint32_t h = 1;
		uint32_t n = 1;
		uint32_t m = 1;

		while(w < width)
		{
			w *= 2;
			n += 1;
		}

		while(h < height)
		{
			h *= 2;
			m += 1;
		}

		if((w != width) || (h != height))
		{
			LOGE("invalid w=%u, width=%u, h=%u, height=%u",
			     w, width, h, height);
			return NULL;
		}

		mip_levels = (m > n) ? m : n;
	}

	vkk_image_t* self;
	self = (vkk_image_t*) CALLOC(1, sizeof(vkk_image_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->engine = engine;

	self->layout_array = (VkImageLayout*)
	                     CALLOC(mip_levels, sizeof(VkImageLayout));
	if(self->layout_array == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_layout_array;
	}

	self->width      = width;
	self->height     = height;
	self->format     = format;
	self->stage      = stage;
	self->mip_levels = mip_levels;

	// initialize the image layout
	int i;
	for(i = 0; i < mip_levels; ++i)
	{
		self->layout_array[i] = VK_IMAGE_LAYOUT_UNDEFINED;
	}

	VkImageUsageFlags  usage      = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	                                VK_IMAGE_USAGE_SAMPLED_BIT;
	VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	if(format == VKK_IMAGE_FORMAT_DEPTH)
	{
		usage      = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT |
		             VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else
	{
		if(mip_levels > 1)
		{
			// mip levels are generated iteratively by blitting
			usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}

		if(pixels == NULL)
		{
			// enable render-to-texture
			usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}
	}

	VkImageCreateInfo i_info =
	{
		.sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext       = NULL,
		.flags       = 0,
		.imageType   = VK_IMAGE_TYPE_2D,
		.format      = vkk_util_imageFormat(format),
		.extent      =
		{
			width,
			height,
			1
		},
		.mipLevels   = mip_levels,
		.arrayLayers = 1,
		.samples     = VK_SAMPLE_COUNT_1_BIT,
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
		goto fail_create_image;
	}

	self->memory = vkk_memoryManager_allocImage(engine->mm,
	                                            self->image);
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
		.format     = vkk_util_imageFormat(format),
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
			.levelCount     = mip_levels,
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

	// upload pixel data
	if(pixels && (format != VKK_IMAGE_FORMAT_DEPTH))
	{
		if(vkk_engine_uploadImage(engine, self, pixels) == 0)
		{
			goto fail_upload;
		}
	}

	// success
	return self;

	// failure
	fail_upload:
		vkDestroyImageView(engine->device, self->image_view,
		                   NULL);
	fail_image_view:
		vkk_memoryManager_free(engine->mm, &self->memory);
	fail_alloc:
		vkDestroyImage(engine->device,
		               self->image, NULL);
	fail_create_image:
		FREE(self->layout_array);
	fail_layout_array:
		FREE(self);
	return NULL;
}

void vkk_image_delete(vkk_image_t** _self)
{
	assert(_self);

	vkk_image_t* self = *_self;
	if(self)
	{
		vkk_engine_deleteObject(self->engine,
		                        VKK_OBJECT_TYPE_IMAGE,
		                        (void*) self);
		*_self = NULL;
	}
}

int vkk_image_format(vkk_image_t* self)
{
	assert(self);

	return self->format;
}

size_t vkk_image_size(vkk_image_t* self,
                      uint32_t* _width, uint32_t* _height)
{
	assert(self);
	assert(_width);
	assert(_height);

	size_t bpp[VKK_IMAGE_FORMAT_COUNT] =
	{
		4, // VKK_IMAGE_FORMAT_RGBA8888
		2, // VKK_IMAGE_FORMAT_RGBA4444
		3, // VKK_IMAGE_FORMAT_RGB888
		2, // VKK_IMAGE_FORMAT_RGB565
		2, // VKK_IMAGE_FORMAT_RG88
		1, // VKK_IMAGE_FORMAT_R8
		4, // VKK_IMAGE_FORMAT_DEPTH
	};

	*_width  = self->width;
	*_height = self->height;
	return self->width*self->height*bpp[self->format];
}
