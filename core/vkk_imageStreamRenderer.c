/*
 * Copyright (c) 2021 Jeff Boody
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
#include "vkk_defaultRenderer.h"
#include "vkk_engine.h"
#include "vkk_image.h"
#include "vkk_imageStreamRenderer.h"
#include "vkk_util.h"

/***********************************************************
* private                                                  *
***********************************************************/

static int
vkk_imageStreamRenderer_newRenderpass(vkk_renderer_t* base,
                                      vkk_imageFormat_e format)
{
	ASSERT(base);

	vkk_imageStreamRenderer_t* self;
	self = (vkk_imageStreamRenderer_t*) base;

	vkk_engine_t* engine = base->engine;

	VkAttachmentDescription attachments[2] =
	{
		{
			.flags          = 0,
			.format         = vkk_util_imageFormat(format),
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
vkk_imageStreamRenderer_newFramebuffer(vkk_renderer_t* base,
                                       uint32_t width,
                                       uint32_t height)
{
	ASSERT(base);

	vkk_imageStreamRenderer_t* self;
	self = (vkk_imageStreamRenderer_t*) base;

	vkk_engine_t* engine = base->engine;

	uint32_t image_count;
	image_count = vkk_renderer_imageCount(self->consumer);

	self->framebuffers = (VkFramebuffer*)
	                     CALLOC(image_count,
	                            sizeof(VkFramebuffer));
	if(self->framebuffers == NULL)
	{
		LOGE("CALLOC failed");
		return 0;
	}

	int i;
	for(i = 0; i < image_count; ++i)
	{
		VkImageView attachments[2] =
		{
			self->image_views[i],
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
			.width           = width,
			.height          = height,
			.layers          = 1,
		};

		if(vkCreateFramebuffer(engine->device, &f_info, NULL,
		                       &self->framebuffers[i]) != VK_SUCCESS)
		{
			LOGE("vkCreateFramebuffer failed");
			goto fail_framebuffer;
		}
	}

	// success
	return 1;

	// failure
	fail_framebuffer:
	{
		int j;
		for(j = 0; j < i; ++j)
		{
			vkDestroyFramebuffer(engine->device,
			                     self->framebuffers[j],
			                     NULL);
			self->framebuffers[j] = VK_NULL_HANDLE;
		}
		FREE(self->framebuffers);
		self->framebuffers = NULL;
	}
	return 0;
}

static void
vkk_imageStreamRenderer_deleteFramebuffer(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_imageStreamRenderer_t* self;
	self = (vkk_imageStreamRenderer_t*) base;

	vkk_engine_t* engine = base->engine;

	uint32_t image_count;
	image_count = vkk_renderer_imageCount(self->consumer);

	if(self->framebuffers == NULL)
	{
		return;
	}

	int i;
	for(i = 0; i < image_count; ++i)
	{
		vkDestroyFramebuffer(engine->device,
		                     self->framebuffers[i],
		                     NULL);
		self->framebuffers[i] = VK_NULL_HANDLE;
	}
	FREE(self->framebuffers);
	self->framebuffers = NULL;
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_renderer_t*
vkk_imageStreamRenderer_new(vkk_renderer_t* consumer,
                            uint32_t width, uint32_t height,
                            vkk_imageFormat_e format,
                            int mipmap,
                            vkk_stage_e stage)
{
	ASSERT(consumer);

	vkk_engine_t* engine = consumer->engine;

	uint32_t image_count;
	image_count = vkk_renderer_imageCount(consumer);

	vkk_imageStreamRenderer_t* self;
	self = (vkk_imageStreamRenderer_t*)
	       CALLOC(1, sizeof(vkk_imageStreamRenderer_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->consumer = consumer;

	vkk_renderer_t* base = &(self->base);
	vkk_renderer_init(base, VKK_RENDERER_TYPE_IMAGESTREAM,
	                  engine);

	self->images = (vkk_image_t**)
	               CALLOC(image_count, sizeof(vkk_image_t*));
	if(self->images == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_images;
	}

	int i;
	for(i = 0; i < image_count; ++i)
	{
		self->images[i] = vkk_image_new(engine, width, height, 1,
		                                format, mipmap, stage,
		                                NULL);
		if(self->images[i] == NULL)
		{
			goto fail_image;
		}

		// create the semaphore that the image stream renderer
		// will signal when rendering to the image is complete
		if(vkk_image_createSemaphore(self->images[i]) == 0)
		{
			vkk_image_delete(&self->images[i]);
			goto fail_image;
		}
	}

	self->image_views = (VkImageView*)
	                    CALLOC(image_count,
	                           sizeof(VkImageView));
	if(self->image_views == NULL)
	{
		goto fail_image_views;
	}

	VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	int j;
	for(j = 0; j < image_count; ++j)
	{
		VkImageViewCreateInfo iv_info =
		{
			.sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext      = NULL,
			.flags      = 0,
			.image      = self->images[j]->image,
			.viewType   = VK_IMAGE_VIEW_TYPE_2D,
			.format     = vkk_util_imageFormat(self->images[j]->format),
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
		                     &self->image_views[j]) != VK_SUCCESS)
		{
			LOGE("vkCreateImageView failed");
			goto fail_image_view;
		}
	}

	if(vkk_imageStreamRenderer_newRenderpass(base, format) == 0)
	{
		goto fail_renderpass;
	}

	self->depth_image = vkk_image_new(engine, width, height,
	                                  1, VKK_IMAGE_FORMAT_DEPTH1X,
	                                  0, VKK_STAGE_DEPTH, NULL);
	if(self->depth_image == NULL)
	{
		goto fail_depth;
	}

	if(vkk_imageStreamRenderer_newFramebuffer(base, width,
	                                          height) == 0)
	{
		goto fail_framebuffer;
	}

	self->cmd_buffers = vkk_commandBuffer_new(engine,
	                                          image_count, 0);
	if(self->cmd_buffers == NULL)
	{
		goto fail_cmd_buffers;
	}

	// success
	return base;

	// failure
	int k;
	fail_cmd_buffers:
		vkk_imageStreamRenderer_deleteFramebuffer(base);
	fail_framebuffer:
		vkk_image_delete(&self->depth_image);
	fail_depth:
		vkDestroyRenderPass(engine->device,
		                    self->render_pass, NULL);
	fail_renderpass:
	fail_image_view:
	{
		for(k = 0; k < j; ++k)
		{
			vkDestroyImageView(engine->device,
			                   self->image_views[k], NULL);
		}
		FREE(self->image_views);
	}
	fail_image_views:
	fail_image:
	{
		for(k = 0; k < i; ++k)
		{
			vkk_image_delete(&self->images[k]);
		}
		FREE(self->images);
	}
	fail_images:
		FREE(self);
	return NULL;
}

void vkk_imageStreamRenderer_delete(vkk_renderer_t** _base)
{
	ASSERT(_base);

	vkk_renderer_t* base = *_base;
	if(base)
	{
		vkk_imageStreamRenderer_t* self;
		self = (vkk_imageStreamRenderer_t*) base;

		vkk_engine_t* engine = base->engine;

		FREE(base->wait_flags);
		FREE(base->wait_array);

		uint32_t image_count;
		image_count = vkk_renderer_imageCount(self->consumer);

		vkk_commandBuffer_delete(&self->cmd_buffers);
		vkk_imageStreamRenderer_deleteFramebuffer(base);
		vkk_image_delete(&self->depth_image);
		vkDestroyRenderPass(engine->device,
		                    self->render_pass, NULL);

		int i;
		for(i = 0; i < image_count; ++i)
		{
			vkDestroyImageView(engine->device,
			                   self->image_views[i], NULL);
			vkk_image_delete(&self->images[i]);
		}
		FREE(self->image_views);
		FREE(self->images);

		FREE(self);
		*_base = NULL;
	}
}

vkk_image_t*
vkk_imageStreamRenderer_begin(vkk_renderer_t* base,
                              vkk_rendererMode_e mode,
                              float* clear_color)
{
	ASSERT(base);
	ASSERT(clear_color);

	vkk_imageStreamRenderer_t* self;
	self = (vkk_imageStreamRenderer_t*) base;

	// note that synchronization via fences is not required
	// for the image stream renderer because its begin
	// function must be called after the renderer which will
	// consume the image produced and the consumer will
	// perform any synchronization necessary

	uint32_t frame;
	frame = vkk_renderer_frame(self->consumer);

	VkCommandBuffer cb;
	cb = vkk_commandBuffer_get(self->cmd_buffers, frame);
	if(vkResetCommandBuffer(cb, 0) != VK_SUCCESS)
	{
		LOGE("vkResetCommandBuffer failed");
		return NULL;
	}

	VkFramebuffer framebuffer;
	framebuffer = self->framebuffers[frame];
	VkCommandBufferInheritanceInfo cbi_info =
	{
		.sType                = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		.pNext                = NULL,
		.renderPass           = self->render_pass,
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
		.flags            = 0,
		.pInheritanceInfo = &cbi_info
	};

	if(vkBeginCommandBuffer(cb, &cb_info) != VK_SUCCESS)
	{
		LOGE("vkBeginCommandBuffer failed");
		return NULL;
	}

	vkk_image_t* image = self->images[frame];
	vkk_util_imageMemoryBarrier(image, cb,
	                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	                            0, 1);
	vkk_util_imageMemoryBarrier(self->depth_image, cb,
	                            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	                            0, 1);

	// initialize viewport and scissor
	if(mode == VKK_RENDERER_MODE_DRAW)
	{
		VkViewport viewport =
		{
			.x        = 0.0f,
			.y        = 0.0f,
			.width    = (float) image->width,
			.height   = (float) image->height,
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
				.width  = image->width,
				.height = image->height,
			}
		};
		vkCmdSetScissor(cb, 0, 1, &scissor);
	}

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
		.framebuffer     = framebuffer,
		.renderArea      = { { .x=0, .y=0 },
		                     { .width=image->width,
		                       .height=image->height } },
		.clearValueCount = 2,
		.pClearValues    = cv
	};

	VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE;
	if(mode == VKK_RENDERER_MODE_EXECUTE)
	{
		contents = VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;
	}

	vkCmdBeginRenderPass(cb, &rp_info, contents);

	vkk_renderer_addWaitSemaphore(self->consumer,
	                              image->semaphore);

	// update timestamp
	self->ts = vkk_renderer_tsCurrent(self->consumer);

	return image;
}

void vkk_imageStreamRenderer_end(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_imageStreamRenderer_t* self;
	self = (vkk_imageStreamRenderer_t*) base;

	vkk_renderer_t* consumer = self->consumer;

	uint32_t frame;
	frame = vkk_renderer_frame(consumer);

	vkk_engine_t* engine = base->engine;
	vkk_image_t*  image  = self->images[frame];

	VkCommandBuffer cb;
	cb = vkk_commandBuffer_get(self->cmd_buffers, frame);
	vkCmdEndRenderPass(cb);

	// at this point we may need to generate mip_levels if
	// mipmapping was enabled
	if(image->mip_levels > 1)
	{
		vkk_engine_mipmapImage(engine, image, cb);
	}

	// transition the image to shading mode
	// note: we do not use the render pass to transition to the
	// finalLayout since the image might be mipmapped which
	// requires further processing after the render pass
	// completes and would cause the image->layout_array to
	// become inconsistent
	vkk_util_imageMemoryBarrier(image, cb,
	                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	                            0, image->mip_levels);

	vkEndCommandBuffer(cb);

	uint32_t queue = VKK_QUEUE_FOREGROUND;
	if(consumer->type != VKK_RENDERER_TYPE_DEFAULT)
	{
		queue = VKK_QUEUE_BACKGROUND;
	}

	if(vkk_engine_queueSubmit(engine, queue, &cb,
	                          base->wait_count,
	                          base->wait_array,
	                          &image->semaphore,
	                          base->wait_flags,
	                          VK_NULL_HANDLE) == 0)
	{
		LOGW("vkk_engine_queueSubmit failed");
	}
}

void vkk_imageStreamRenderer_surfaceSize(vkk_renderer_t* base,
                                         uint32_t* _width,
                                         uint32_t* _height)
{
	ASSERT(base);
	ASSERT(_width);
	ASSERT(_height);

	vkk_imageStreamRenderer_t* self;
	self = (vkk_imageStreamRenderer_t*) base;

	uint32_t frame;
	frame = vkk_renderer_frame(self->consumer);

	vkk_image_t* image = self->images[frame];

	*_width  = image->width;
	*_height = image->height;
}

VkRenderPass
vkk_imageStreamRenderer_renderPass(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_imageStreamRenderer_t* self;
	self = (vkk_imageStreamRenderer_t*) base;

	return self->render_pass;
}

VkFramebuffer
vkk_imageStreamRenderer_framebuffer(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_imageStreamRenderer_t* self;
	self = (vkk_imageStreamRenderer_t*) base;

	uint32_t frame;
	frame = vkk_renderer_frame(self->consumer);

	return self->framebuffers[frame];
}

VkCommandBuffer
vkk_imageStreamRenderer_commandBuffer(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_imageStreamRenderer_t* self;
	self = (vkk_imageStreamRenderer_t*) base;

	uint32_t frame;
	frame = vkk_renderer_frame(self->consumer);

	return vkk_commandBuffer_get(self->cmd_buffers, frame);
}
