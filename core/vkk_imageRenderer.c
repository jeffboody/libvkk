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
#include "vkk_image.h"
#include "vkk_imageRenderer.h"
#include "vkk_memoryManager.h"
#include "vkk_util.h"

/***********************************************************
* private                                                  *
***********************************************************/

static int
vkk_imageRenderer_newRenderpass(vkk_renderer_t* base,
                                    vkk_imageFormat_e format)
{
	ASSERT(base);

	vkk_imageRenderer_t* self;
	self = (vkk_imageRenderer_t*) base;

	vkk_engine_t* engine = base->engine;

	// when MSAA is enabled
	// 1. rendering is performed on a MS attachment
	// 2. the MS attachment is resolved to the color
	//    attachment at the end of the renderpass
	// 3. the depth attachment has the same number of samples
	//    as the MS attachment
	//
	// otherwise
	// 1. rendering is performed on the color attachment
	// 2. the resolve step is not required
	VkSampleCountFlagBits samples;
	VkAttachmentLoadOp    load_op;
	samples = VK_SAMPLE_COUNT_4_BIT;
	load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	if(vkk_renderer_msaaSampleCount(base) == 1)
	{
		samples = VK_SAMPLE_COUNT_1_BIT;
		load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
	}

	VkAttachmentDescription attachments[] =
	{
		{
			.flags          = 0,
			.format         = vkk_util_imageFormat(format),
			.samples        = VK_SAMPLE_COUNT_1_BIT,
			.loadOp         = load_op,
			.storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		},
		{
			.flags          = 0,
			.format         = VK_FORMAT_D24_UNORM_S8_UINT,
			.samples        = samples,
			.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		},
		{
			.flags          = 0,
			.format         = vkk_util_imageFormat(format),
			.samples        = samples,
			.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		},
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

	VkAttachmentReference msaa_attachment =
	{
		.attachment = 2,
		.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	uint32_t attachment_count = 3;
	VkAttachmentReference* _color_attachment   = &msaa_attachment;
	VkAttachmentReference* _resolve_attachment = &color_attachment;
	if(vkk_renderer_msaaSampleCount(base) == 1)
	{
		attachment_count    = 2;
		_color_attachment   = &color_attachment;
		_resolve_attachment = NULL;
	}

	VkSubpassDescription subpass =
	{
		.flags = 0,
		.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount    = 0,
		.pInputAttachments       = NULL,
		.colorAttachmentCount    = 1,
		.pColorAttachments       = _color_attachment,
		.pResolveAttachments     = _resolve_attachment,
		.pDepthStencilAttachment = &depth_attachment,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments    = NULL,
	};

	VkRenderPassCreateInfo rp_info =
	{
		.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext           = NULL,
		.flags           = 0,
		.attachmentCount = attachment_count,
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
vkk_imageRenderer_newDepth(vkk_renderer_t* base,
                           uint32_t width,
                           uint32_t height)
{
	ASSERT(base);

	vkk_imageRenderer_t* self;
	self = (vkk_imageRenderer_t*) base;

	vkk_engine_t* engine = base->engine;

	vkk_imageFormat_e format_depth;
	format_depth = VKK_IMAGE_FORMAT_DEPTH4X;
	if(vkk_renderer_msaaSampleCount(base) == 1)
	{
		format_depth = VKK_IMAGE_FORMAT_DEPTH1X;
	}

	self->depth_image = vkk_image_new(engine, width, height,
	                                  1, format_depth,
	                                  0, VKK_STAGE_DEPTH, NULL);
	if(self->depth_image == NULL)
	{
		return 0;
	}

	return 1;
}

static void
vkk_imageRenderer_deleteDepth(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_imageRenderer_t* self;
	self = (vkk_imageRenderer_t*) base;

	vkk_image_delete(&self->depth_image);
}

static int
vkk_imageRenderer_newMSAA(vkk_renderer_t* base,
                          uint32_t width,
                          uint32_t height,
                          vkk_imageFormat_e format)
{
	ASSERT(base);

	vkk_imageRenderer_t* self;
	self = (vkk_imageRenderer_t*) base;

	vkk_engine_t* engine = base->engine;

	// check if MSAA is supported
	if(vkk_renderer_msaaSampleCount(base) == 1)
	{
		return 1;
	}

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
		.format      = vkk_util_imageFormat(format),
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
	                 &self->msaa_image) != VK_SUCCESS)
	{
		LOGE("vkCreateImage failed");
		return 0;
	}

	// msaa memory is uninitialized
	self->msaa_memory =
		vkk_memoryManager_allocImage(engine->mm,
		                             self->msaa_image, 1, 1);
	if(self->msaa_memory == NULL)
	{
		goto fail_alloc;
	}

	VkImageViewCreateInfo iv_info =
	{
		.sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext      = NULL,
		.flags      = 0,
		.image      = self->msaa_image,
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
			.levelCount     = 1,
			.baseArrayLayer = 0,
			.layerCount     = 1
		}
	};

	if(vkCreateImageView(engine->device, &iv_info, NULL,
	                     &self->msaa_image_view) != VK_SUCCESS)
	{
		LOGE("vkCreateImageView failed");
		goto fail_image_view;
	}

	// success
	return 1;

	// failure
	fail_image_view:
		vkk_memoryManager_free(engine->mm, &self->msaa_memory);
	fail_alloc:
	{
		vkDestroyImage(engine->device, self->msaa_image, NULL);
		self->msaa_image = VK_NULL_HANDLE;
	}
	return 0;
}

static void
vkk_imageRenderer_deleteMSAA(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_imageRenderer_t* self;
	self = (vkk_imageRenderer_t*) base;

	vkk_engine_t* engine = base->engine;

	vkDestroyImageView(engine->device, self->msaa_image_view,
	                   NULL);
	vkk_memoryManager_free(engine->mm, &self->msaa_memory);
	vkDestroyImage(engine->device, self->msaa_image, NULL);
	self->msaa_image_view = VK_NULL_HANDLE;
	self->msaa_image      = VK_NULL_HANDLE;
}

static int
vkk_imageRenderer_newFramebuffer(vkk_renderer_t* base,
                                 uint32_t width,
                                 uint32_t height,
                                 vkk_imageFormat_e format)
{
	ASSERT(base);

	vkk_imageRenderer_t* self;
	self = (vkk_imageRenderer_t*) base;

	vkk_engine_t* engine = base->engine;

	self->src_image = vkk_image_new(engine, width, height, 1,
	                                format, 0, VKK_STAGE_FS,
	                                NULL);
	if(self->src_image == NULL)
	{
		return 0;
	}

	VkImageView attachments[] =
	{
		self->src_image->image_view,
		self->depth_image->image_view,
		self->msaa_image_view,
	};

	int attachment_count = 3;
	if(vkk_renderer_msaaSampleCount(base) == 1)
	{
		attachment_count = 2;
	}

	VkFramebufferCreateInfo f_info =
	{
		.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext           = NULL,
		.flags           = 0,
		.renderPass      = self->render_pass,
		.attachmentCount = attachment_count,
		.pAttachments    = attachments,
		.width           = width,
		.height          = height,
		.layers          = 1,
	};

	if(vkCreateFramebuffer(engine->device, &f_info, NULL,
	                       &self->framebuffer) != VK_SUCCESS)
	{
		LOGE("vkCreateFramebuffer failed");
		goto fail_framebuffer;
	}

	// success
	return 1;

	// failure
	fail_framebuffer:
		vkk_image_delete(&self->src_image);
	return 0;
}

static void
vkk_imageRenderer_deleteFramebuffer(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_imageRenderer_t* self;
	self = (vkk_imageRenderer_t*) base;

	vkk_engine_t* engine = base->engine;

	vkDestroyFramebuffer(engine->device, self->framebuffer,
	                     NULL);
	self->framebuffer = VK_NULL_HANDLE;
	vkk_image_delete(&self->src_image);
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_renderer_t*
vkk_imageRenderer_new(vkk_engine_t* engine,
                      uint32_t width, uint32_t height,
                      vkk_imageFormat_e format,
                      vkk_rendererMsaa_e msaa)
{
	ASSERT(engine);

	vkk_imageRenderer_t* self;
	self = (vkk_imageRenderer_t*)
	       CALLOC(1, sizeof(vkk_imageRenderer_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	vkk_renderer_t* base = &(self->base);
	vkk_renderer_init(base, VKK_RENDERER_TYPE_IMAGE,
	                  msaa, engine);

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

	if(vkk_imageRenderer_newRenderpass(base, format) == 0)
	{
		goto fail_renderpass;
	}

	if(vkk_imageRenderer_newDepth(base, width,
	                              height) == 0)
	{
		goto fail_depth;
	}

	if(vkk_imageRenderer_newMSAA(base, width, height,
	                             format) == 0)
	{
		goto fail_msaa;
	}

	if(vkk_imageRenderer_newFramebuffer(base,
	                                    width, height,
	                                    format) == 0)
	{
		goto fail_framebuffer;
	}

	self->cmd_buffer = vkk_commandBuffer_new(engine, 1, 0);
	if(self->cmd_buffer == NULL)
	{
		goto fail_cmd_buffer;
	}

	// success
	return base;

	// failure
	fail_cmd_buffer:
		vkk_imageRenderer_deleteFramebuffer(base);
	fail_framebuffer:
		vkk_imageRenderer_deleteMSAA(base);
	fail_msaa:
		vkk_imageRenderer_deleteDepth(base);
	fail_depth:
		vkDestroyRenderPass(engine->device,
		                    self->render_pass, NULL);
	fail_renderpass:
		vkDestroyFence(engine->device, self->fence, NULL);
	fail_create_fence:
		FREE(self);
	return NULL;
}

void vkk_imageRenderer_delete(vkk_renderer_t** _base)
{
	ASSERT(_base);

	vkk_renderer_t* base = *_base;
	if(base)
	{
		vkk_imageRenderer_t* self;
		self = (vkk_imageRenderer_t*) base;

		vkk_engine_t* engine = base->engine;

		FREE(base->wait_flags);
		FREE(base->wait_array);

		vkk_commandBuffer_delete(&self->cmd_buffer);
		vkk_imageRenderer_deleteFramebuffer(base);
		vkk_imageRenderer_deleteMSAA(base);
		vkk_imageRenderer_deleteDepth(base);
		vkDestroyRenderPass(engine->device,
		                    self->render_pass, NULL);
		vkDestroyFence(engine->device, self->fence, NULL);
		FREE(self);
		*_base = NULL;
	}
}

int
vkk_imageRenderer_begin(vkk_renderer_t* base,
                        vkk_rendererMode_e mode,
                        vkk_image_t* image,
                        float* clear_color)
{
	ASSERT(base);
	ASSERT(image);
	ASSERT(clear_color);

	vkk_imageRenderer_t* self;
	self = (vkk_imageRenderer_t*) base;

	vkk_image_t* src_image = self->src_image;

	ASSERT((image->width  == src_image->width)  &&
	       (image->height == src_image->height) &&
	       (image->depth  == src_image->depth)  &&
	       (image->format == src_image->format));

	VkCommandBuffer cb;
	cb = vkk_commandBuffer_get(self->cmd_buffer, 0);
	if(vkResetCommandBuffer(cb, 0) != VK_SUCCESS)
	{
		LOGE("vkResetCommandBuffer failed");
		return 0;
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

	if(vkBeginCommandBuffer(cb, &cb_info) != VK_SUCCESS)
	{
		LOGE("vkBeginCommandBuffer failed");
		return 0;
	}

	vkk_util_imageMemoryBarrier(src_image, cb,
	                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	                            0, 1);
	vkk_util_imageMemoryBarrier(self->depth_image, cb,
	                            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	                            0, 1);

	// the secondary renderers also initialize
	// viewport and scissor in beginSecondary
	if(mode == VKK_RENDERER_MODE_DRAW)
	{
		VkViewport viewport =
		{
			.x        = 0.0f,
			.y        = 0.0f,
			.width    = (float) src_image->width,
			.height   = (float) src_image->height,
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
				.width  = src_image->width,
				.height = src_image->height,
			}
		};
		vkCmdSetScissor(cb, 0, 1, &scissor);
	}

	VkClearValue cv[] =
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
		},
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
	};

	uint32_t cv_count = 3;
	if(vkk_renderer_msaaSampleCount(base) == 1)
	{
		cv_count = 2;
	}

	VkRenderPassBeginInfo rp_info =
	{
		.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext           = NULL,
		.renderPass      = self->render_pass,
		.framebuffer     = self->framebuffer,
		.renderArea      = { { .x=0, .y=0 },
		                     { .width=src_image->width,
		                       .height=src_image->height } },
		.clearValueCount = cv_count,
		.pClearValues    = cv
	};

	VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE;
	if(mode == VKK_RENDERER_MODE_EXECUTE)
	{
		contents = VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;
	}

	vkCmdBeginRenderPass(cb, &rp_info, contents);

	self->dst_image = image;

	return 1;
}

void vkk_imageRenderer_end(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_imageRenderer_t* self;
	self = (vkk_imageRenderer_t*) base;

	vkk_engine_t* engine    = base->engine;
	vkk_image_t*  src_image = self->src_image;
	vkk_image_t*  dst_image = self->dst_image;

	VkCommandBuffer cb;
	cb = vkk_commandBuffer_get(self->cmd_buffer, 0);
	vkCmdEndRenderPass(cb);

	// check if dst_image is in use
	vkk_engine_rendererWaitForTimestamp(engine, dst_image->ts);

	vkk_util_imageMemoryBarrier(src_image, cb,
	                            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	                            0, 1);
	vkk_util_imageMemoryBarrier(dst_image, cb,
	                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	                            0, 1);

	VkImageBlit ib =
	{
		.srcSubresource =
		{
			.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel       = 0,
			.baseArrayLayer = 0,
			.layerCount     = 1
		},
		.srcOffsets =
		{
			{
				.x = 0,
				.y = 0,
				.z = 0,
			},
			{
				.x = src_image->width,
				.y = src_image->height,
				.z = 1,
			}
		},
		.dstSubresource =
		{
			.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel       = 0,
			.baseArrayLayer = 0,
			.layerCount     = 1
		},
		.dstOffsets =
		{
			{
				.x = 0,
				.y = 0,
				.z = 0,
			},
			{
				.x = dst_image->width,
				.y = dst_image->height,
				.z = 1,
			}
		}
	};

	vkCmdBlitImage(cb, src_image->image,
	               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	               dst_image->image,
	               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	               1, &ib, VK_FILTER_NEAREST);

	// at this point we may need to generate mip_levels if
	// mipmapping was enabled
	if(dst_image->mip_levels > 1)
	{
		vkk_engine_mipmapImage(engine, dst_image, cb);
	}

	// transition the dst_image to shading mode
	// note: we do not use the render pass to transition to the
	// finalLayout since the image might be mipmapped which
	// requires further processing after the render pass
	// completes and would cause the image->layout_array to
	// become inconsistent
	vkk_util_imageMemoryBarrier(dst_image, cb,
	                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	                            0, dst_image->mip_levels);

	vkEndCommandBuffer(cb);

	vkResetFences(engine->device, 1, &self->fence);
	if(vkk_engine_queueSubmit(engine, VKK_QUEUE_BACKGROUND, &cb,
	                          base->wait_count,
	                          base->wait_array,
	                          NULL, base->wait_flags,
	                          self->fence) == 0)
	{
		LOGW("vkk_engine_queueSubmit failed");
		self->dst_image = NULL;
		return;
	}

	uint64_t timeout = UINT64_MAX;
	if(vkWaitForFences(engine->device, 1, &self->fence, VK_TRUE,
	                   timeout) != VK_SUCCESS)
	{
		LOGW("vkWaitForFences failed");
		vkk_engine_queueWaitIdle(engine, VKK_QUEUE_BACKGROUND);
	}

	self->dst_image = NULL;
}

void
vkk_imageRenderer_surfaceSize(vkk_renderer_t* base,
                              uint32_t* _width,
                              uint32_t* _height)
{
	ASSERT(base);
	ASSERT(_width);
	ASSERT(_height);

	vkk_imageRenderer_t* self;
	self = (vkk_imageRenderer_t*) base;

	*_width  = self->src_image->width;
	*_height = self->src_image->height;
}

VkRenderPass
vkk_imageRenderer_renderPass(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_imageRenderer_t* self;
	self = (vkk_imageRenderer_t*) base;

	return self->render_pass;
}

VkFramebuffer
vkk_imageRenderer_framebuffer(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_imageRenderer_t* self;
	self = (vkk_imageRenderer_t*) base;

	return self->framebuffer;
}

VkCommandBuffer
vkk_imageRenderer_commandBuffer(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_imageRenderer_t* self;
	self = (vkk_imageRenderer_t*) base;

	return vkk_commandBuffer_get(self->cmd_buffer, 0);
}
