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
#include "vkk_util.h"

/***********************************************************
* public                                                   *
***********************************************************/

void vkk_imageMemoryBarrier(VkCommandBuffer cb,
                            VkImage image,
                            int stage,
                            VkImageLayout oldLayout,
                            VkImageLayout newLayout,
                            VkImageAspectFlags aspectMask,
                            uint32_t baseMipLevel,
                            uint32_t levelCount)
{
	assert(cb != VK_NULL_HANDLE);
	assert(image != VK_NULL_HANDLE);

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
		// TODO - VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	}
	else if(oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		// TODO - VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	}
	else
	{
		LOGW("invalid oldLayout=%u", oldLayout);
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
		VkPipelineStageFlagBits ps_map[VKK_STAGE_COUNT] =
		{
			0,
			VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		};

		dstStageMask      = ps_map[stage];
		imb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}
	else
	{
		LOGW("invalid newLayout=%u", newLayout);
	}

	vkCmdPipelineBarrier(cb, srcStageMask, dstStageMask,
	                     0, 0, NULL, 0, NULL, 1, &imb);
}
