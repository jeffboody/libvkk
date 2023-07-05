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
#include "../../libcc/cc_timestamp.h"
#include "vkk_defaultRenderer.h"
#include "vkk_commandBuffer.h"
#include "vkk_engine.h"
#include "vkk_image.h"
#include "vkk_renderer.h"
#include "vkk_secondaryRenderer.h"
#include "vkk_util.h"

/***********************************************************
* public                                                   *
***********************************************************/

vkk_renderer_t*
vkk_secondaryRenderer_new(vkk_renderer_t* executor)
{
	ASSERT(executor);
	ASSERT(executor->type != VKK_RENDERER_TYPE_SECONDARY);
	ASSERT(executor->mode == VKK_RENDERER_MODE_EXECUTE);

	vkk_engine_t* engine = executor->engine;

	vkk_secondaryRenderer_t* self;
	self = (vkk_secondaryRenderer_t*)
	       CALLOC(1, sizeof(vkk_secondaryRenderer_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	vkk_renderer_t* base = &(self->base);
	vkk_renderer_init(base, VKK_RENDERER_TYPE_SECONDARY,
	                  engine);

	self->executor = executor;

	uint32_t swapchain_image_count = 1;
	if(executor->type == VKK_RENDERER_TYPE_DEFAULT)
	{
		vkk_defaultRenderer_t* def;
		def = (vkk_defaultRenderer_t*) executor;
		swapchain_image_count = def->swapchain_image_count;
	}

	self->cmd_buffers = vkk_commandBuffer_new(engine,
	                                          swapchain_image_count,
	                                          1);
	if(self->cmd_buffers == NULL)
	{
		goto fail_cmd_buffers;
	}

	// success
	return (vkk_renderer_t*) self;

	// failure
	fail_cmd_buffers:
		FREE(self);
	return NULL;
}

void vkk_secondaryRenderer_delete(vkk_renderer_t** _base)
{
	ASSERT(_base);

	vkk_renderer_t* base = *_base;
	if(base)
	{
		vkk_secondaryRenderer_t* self;
		self = (vkk_secondaryRenderer_t*) base;

		FREE(base->wait_flags);
		FREE(base->wait_array);

		vkk_commandBuffer_delete(&self->cmd_buffers);
		FREE(self);
		*_base = NULL;
	}
}

int vkk_secondaryRenderer_begin(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_secondaryRenderer_t* self;
	self = (vkk_secondaryRenderer_t*) base;

	uint32_t swapchain_frame;
	swapchain_frame = vkk_renderer_frame(self->executor);

	VkCommandBuffer cb;
	cb = vkk_commandBuffer_get(self->cmd_buffers,
	                           swapchain_frame);
	if(vkResetCommandBuffer(cb, 0) != VK_SUCCESS)
	{
		LOGE("vkResetCommandBuffer failed");
		return 0;
	}

	VkFramebuffer framebuffer;
	VkRenderPass  render_pass;
	framebuffer = vkk_renderer_framebuffer(self->executor);
	render_pass = vkk_renderer_renderPass(self->executor);
	VkCommandBufferInheritanceInfo cbi_info =
	{
		.sType                = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		.pNext                = NULL,
		.renderPass           = render_pass,
		.subpass              = 0,
		.framebuffer          = framebuffer,
		.occlusionQueryEnable = VK_FALSE,
		.queryFlags           = 0,
		.pipelineStatistics   = 0
	};

	VkCommandBufferBeginInfo cb_info =
	{
		.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext            = NULL,
		.flags            = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
		.pInheritanceInfo = &cbi_info
	};

	if(vkBeginCommandBuffer(cb, &cb_info) != VK_SUCCESS)
	{
		LOGE("vkBeginCommandBuffer failed");
		return 0;
	}

	uint32_t width;
	uint32_t height;
	vkk_renderer_surfaceSize(self->executor,
	                         &width, &height);
	VkViewport viewport =
	{
		.x        = 0.0f,
		.y        = 0.0f,
		.width    = (float) width,
		.height   = (float) height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};
	vkCmdSetViewport(cb, 0, 1, &viewport);

	VkRect2D scissor =
	{
		.offset =
		{
			.x = 0,
			.y = 0
		},
		.extent =
		{
			.width  = width,
			.height = height,
		}
	};
	vkCmdSetScissor(cb, 0, 1, &scissor);

	// update timestamp
	self->ts = vkk_renderer_tsCurrent(self->executor);

	return 1;
}

void vkk_secondaryRenderer_end(vkk_renderer_t* base)
{
	ASSERT(base);

	VkCommandBuffer cb;
	cb = vkk_secondaryRenderer_commandBuffer(base);
	vkEndCommandBuffer(cb);
}

void vkk_secondaryRenderer_surfaceSize(vkk_renderer_t* base,
                                       uint32_t* _width,
                                       uint32_t* _height)
{
	ASSERT(base);
	ASSERT(_width);
	ASSERT(_height);

	vkk_secondaryRenderer_t* self;
	self = (vkk_secondaryRenderer_t*) base;

	vkk_renderer_surfaceSize(self->executor,
	                         _width, _height);
}

VkRenderPass
vkk_secondaryRenderer_renderPass(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_secondaryRenderer_t* self;
	self = (vkk_secondaryRenderer_t*) base;

	return vkk_renderer_renderPass(self->executor);
}

VkFramebuffer
vkk_secondaryRenderer_framebuffer(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_secondaryRenderer_t* self;
	self = (vkk_secondaryRenderer_t*) base;

	return vkk_renderer_framebuffer(self->executor);
}

VkCommandBuffer
vkk_secondaryRenderer_commandBuffer(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_secondaryRenderer_t* self;
	self = (vkk_secondaryRenderer_t*) base;

	uint32_t swapchain_frame;
	swapchain_frame = vkk_renderer_frame(self->executor);
	return vkk_commandBuffer_get(self->cmd_buffers,
	                             swapchain_frame);
}
