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
#include "vkk.h"
#include "vkk_engine.h"
#include "vkk_util.h"

/***********************************************************
* public                                                   *
***********************************************************/

void vkk_util_imageMemoryBarrier(vkk_image_t* image,
                                 VkCommandBuffer cb,
                                 VkImageLayout newLayout,
                                 uint32_t baseMipLevel,
                                 uint32_t levelCount)
{
	assert(image);
	assert(cb != VK_NULL_HANDLE);

	// add the memory barrier
	VkImageLayout oldLayout;
	oldLayout = image->layout_array[baseMipLevel];
	vkk_util_imageMemoryBarrierRaw(image->image,
	                               cb,
	                               image->stage,
	                               oldLayout,
	                               newLayout,
	                               baseMipLevel,
	                               levelCount);

	// save the new layout
	int i;
	for(i = baseMipLevel; i < levelCount; ++i)
	{
		image->layout_array[i] = newLayout;
	}
}

void vkk_util_imageMemoryBarrierRaw(VkImage image,
                                    VkCommandBuffer cb,
                                    int stage,
                                    VkImageLayout oldLayout,
                                    VkImageLayout newLayout,
                                    uint32_t baseMipLevel,
                                    uint32_t levelCount)
{
	assert(image != VK_NULL_HANDLE);
	assert(cb != VK_NULL_HANDLE);

	// check for desired layout
	if(newLayout == oldLayout)
	{
		return;
	}

	// derive the aspect mask from the layout
	VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	if(newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT |
		             VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	VkImageMemoryBarrier imb =
	{
		.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext               = NULL,
		.srcAccessMask       = 0,
		.dstAccessMask       = 0,
		.oldLayout           = oldLayout,
		.newLayout           = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image               = image,
		.subresourceRange    =
		{
			.aspectMask     = aspectMask,
			.baseMipLevel   = baseMipLevel,
			.levelCount     = levelCount,
			.baseArrayLayer = 0,
			.layerCount     = 1
		}
	};

	VkPipelineStageFlagBits ps_map[VKK_STAGE_COUNT] =
	{
		0,
		VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	};


	// derive the access masks and stage flags from the
	// image layouts
	VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED)
	{
		srcStageMask      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		imb.srcAccessMask = 0;
	}
	else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		srcStageMask      = VK_PIPELINE_STAGE_TRANSFER_BIT;
		imb.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}
	else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		srcStageMask      = VK_PIPELINE_STAGE_TRANSFER_BIT;
		imb.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	else if(oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		srcStageMask      = ps_map[stage];
		imb.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}
	else if(oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		srcStageMask      = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		imb.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}
	else
	{
		LOGW("invalid oldLayout=%u", oldLayout);
		return;
	}

	if(newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		dstStageMask      = VK_PIPELINE_STAGE_TRANSFER_BIT;
		imb.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	else if(newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		dstStageMask      = VK_PIPELINE_STAGE_TRANSFER_BIT;
		imb.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}
	else if(newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		dstStageMask      = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		imb.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}
	else if(newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		dstStageMask      = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		imb.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}
	else if(newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		dstStageMask      = ps_map[stage];
		imb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}
	else
	{
		LOGW("invalid newLayout=%u", newLayout);
		return;
	}

	vkCmdPipelineBarrier(cb, srcStageMask, dstStageMask,
	                     0, 0, NULL, 0, NULL, 1, &imb);
}

size_t vkk_util_imageSize(vkk_image_t* self)
{
	assert(self);

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

	return self->width*self->height*bpp[self->format];
}

VkFormat vkk_util_imageFormat(int format)
{
	VkFormat format_map[VKK_IMAGE_FORMAT_COUNT] =
	{
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_R4G4B4A4_UNORM_PACK16,
		VK_FORMAT_R8G8B8_UNORM,
		VK_FORMAT_R5G6B5_UNORM_PACK16,
		VK_FORMAT_R8G8_UNORM,
		VK_FORMAT_R8_UNORM,
		VK_FORMAT_D24_UNORM_S8_UINT,
	};

	return format_map[format];
}
