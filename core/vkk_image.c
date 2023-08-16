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
#include "vkk_image.h"
#include "vkk_memoryManager.h"
#include "vkk_util.h"

/***********************************************************
* protected                                                *
***********************************************************/

int vkk_image_createSemaphore(vkk_image_t* self)
{
	ASSERT(self);

	vkk_engine_t* engine = self->engine;

	VkSemaphoreCreateInfo sa_info =
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0
	};

	if(vkCreateSemaphore(engine->device, &sa_info, NULL,
	                     &self->semaphore) != VK_SUCCESS)
	{
		LOGE("vkCreateSemaphore failed");
		return 0;
	}

	return 1;
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_image_t* vkk_image_new(vkk_engine_t* engine,
                           uint32_t width,
                           uint32_t height,
                           uint32_t depth,
                           vkk_imageFormat_e format,
                           int mipmap,
                           vkk_stage_e stage,
                           const void* pixels)
{
	// pixels may be NULL for depth image or
	// image rendering
	ASSERT(engine);

	// check if mipmapped images are a power-of-two
	// and compute the mip_levels
	uint32_t mip_levels = 1;
	if(mipmap)
	{
		// mipmap does not apply to the depth image
		ASSERT(format != VKK_IMAGE_FORMAT_DEPTH1X);
		ASSERT(format != VKK_IMAGE_FORMAT_DEPTH4X);

		uint32_t w = 1;
		uint32_t h = 1;
		uint32_t d = 1;
		uint32_t n = 1;
		uint32_t m = 1;
		uint32_t o = 1;

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

		while(d < depth)
		{
			d *= 2;
			o += 1;
		}

		if((w != width) || (h != height) || (d != depth))
		{
			LOGE("invalid w=%u, width=%u, h=%u, height=%u, d=%u, depth=%u",
			     w, width, h, height, d, depth);
			return NULL;
		}

		// get the maximum mip levels
		if(m > n)
		{
			mip_levels = (m > o) ? m : o;
		}
		else
		{
			mip_levels = (n > o) ? n : o;
		}
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
	self->depth      = depth;
	self->format     = format;
	self->mipmap     = mipmap;
	self->stage      = stage;
	self->mip_levels = mip_levels;

	// initialize the image layout
	int i;
	for(i = 0; i < mip_levels; ++i)
	{
		self->layout_array[i] = VK_IMAGE_LAYOUT_UNDEFINED;
	}

	// note that the depth images are allocated as transient
	// buffers with the local_memory flag which allows the
	// allocation to be performed in tiled memory
	VkImageUsageFlags     usage;
	VkImageAspectFlags    aspectMask;
	VkSampleCountFlagBits samples;
	usage      = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
	             VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	             VK_IMAGE_USAGE_SAMPLED_BIT;
	aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	samples    = VK_SAMPLE_COUNT_1_BIT;

	#ifdef ANDROID
	int local_memory = 0;
	#else
	int local_memory = 1;
	#endif

	if(format == VKK_IMAGE_FORMAT_DEPTH1X)
	{
		usage        = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
		               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		aspectMask   = VK_IMAGE_ASPECT_DEPTH_BIT |
		               VK_IMAGE_ASPECT_STENCIL_BIT;
		local_memory = 1;
	}
	else if(format == VKK_IMAGE_FORMAT_DEPTH4X)
	{
		usage        = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
		               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		aspectMask   = VK_IMAGE_ASPECT_DEPTH_BIT |
		               VK_IMAGE_ASPECT_STENCIL_BIT;
		samples      = VK_SAMPLE_COUNT_4_BIT;
		local_memory = 1;
	}
	else
	{
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
		.imageType   = (depth == 1) ? VK_IMAGE_TYPE_2D :
		                              VK_IMAGE_TYPE_3D,
		.format      = vkk_util_imageFormat(format),
		.extent      =
		{
			.width  = width,
			.height = height,
			.depth  = depth
		},
		.mipLevels   = mip_levels,
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
		goto fail_create_image;
	}

	self->memory =
		vkk_memoryManager_allocImage(engine->mm,
		                             self->image,
		                             local_memory);
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
		.viewType   = (depth == 1) ? VK_IMAGE_VIEW_TYPE_2D :
		                             VK_IMAGE_VIEW_TYPE_3D,
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
	if((format == VKK_IMAGE_FORMAT_DEPTH1X) ||
	   (format == VKK_IMAGE_FORMAT_DEPTH4X))
	{
		// skip depth
	}
	else if(pixels)
	{
		if(vkk_xferManager_writeImage(engine->xfer, self,
		                              pixels) == 0)
		{
			goto fail_upload;
		}
	}

	// semaphore is created on demand and is used for
	// synchronization with the image stream renderer
	self->semaphore = VK_NULL_HANDLE;

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
	ASSERT(_self);

	vkk_image_t* self = *_self;
	if(self)
	{
		vkk_engine_t* engine = self->engine;

		vkDestroySemaphore(engine->device,
		                   self->semaphore,
		                   NULL);
		vkk_engine_deleteObject(self->engine,
		                        VKK_OBJECT_TYPE_IMAGE,
		                        (void*) self);
		*_self = NULL;
	}
}

vkk_imageFormat_e vkk_image_format(vkk_image_t* self)
{
	ASSERT(self);

	return self->format;
}

size_t vkk_image_size(vkk_image_t* self,
                      uint32_t* _width,
                      uint32_t* _height,
                      uint32_t* _depth)
{
	ASSERT(self);
	ASSERT(_width);
	ASSERT(_height);
	ASSERT(_depth);

	size_t bpp[VKK_IMAGE_FORMAT_COUNT] =
	{
		4,  // VKK_IMAGE_FORMAT_RGBA8888
		2,  // VKK_IMAGE_FORMAT_RGBA4444
		16, // VKK_IMAGE_FORMAT_RGBAF32
		8,  // VKK_IMAGE_FORMAT_RGBAF16
		3,  // VKK_IMAGE_FORMAT_RGB888
		2,  // VKK_IMAGE_FORMAT_RGB565
		12, // VKK_IMAGE_FORMAT_RGBF32
		6,  // VKK_IMAGE_FORMAT_RGBF16
		2,  // VKK_IMAGE_FORMAT_RG88
		8,  // VKK_IMAGE_FORMAT_RGF32
		4,  // VKK_IMAGE_FORMAT_RGF16
		1,  // VKK_IMAGE_FORMAT_R8
		4,  // VKK_IMAGE_FORMAT_RF32
		2,  // VKK_IMAGE_FORMAT_RF16
		4,  // VKK_IMAGE_FORMAT_DEPTH1X
		16, // VKK_IMAGE_FORMAT_DEPTH4X
	};

	*_width  = self->width;
	*_height = self->height;
	*_depth  = self->depth;
	return self->width*self->height*self->depth*
	       bpp[self->format];
}

int vkk_image_readPixels(vkk_image_t* self,
                         void* pixels)
{
	ASSERT(self);
	ASSERT(pixels);

	vkk_engine_t* engine = self->engine;

	return vkk_xferManager_readImage(engine->xfer,
	                                 self, pixels);
}
