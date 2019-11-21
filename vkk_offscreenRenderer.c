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
#include <string.h>

#define LOG_TAG "vkk"
#include "../libcc/cc_log.h"
#include "../libcc/cc_memory.h"
#include "vkk_offscreenRenderer.h"
#include "vkk_engine.h"
#include "vkk_image.h"
#include "vkk_util.h"

/***********************************************************
* private                                                  *
***********************************************************/

static int
vkk_offscreenRenderer_newRenderpass(vkk_renderer_t* base)
{
	assert(base);

	vkk_offscreenRenderer_t* self;
	self = (vkk_offscreenRenderer_t*) base;

	vkk_engine_t* engine = base->engine;

	VkAttachmentDescription attachments[2] =
	{
		{
			.flags          = 0,
			.format         = vkk_util_imageFormat(self->format),
			.samples        = VK_SAMPLE_COUNT_1_BIT,
			.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		},
		{
			.flags          = 0,
			.format         = VK_FORMAT_D24_UNORM_S8_UINT,
			.samples        = VK_SAMPLE_COUNT_1_BIT,
			.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		}
	};

	VkAttachmentReference color_attachment =
	{
		.attachment = 0,
		.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkAttachmentReference depth_attachment =
	{
		.attachment = 1,
		.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass =
	{
		.flags = 0,
		.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount    = 0,
		.pInputAttachments       = NULL,
		.colorAttachmentCount    = 1,
		.pColorAttachments       = &color_attachment,
		.pResolveAttachments     = NULL,
		.pDepthStencilAttachment = &depth_attachment,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments    = NULL,
	};

	VkRenderPassCreateInfo rp_info =
	{
		.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext           = NULL,
		.flags           = 0,
		.attachmentCount = 2,
		.pAttachments    = attachments,
		.subpassCount    = 1,
		.pSubpasses      = &subpass,
		.dependencyCount = 0,
		.pDependencies   = NULL
	};

	if(vkCreateRenderPass(engine->device, &rp_info, NULL,
	                      &self->render_pass) != VK_SUCCESS)
	{
		LOGE("vkCreateRenderPass failed");
		return 0;
	}

	return 1;
}

static int
vkk_offscreenRenderer_newDepth(vkk_renderer_t* base)
{
	assert(base);

	vkk_offscreenRenderer_t* self;
	self = (vkk_offscreenRenderer_t*) base;

	vkk_engine_t* engine = base->engine;

	self->depth_image = vkk_image_new(engine,
	                                  self->width,
	                                  self->height,
	                                  VKK_IMAGE_FORMAT_DEPTH,
	                                  0, VKK_STAGE_DEPTH, NULL);
	if(self->depth_image == NULL)
	{
		return 0;
	}

	return 1;
}

static void
vkk_offscreenRenderer_deleteDepth(vkk_renderer_t* base)
{
	assert(base);

	vkk_offscreenRenderer_t* self;
	self = (vkk_offscreenRenderer_t*) base;

	vkk_image_delete(&self->depth_image);
}

static int
vkk_offscreenRenderer_newFramebuffer(vkk_renderer_t* base,
                                     vkk_image_t* image)
{
	assert(base);
	assert(image);

	vkk_offscreenRenderer_t* self;
	self = (vkk_offscreenRenderer_t*) base;

	vkk_engine_t* engine = base->engine;

	VkImageViewCreateInfo iv_info =
	{
		.sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext      = NULL,
		.flags      = 0,
		.image      = image->image,
		.viewType   = VK_IMAGE_VIEW_TYPE_2D,
		.format     = vkk_util_imageFormat(self->format),
		.components =
		{
			.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.a = VK_COMPONENT_SWIZZLE_IDENTITY
		},
		.subresourceRange =
		{
			.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel   = 0,
			.levelCount     = 1,
			.baseArrayLayer = 0,
			.layerCount     = 1
		}
	};

	if(vkCreateImageView(engine->device, &iv_info, NULL,
	                     &self->framebuffer_image_view) != VK_SUCCESS)
	{
		LOGE("vkCreateImageView failed");
		return 0;
	}

	VkImageView attachments[2] =
	{
		self->framebuffer_image_view,
		self->depth_image->image_view,
	};

	VkFramebufferCreateInfo f_info =
	{
		.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext           = NULL,
		.flags           = 0,
		.renderPass      = self->render_pass,
		.attachmentCount = 2,
		.pAttachments    = attachments,
		.width           = self->width,
		.height          = self->height,
		.layers          = 1,
	};

	if(vkCreateFramebuffer(engine->device, &f_info, NULL,
	                       &self->framebuffer) != VK_SUCCESS)
	{
		LOGE("vkCreateFramebuffer failed");
		goto fail_framebuffer;
	}

	self->image = image;

	// success
	return 1;

	// failure
	fail_framebuffer:
		vkDestroyImageView(engine->device,
		                   self->framebuffer_image_view,
		                   NULL);
	return 0;
}

static void
vkk_offscreenRenderer_deleteFramebuffer(vkk_renderer_t* base)
{
	assert(base);

	vkk_offscreenRenderer_t* self;
	self = (vkk_offscreenRenderer_t*) base;

	vkk_engine_t* engine = base->engine;

	vkDestroyFramebuffer(engine->device,
	                     self->framebuffer,
	                     NULL);
	vkDestroyImageView(engine->device,
	                   self->framebuffer_image_view,
	                   NULL);
	self->framebuffer            = VK_NULL_HANDLE;
	self->framebuffer_image_view = VK_NULL_HANDLE;
	self->image                  = NULL;
}

static int
vkk_offscreenRenderer_newCommandBuffer(vkk_renderer_t* base)
{
	assert(base);

	vkk_offscreenRenderer_t* self;
	self = (vkk_offscreenRenderer_t*) base;

	vkk_engine_t* engine = base->engine;

	if(vkk_engine_allocateCommandBuffers(engine, 1,
	                                     &self->cb) == 0)
	{
		return 0;
	}

	return 1;
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_renderer_t*
vkk_offscreenRenderer_new(vkk_engine_t* engine,
                          uint32_t width,
                          uint32_t height,
                          int format)
{
	assert(engine);

	vkk_offscreenRenderer_t* self;
	self = (vkk_offscreenRenderer_t*)
	       CALLOC(1, sizeof(vkk_offscreenRenderer_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->width  = width;
	self->height = height;
	self->format = format;
	self->image  = NULL;

	vkk_renderer_t* base = &(self->base);
	vkk_renderer_init(base, engine);

	VkFenceCreateInfo f_info =
	{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = NULL,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	if(vkCreateFence(engine->device, &f_info, NULL,
	                 &self->fence) != VK_SUCCESS)
	{
		goto fail_create_fence;
	}

	if(vkk_offscreenRenderer_newRenderpass(base) == 0)
	{
		goto fail_renderpass;
	}

	if(vkk_offscreenRenderer_newDepth(base) == 0)
	{
		goto fail_depth;
	}

	// framebuffer is created in begin
	self->framebuffer_image_view = VK_NULL_HANDLE;
	self->framebuffer            = VK_NULL_HANDLE;

	if(vkk_offscreenRenderer_newCommandBuffer(base) == 0)
	{
		goto fail_cb;
	}

	// success
	return base;

	// failure
	fail_cb:
		vkk_offscreenRenderer_deleteDepth(base);
	fail_depth:
		vkDestroyRenderPass(engine->device,
		                    self->render_pass, NULL);
	fail_renderpass:
		vkDestroyFence(engine->device, self->fence, NULL);
	fail_create_fence:
		FREE(self);
	return NULL;
}

void vkk_offscreenRenderer_delete(vkk_renderer_t** _base)
{
	assert(_base);

	vkk_renderer_t* base = *_base;
	if(base)
	{
		vkk_offscreenRenderer_t* self;
		self = (vkk_offscreenRenderer_t*) base;

		vkk_engine_t* engine = base->engine;

		// framebuffer is deleted by end function
		vkk_engine_freeCommandBuffers(engine, 1, &self->cb);
		vkk_offscreenRenderer_deleteDepth(base);
		vkDestroyRenderPass(engine->device,
		                    self->render_pass, NULL);
		vkDestroyFence(engine->device, self->fence, NULL);
		FREE(self);
		*_base = NULL;
	}
}

int
vkk_offscreenRenderer_begin(vkk_renderer_t* base,
                            vkk_image_t* image,
                            float* clear_color)
{
	assert(base);
	assert(image);
	assert(clear_color);

	vkk_offscreenRenderer_t* self;
	self = (vkk_offscreenRenderer_t*) base;

	vkk_engine_t* engine = base->engine;

	// check if renderer and image are compatible
	if((self->width  != image->width)  ||
	   (self->height != image->height) ||
	   (self->format != image->format))
	{
		LOGE("invalid width=%u:%u, height=%u:%u, format=%i:%i",
		     self->width, image->width,
		     self->height, image->height,
		     self->format, image->format);
		return 0;
	}

	// check if image is in use
	vkk_engine_rendererWaitForTimestamp(engine, image->ts);

	if(vkk_offscreenRenderer_newFramebuffer(base, image) == 0)
	{
		self->image = NULL;
		return 0;
	}

	vkk_engine_cmdLock(engine);
	if(vkResetCommandBuffer(self->cb, 0) != VK_SUCCESS)
	{
		LOGE("vkResetCommandBuffer failed");
		vkk_engine_cmdUnlock(engine);
		goto fail_reset;
	}

	VkCommandBufferInheritanceInfo cbi_info =
	{
		.sType                = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		.pNext                = NULL,
		.renderPass           = self->render_pass,
		.subpass              = 0,
		.framebuffer          = self->framebuffer,
		.occlusionQueryEnable = VK_FALSE,
		.queryFlags           = 0,
		.pipelineStatistics   = 0
	};

	VkCommandBufferBeginInfo cb_info =
	{
		.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext            = NULL,
		.flags            = 0,
		.pInheritanceInfo = &cbi_info
	};

	if(vkBeginCommandBuffer(self->cb, &cb_info) != VK_SUCCESS)
	{
		LOGE("vkBeginCommandBuffer failed");
		vkk_engine_cmdUnlock(engine);
		goto fail_begin_command_buffer;
	}

	vkk_util_imageMemoryBarrier(image, self->cb,
	                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	                            0, 1);
	vkk_util_imageMemoryBarrier(self->depth_image, self->cb,
	                            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	                            0, 1);


	VkViewport viewport =
	{
		.x        = 0.0f,
		.y        = 0.0f,
		.width    = (float) self->width,
		.height   = (float) self->height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};
	vkCmdSetViewport(self->cb, 0, 1, &viewport);

	VkRect2D scissor =
	{
		.offset =
		{
			.x = 0,
			.y = 0
		},
		.extent =
		{
			.width  = self->width,
			.height = self->height,
		}
	};
	vkCmdSetScissor(self->cb, 0, 1, &scissor);

	VkClearValue cv[2] =
	{
		{
			.color =
			{
				.float32 =
				{
					clear_color[0],
					clear_color[1],
					clear_color[2],
					clear_color[3]
				}
			},
		},
		{
			.depthStencil =
			{
				.depth   = 1.0f,
				.stencil = 0
			}
		}
	};

	VkRenderPassBeginInfo rp_info =
	{
		.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext           = NULL,
		.renderPass      = self->render_pass,
		.framebuffer     = self->framebuffer,
		.renderArea      = { { .x=0, .y=0 },
		                     { .width=self->width,
		                       .height=self->height } },
		.clearValueCount = 2,
		.pClearValues    = cv
	};

	vkCmdBeginRenderPass(self->cb,
	                     &rp_info,
	                     VK_SUBPASS_CONTENTS_INLINE);

	// success
	return 1;

	// failure
	fail_begin_command_buffer:
	fail_reset:
		vkk_offscreenRenderer_deleteFramebuffer(base);
	return 0;
}

void vkk_offscreenRenderer_end(vkk_renderer_t* base)
{
	assert(base);

	vkk_offscreenRenderer_t* self;
	self = (vkk_offscreenRenderer_t*) base;

	vkk_engine_t* engine = base->engine;
	vkk_image_t*  image  = self->image;

	vkCmdEndRenderPass(self->cb);

	// at this point we may need to generate mip_levels if
	// mipmapping was enabled
	if(image->mip_levels > 1)
	{
		vkk_engine_mipmapImage(engine, image, self->cb);
	}

	// transition the image to shading mode
	// note: we do not use the render pass to transition to the
	// finalLayout since the image might be mipmapped which
	// requires further processing after the render pass
	// completes and would cause the image->layout_array to
	// become inconsistent
	vkk_util_imageMemoryBarrier(image, self->cb,
	                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	                            0, image->mip_levels);

	vkEndCommandBuffer(self->cb);
	vkk_engine_cmdUnlock(engine);

	VkPipelineStageFlags wait_dst_stage_mask;
	wait_dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	vkResetFences(engine->device, 1, &self->fence);
	if(vkk_engine_queueSubmit(engine, &self->cb,
	                          NULL, NULL,
	                          &wait_dst_stage_mask,
	                          self->fence) == 0)
	{
		vkk_offscreenRenderer_deleteFramebuffer(base);
		return;
	}

	uint64_t timeout = UINT64_MAX;
	if(vkWaitForFences(engine->device, 1, &self->fence, VK_TRUE,
	                   timeout) != VK_SUCCESS)
	{
		LOGW("vkWaitForFences failed");
		vkk_engine_queueWaitIdle(engine);
	}

	vkk_offscreenRenderer_deleteFramebuffer(base);
}

void
vkk_offscreenRenderer_surfaceSize(vkk_renderer_t* base,
                                  uint32_t* _width,
                                  uint32_t* _height)
{
	assert(base);
	assert(_width);
	assert(_height);

	vkk_offscreenRenderer_t* self;
	self = (vkk_offscreenRenderer_t*) base;

	*_width  = self->width;
	*_height = self->height;
}

VkRenderPass
vkk_offscreenRenderer_renderPass(vkk_renderer_t* base)
{
	assert(base);

	vkk_offscreenRenderer_t* self;
	self = (vkk_offscreenRenderer_t*) base;

	return self->render_pass;
}

VkCommandBuffer
vkk_offscreenRenderer_commandBuffer(vkk_renderer_t* base)
{
	assert(base);

	vkk_offscreenRenderer_t* self;
	self = (vkk_offscreenRenderer_t*) base;

	return self->cb;
}

uint32_t
vkk_offscreenRenderer_swapchainFrame(vkk_renderer_t* base)
{
	assert(base);

	return 0;
}
