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
#include "vkk_memoryManager.h"
#include "vkk_util.h"

/***********************************************************
* private                                                  *
***********************************************************/

static void
vkk_defaultRenderer_beginSemaphore(vkk_renderer_t* base,
                                   VkSemaphore* semaphore_acquire,
                                   VkSemaphore* semaphore_submit)
{
	ASSERT(base);
	ASSERT(semaphore_acquire);
	ASSERT(semaphore_submit);

	vkk_defaultRenderer_t* self;
	self = (vkk_defaultRenderer_t*) base;

	uint32_t idx         = self->semaphore_index;
	*semaphore_acquire   = self->semaphore_acquire[idx];
	*semaphore_submit    = self->semaphore_submit[idx];
}

static void
vkk_defaultRenderer_endSemaphore(vkk_renderer_t* base,
                                 VkSemaphore* semaphore_acquire,
                                 VkSemaphore* semaphore_submit)
{
	ASSERT(base);
	ASSERT(semaphore_acquire);
	ASSERT(semaphore_submit);

	vkk_defaultRenderer_t* self;
	self = (vkk_defaultRenderer_t*) base;

	uint32_t idx         = self->semaphore_index;
	*semaphore_acquire   = self->semaphore_acquire[idx];
	*semaphore_submit    = self->semaphore_submit[idx];

	++idx;
	self->semaphore_index = idx%self->swapchain_image_count;
}

static int
vkk_defaultRenderer_newSwapchain(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_defaultRenderer_t* self;
	self = (vkk_defaultRenderer_t*) base;

	vkk_engine_t* engine = base->engine;

	VkSurfaceCapabilitiesKHR caps;
	if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(engine->physical_device,
	                                             engine->surface,
	                                             &caps) != VK_SUCCESS)
	{
		LOGE("vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed");
		return 0;
	}

	// check the minImageCount
	uint32_t minImageCount = 3;
	if((caps.maxImageCount > 0) &&
	   (minImageCount > caps.maxImageCount))
	{
		minImageCount = caps.maxImageCount;
	}
	else if(minImageCount < caps.minImageCount)
	{
		minImageCount = caps.minImageCount;
	}

	uint32_t sf_count;
	if(vkGetPhysicalDeviceSurfaceFormatsKHR(engine->physical_device,
	                                        engine->surface,
	                                        &sf_count,
	                                        NULL) != VK_SUCCESS)
	{
		LOGE("vkGetPhysicalDeviceSurfaceFormatsKHR failed");
		return 0;
	}

	VkSurfaceFormatKHR* sf;
	sf = (VkSurfaceFormatKHR*)
	     CALLOC(sf_count, sizeof(VkSurfaceFormatKHR));
	if(sf == NULL)
	{
		LOGE("CALLOC failed");
		return 0;
	}

	if(vkGetPhysicalDeviceSurfaceFormatsKHR(engine->physical_device,
	                                        engine->surface,
	                                        &sf_count,
	                                        sf) != VK_SUCCESS)
	{
		LOGE("vkGetPhysicalDeviceSurfaceFormatsKHR failed");
		goto fail_surface_formats3;
	}

	self->swapchain_frame       = 0;
	self->swapchain_format      = sf[0].format;
	self->swapchain_color_space = sf[0].colorSpace;
	int i;
	for(i = 0; i < sf_count; ++i)
	{
		if((sf[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) &&
		   ((sf[i].format == VK_FORMAT_R8G8B8A8_UNORM) ||
		    (sf[i].format == VK_FORMAT_B8G8R8A8_UNORM)))
		{
			self->swapchain_format      = sf[i].format;
			self->swapchain_color_space = sf[i].colorSpace;
			break;
		}
	}

	uint32_t pm_count;
	if(vkGetPhysicalDeviceSurfacePresentModesKHR(engine->physical_device,
	                                             engine->surface,
	                                             &pm_count,
	                                             NULL) != VK_SUCCESS)
	{
		goto fail_present_modes1;
	}

	VkPresentModeKHR* pm;
	pm = (VkPresentModeKHR*)
	     CALLOC(pm_count, sizeof(VkPresentModeKHR));
	if(pm == NULL)
	{
		goto fail_present_modes2;
	}

	if(vkGetPhysicalDeviceSurfacePresentModesKHR(engine->physical_device,
	                                             engine->surface,
	                                             &pm_count,
	                                             pm) != VK_SUCCESS)
	{
		goto fail_present_modes3;
	}

	VkPresentModeKHR present_mode = pm[0];
	for(i = 0; i < pm_count; ++i)
	{
		if(pm[i] == VK_PRESENT_MODE_FIFO_KHR)
		{
			present_mode = pm[i];
			break;
		}
	}

	VkSurfaceTransformFlagBitsKHR preTransform;
	if(caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
	{
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else
	{
		preTransform = caps.currentTransform;
	}

	VkSwapchainCreateInfoKHR sc_info =
	{
		.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext                 = NULL,
		.flags                 = 0,
		.surface               = engine->surface,
		.minImageCount         = minImageCount,
		.imageFormat           = self->swapchain_format,
		.imageColorSpace       = self->swapchain_color_space,
		.imageExtent           = caps.currentExtent,
		.imageArrayLayers      = 1,
		.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices   = &engine->queue_family_index,
		.preTransform          = preTransform,
		.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode           = present_mode,
		.clipped               = VK_TRUE,
		.oldSwapchain          = VK_NULL_HANDLE
	};

	self->swapchain_extent = caps.currentExtent;

	if(vkCreateSwapchainKHR(engine->device, &sc_info, NULL,
	                        &self->swapchain) != VK_SUCCESS)
	{
		LOGE("vkCreateSwapchainKHR failed");
		goto fail_swapchain;
	}

	uint32_t count = 0;
	if(vkGetSwapchainImagesKHR(engine->device,
	                           self->swapchain,
	                           &count,
	                           NULL) != VK_SUCCESS)
	{
		LOGE("vkGetSwapchainImagesKHR failed");
		goto fail_get1;
	}

	// validate swapchain_image_count across resizes
	if(self->swapchain_image_count &&
	   (self->swapchain_image_count != count))
	{
		LOGE("invalid %u, %u",
		     self->swapchain_image_count, count);
		goto fail_count;
	}
	self->swapchain_image_count = count;

	self->swapchain_images = (VkImage*)
	                         CALLOC(self->swapchain_image_count,
	                                sizeof(VkImage));
	if(self->swapchain_images == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_images;
	}

	self->swapchain_fences = (VkFence*)
	                         CALLOC(self->swapchain_image_count,
	                                sizeof(VkFence));
	if(self->swapchain_fences == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_fences;
	}

	uint32_t image_count = self->swapchain_image_count;
	if(vkGetSwapchainImagesKHR(engine->device,
	                           self->swapchain,
	                           &image_count,
	                           self->swapchain_images) != VK_SUCCESS)
	{
		LOGE("vkGetSwapchainImagesKHR failed");
		goto fail_get2;
	}

	for(i = 0; i < image_count; ++i)
	{
		VkFenceCreateInfo f_info =
		{
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = NULL,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT
		};

		if(vkCreateFence(engine->device, &f_info, NULL,
		                 &self->swapchain_fences[i]) != VK_SUCCESS)
		{
			goto fail_create_fence;
		}
	}

	FREE(pm);
	FREE(sf);

	// success
	return 1;

	// failure
	fail_create_fence:
	{
		int j;
		for(j = 0; j < i; ++j)
		{
			vkDestroyFence(engine->device,
			               self->swapchain_fences[j], NULL);
		}
	}
	fail_get2:
		FREE(self->swapchain_fences);
	fail_fences:
		FREE(self->swapchain_images);
	fail_images:
	fail_count:
	fail_get1:
		vkDestroySwapchainKHR(engine->device,
		                      self->swapchain, NULL);
	fail_swapchain:
	fail_present_modes3:
		FREE(pm);
	fail_present_modes2:
	fail_present_modes1:
	fail_surface_formats3:
		FREE(sf);
	return 0;
}

static void
vkk_defaultRenderer_deleteSwapchain(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_defaultRenderer_t* self;
	self = (vkk_defaultRenderer_t*) base;

	vkk_engine_t* engine = base->engine;

	if(self->swapchain == VK_NULL_HANDLE)
	{
		return;
	}

	int i;
	for(i = 0; i < self->swapchain_image_count; ++i)
	{
		vkDestroyFence(engine->device,
		               self->swapchain_fences[i], NULL);
	}
	FREE(self->swapchain_fences);
	FREE(self->swapchain_images);
	vkDestroySwapchainKHR(engine->device,
	                      self->swapchain, NULL);
	self->swapchain_fences = NULL;
	self->swapchain_images = NULL;
	self->swapchain        = VK_NULL_HANDLE;
}

static int
vkk_defaultRenderer_newDepth(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_defaultRenderer_t* self;
	self = (vkk_defaultRenderer_t*) base;

	vkk_engine_t* engine = base->engine;

	vkk_imageFormat_e format_depth;
	format_depth = VKK_IMAGE_FORMAT_DEPTH4X;
	if(vkk_renderer_msaaSampleCount(base) == 1)
	{
		format_depth = VKK_IMAGE_FORMAT_DEPTH1X;
	}

	self->depth_image = vkk_image_new(engine,
	                                  self->swapchain_extent.width,
	                                  self->swapchain_extent.height,
	                                  1, format_depth,
	                                  0, VKK_STAGE_DEPTH, NULL);
	if(self->depth_image == NULL)
	{
		return 0;
	}

	return 1;
}

static void
vkk_defaultRenderer_deleteDepth(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_defaultRenderer_t* self;
	self = (vkk_defaultRenderer_t*) base;

	vkk_engine_t* engine = base->engine;

	vkk_engine_deleteDefaultDepthImage(engine,
	                                   &self->depth_image);
}

static int
vkk_defaultRenderer_newMSAA(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_defaultRenderer_t* self;
	self = (vkk_defaultRenderer_t*) base;

	// check if MSAA is supported
	vkk_engine_t* engine = base->engine;
	if(engine->msaa_sample_count == 1)
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
		.format      = self->swapchain_format,
		.extent      =
		{
			.width  = self->swapchain_extent.width,
			.height = self->swapchain_extent.height,
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

	self->msaa_memory =
		vkk_memoryManager_allocImage(engine->mm,
	                                 self->msaa_image, 1);
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
		.format     = self->swapchain_format,
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
vkk_defaultRenderer_deleteMSAA(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_defaultRenderer_t* self;
	self = (vkk_defaultRenderer_t*) base;

	vkk_engine_t* engine = base->engine;

	vkDestroyImageView(engine->device, self->msaa_image_view,
	                   NULL);
	vkk_memoryManager_free(engine->mm, &self->msaa_memory);
	vkDestroyImage(engine->device, self->msaa_image, NULL);
	self->msaa_image_view = VK_NULL_HANDLE;
	self->msaa_image      = VK_NULL_HANDLE;
}

static int
vkk_defaultRenderer_newFramebuffer(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_defaultRenderer_t* self;
	self = (vkk_defaultRenderer_t*) base;

	vkk_engine_t* engine = base->engine;

	self->framebuffer_image_views = (VkImageView*)
	                                CALLOC(self->swapchain_image_count,
	                                       sizeof(VkImageView));
	if(self->framebuffer_image_views == NULL)
	{
		LOGE("CALLOC failed");
		return 0;
	}

	self->framebuffers = (VkFramebuffer*)
	                     CALLOC(self->swapchain_image_count,
	                            sizeof(VkFramebuffer));
	if(self->framebuffers == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_alloc_framebuffers;
	}

	int i;
	for(i = 0; i < self->swapchain_image_count; ++i)
	{
		VkImageViewCreateInfo iv_info =
		{
			.sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext      = NULL,
			.flags      = 0,
			.image      = self->swapchain_images[i],
			.viewType   = VK_IMAGE_VIEW_TYPE_2D,
			.format     = self->swapchain_format,
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
		                     &self->framebuffer_image_views[i]) != VK_SUCCESS)
		{
			LOGE("vkCreateImageView failed");
			goto fail_image_view;
		}

		VkImageView attachments[] =
		{
			self->framebuffer_image_views[i],
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
			.width           = self->swapchain_extent.width,
			.height          = self->swapchain_extent.height,
			.layers          = 1,
		};

		if(vkCreateFramebuffer(engine->device, &f_info, NULL,
		                       &self->framebuffers[i]) != VK_SUCCESS)
		{
			vkDestroyImageView(engine->device,
			                   self->framebuffer_image_views[i],
			                   NULL);
			self->framebuffer_image_views[i] = VK_NULL_HANDLE;

			LOGE("vkCreateFramebuffer failed");
			goto fail_framebuffer;
		}
	}

	// success
	return 1;

	// failure
	fail_framebuffer:
	fail_image_view:
	{
		int j;
		for(j = 0; j < i; ++j)
		{
			vkDestroyFramebuffer(engine->device,
			                     self->framebuffers[j],
			                     NULL);
			vkDestroyImageView(engine->device,
			                   self->framebuffer_image_views[j],
			                   NULL);
			self->framebuffers[j]            = VK_NULL_HANDLE;
			self->framebuffer_image_views[j] = VK_NULL_HANDLE;
		}
		FREE(self->framebuffers);
		self->framebuffers = NULL;
	}
	fail_alloc_framebuffers:
		FREE(self->framebuffer_image_views);
		self->framebuffer_image_views = NULL;
	return 0;
}

static void
vkk_defaultRenderer_deleteFramebuffer(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_defaultRenderer_t* self;
	self = (vkk_defaultRenderer_t*) base;

	vkk_engine_t* engine = base->engine;

	if(self->framebuffers == NULL)
	{
		return;
	}

	int i;
	for(i = 0; i < self->swapchain_image_count; ++i)
	{
		vkDestroyFramebuffer(engine->device,
		                     self->framebuffers[i],
		                     NULL);
		vkDestroyImageView(engine->device,
		                   self->framebuffer_image_views[i],
		                   NULL);
		self->framebuffers[i]            = VK_NULL_HANDLE;
		self->framebuffer_image_views[i] = VK_NULL_HANDLE;
	}
	FREE(self->framebuffers);
	FREE(self->framebuffer_image_views);
	self->framebuffers            = NULL;
	self->framebuffer_image_views = NULL;
}

static int
vkk_defaultRenderer_newRenderpass(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_defaultRenderer_t* self;
	self = (vkk_defaultRenderer_t*) base;

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
			.format         = self->swapchain_format,
			.samples        = VK_SAMPLE_COUNT_1_BIT,
			.loadOp         = load_op,
			.storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
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
			.format         = self->swapchain_format,
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
vkk_defaultRenderer_newSemaphores(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_defaultRenderer_t* self;
	self = (vkk_defaultRenderer_t*) base;

	vkk_engine_t* engine = base->engine;

	self->semaphore_index   = 0;
	self->semaphore_acquire = (VkSemaphore*)
	                          CALLOC(self->swapchain_image_count,
	                                 sizeof(VkSemaphore));
	if(self->semaphore_acquire == NULL)
	{
		LOGE("CALLOC failed");
		return 0;
	}

	self->semaphore_submit = (VkSemaphore*)
	                         CALLOC(self->swapchain_image_count,
	                                sizeof(VkSemaphore));
	if(self->semaphore_submit == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_alloc_ss;
	}

	int i;
	for(i = 0; i < self->swapchain_image_count; ++i)
	{
		VkSemaphoreCreateInfo sa_info =
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0
		};

		if(vkCreateSemaphore(engine->device, &sa_info, NULL,
		                     &self->semaphore_acquire[i]) != VK_SUCCESS)
		{
			LOGE("vkCreateSemaphore failed");
			goto fail_create;
		}

		VkSemaphoreCreateInfo ss_info =
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0
		};

		if(vkCreateSemaphore(engine->device, &ss_info, NULL,
		                     &self->semaphore_submit[i]) != VK_SUCCESS)
		{
			vkDestroySemaphore(engine->device,
			                   self->semaphore_acquire[i],
			                   NULL);

			LOGE("vkCreateSemaphore failed");
			goto fail_create;
		}
	}

	// success
	return 1;

	// failure
	fail_create:
	{
		int j;
		for(j = 0; j < i; ++j)
		{
			vkDestroySemaphore(engine->device,
			                   self->semaphore_submit[j],
			                   NULL);
			vkDestroySemaphore(engine->device,
			                   self->semaphore_acquire[j],
			                   NULL);
		}
		FREE(self->semaphore_submit);
	}
	fail_alloc_ss:
		FREE(self->semaphore_acquire);
	return 0;
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_renderer_t*
vkk_defaultRenderer_new(vkk_engine_t* engine)
{
	ASSERT(engine);

	vkk_defaultRenderer_t* self;
	self = (vkk_defaultRenderer_t*)
	       CALLOC(1, sizeof(vkk_defaultRenderer_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	vkk_renderer_t* base = &(self->base);
	vkk_renderer_init(base, VKK_RENDERER_TYPE_DEFAULT,
	                  engine);

	if(vkk_defaultRenderer_newSwapchain(base) == 0)
	{
		goto fail_swapchain;
	}

	if(vkk_defaultRenderer_newRenderpass(base) == 0)
	{
		goto fail_renderpass;
	}

	if(vkk_defaultRenderer_newDepth(base) == 0)
	{
		goto fail_depth;
	}

	if(vkk_defaultRenderer_newMSAA(base) == 0)
	{
		goto fail_msaa;
	}

	if(vkk_defaultRenderer_newFramebuffer(base) == 0)
	{
		goto fail_framebuffer;
	}

	self->cmd_buffers = vkk_commandBuffer_new(engine,
	                                          self->swapchain_image_count,
	                                          0);
	if(self->cmd_buffers == NULL)
	{
		goto fail_cmd_buffers;
	}

	self->ts_array = (double*)
	                 CALLOC(self->swapchain_image_count,
	                        sizeof(double));
	if(self->ts_array == NULL)
	{
		goto fail_timestamps;
	}

	if(vkk_defaultRenderer_newSemaphores(base) == 0)
	{
		goto fail_semaphores;
	}

	// success
	return (vkk_renderer_t*) self;

	// failure
	fail_semaphores:
		FREE(self->ts_array);
	fail_timestamps:
		vkk_commandBuffer_delete(&self->cmd_buffers);
	fail_cmd_buffers:
		vkk_defaultRenderer_deleteFramebuffer(base);
	fail_framebuffer:
		vkk_defaultRenderer_deleteMSAA(base);
	fail_msaa:
		vkk_defaultRenderer_deleteDepth(base);
	fail_depth:
		vkDestroyRenderPass(engine->device,
		                    self->render_pass, NULL);
	fail_renderpass:
		vkk_defaultRenderer_deleteSwapchain(base);
	fail_swapchain:
		FREE(self);
	return NULL;
}

void vkk_defaultRenderer_delete(vkk_renderer_t** _base)
{
	ASSERT(_base);

	vkk_renderer_t* base = *_base;
	if(base)
	{
		vkk_defaultRenderer_t* self;
		self = (vkk_defaultRenderer_t*) base;

		vkk_engine_t* engine = base->engine;

		FREE(base->wait_flags);
		FREE(base->wait_array);

		int i;
		for(i = 0; i < self->swapchain_image_count; ++i)
		{
			vkDestroySemaphore(engine->device,
			                   self->semaphore_submit[i],
			                   NULL);
			vkDestroySemaphore(engine->device,
			                   self->semaphore_acquire[i],
			                   NULL);
		}
		FREE(self->semaphore_submit);
		FREE(self->semaphore_acquire);

		FREE(self->ts_array);

		vkk_commandBuffer_delete(&self->cmd_buffers);
		vkk_defaultRenderer_deleteFramebuffer(base);
		vkk_defaultRenderer_deleteMSAA(base);
		vkk_defaultRenderer_deleteDepth(base);
		vkDestroyRenderPass(engine->device,
		                    self->render_pass, NULL);
		vkk_defaultRenderer_deleteSwapchain(base);
		FREE(self);
		*_base = NULL;
	}
}

int vkk_defaultRenderer_resize(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_engine_t* engine = base->engine;

	vkk_defaultRenderer_t* self;
	self = (vkk_defaultRenderer_t*) base;

	vkDeviceWaitIdle(engine->device);

	vkk_defaultRenderer_deleteMSAA(base);
	vkk_defaultRenderer_deleteDepth(base);
	vkk_defaultRenderer_deleteFramebuffer(base);
	vkk_defaultRenderer_deleteSwapchain(base);

	if(vkk_defaultRenderer_newSwapchain(base) == 0)
	{
		return 0;
	}

	if(vkk_defaultRenderer_newDepth(base) == 0)
	{
		goto fail_depth;
	}

	if(vkk_defaultRenderer_newMSAA(base) == 0)
	{
		goto fail_msaa;
	}

	if(vkk_defaultRenderer_newFramebuffer(base) == 0)
	{
		goto fail_framebuffer;
	}

	self->resize = 0;

	// success
	return 1;

	// failure
	fail_framebuffer:
		vkk_defaultRenderer_deleteMSAA(base);
	fail_msaa:
		vkk_defaultRenderer_deleteDepth(base);
	fail_depth:
		vkk_defaultRenderer_deleteSwapchain(base);
	return 0;
}

int vkk_defaultRenderer_recreate(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_engine_t* engine = base->engine;

	vkDeviceWaitIdle(engine->device);

	vkk_defaultRenderer_deleteMSAA(base);
	vkk_defaultRenderer_deleteDepth(base);
	vkk_defaultRenderer_deleteFramebuffer(base);
	vkk_defaultRenderer_deleteSwapchain(base);
	vkk_engine_deleteSurface(engine);

	if(vkk_engine_newSurface(engine) == 0)
	{
		return 0;
	}

	if(vkk_defaultRenderer_newSwapchain(base) == 0)
	{
		goto fail_swapchain;
	}

	if(vkk_defaultRenderer_newDepth(base) == 0)
	{
		goto fail_depth;
	}

	if(vkk_defaultRenderer_newMSAA(base) == 0)
	{
		goto fail_msaa;
	}

	if(vkk_defaultRenderer_newFramebuffer(base) == 0)
	{
		goto fail_framebuffer;
	}

	// success
	return 1;

	// failure
	fail_framebuffer:
		vkk_defaultRenderer_deleteMSAA(base);
	fail_msaa:
		vkk_defaultRenderer_deleteDepth(base);
	fail_depth:
		vkk_defaultRenderer_deleteSwapchain(base);
	fail_swapchain:
		vkk_engine_deleteSurface(engine);
	return 0;
}

double vkk_defaultRenderer_tsCurrent(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_defaultRenderer_t* self;
	self = (vkk_defaultRenderer_t*) base;

	return self->ts_array[self->swapchain_frame];
}

double
vkk_defaultRenderer_tsExpiredLocked(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_defaultRenderer_t* self;
	self = (vkk_defaultRenderer_t*) base;

	return self->ts_expired;
}

void vkk_defaultRenderer_deviceWaitIdle(vkk_renderer_t* base)
{
	vkk_defaultRenderer_t* self;
	self = (vkk_defaultRenderer_t*) base;

	vkk_engine_t* engine = base->engine;

	vkDeviceWaitIdle(engine->device);

	// expire the completed frame
	int i;
	vkk_engine_rendererLock(engine);
	for(i = 0; i < self->swapchain_image_count; ++i)
	{
		if(self->ts_array[i] > self->ts_expired)
		{
			self->ts_expired = self->ts_array[i];
		}
	}
	vkk_engine_rendererSignal(base->engine);
	vkk_engine_rendererUnlock(engine);
}

int
vkk_defaultRenderer_begin(vkk_renderer_t* base,
                          vkk_rendererMode_e mode,
                          float* clear_color)
{
	ASSERT(base);
	ASSERT(clear_color);

	vkk_defaultRenderer_t* self;
	self = (vkk_defaultRenderer_t*) base;

	vkk_engine_t* engine = base->engine;

	// check if a resize event was detected
	if(self->resize &&
	   (vkk_defaultRenderer_resize(base) == 0))
	{
		return 0;
	}

	VkSemaphore semaphore_acquire;
	VkSemaphore semaphore_submit;
	vkk_defaultRenderer_beginSemaphore(base,
	                                   &semaphore_acquire,
	                                   &semaphore_submit);

	// Android only supports infinite timeout
	// Linux needs a timeout to avoid deadlock on resize
	#ifdef ANDROID
		uint64_t timeout = UINT64_MAX;
	#else
		uint64_t timeout = 250000000;
	#endif
	VkResult acquire;
	acquire = vkAcquireNextImageKHR(engine->device,
	                                self->swapchain,
	                                timeout,
	                                semaphore_acquire,
	                                VK_NULL_HANDLE,
	                                &self->swapchain_frame);
	if((acquire == VK_SUCCESS) ||
	   (acquire == VK_SUBOPTIMAL_KHR))
	{
		// ignore
	}
	else if(acquire == VK_ERROR_OUT_OF_DATE_KHR)
	{
		self->resize = 1;
		goto fail_acquire;
	}
	else
	{
		LOGW("acquire=%i", (int) acquire);
		goto fail_acquire;
	}

	VkSurfaceCapabilitiesKHR caps;
	if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(engine->physical_device,
	                                             engine->surface,
	                                             &caps) != VK_SUCCESS)
	{
		LOGE("vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed");
		goto fail_caps;
	}

	// check for resizes
	if((self->swapchain_extent.width  != caps.currentExtent.width) ||
	   (self->swapchain_extent.height != caps.currentExtent.height))
	{
		self->resize = 1;
		goto fail_resize;
	}

	// wait for a frame to complete
	VkFence sc_fence;
	sc_fence = self->swapchain_fences[self->swapchain_frame];
	vkWaitForFences(engine->device, 1,
	                &sc_fence, VK_TRUE, UINT64_MAX);
	vkResetFences(engine->device, 1, &sc_fence);

	// expire the completed frame
	vkk_engine_rendererLock(engine);
	if(self->ts_array[self->swapchain_frame] > self->ts_expired)
	{
		self->ts_expired = self->ts_array[self->swapchain_frame];
		vkk_engine_rendererSignal(base->engine);
	}
	self->ts_array[self->swapchain_frame] = cc_timestamp();
	vkk_engine_rendererUnlock(engine);

	VkCommandBuffer cb;
	cb = vkk_commandBuffer_get(self->cmd_buffers,
	                           self->swapchain_frame);
	if(vkResetCommandBuffer(cb, 0) != VK_SUCCESS)
	{
		LOGE("vkResetCommandBuffer failed");
		goto fail_reset_cb;
	}

	VkFramebuffer framebuffer;
	framebuffer = self->framebuffers[self->swapchain_frame];
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
		goto fail_begin_cb;
	}

	// stage only applies to textures
	VkImage image;
	image = self->swapchain_images[self->swapchain_frame];
	vkk_util_imageMemoryBarrierRaw(image, cb, 0,
	                               VK_IMAGE_LAYOUT_UNDEFINED,
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
			.width    = (float) self->swapchain_extent.width,
			.height   = (float) self->swapchain_extent.height,
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
				.width  = (uint32_t) self->swapchain_extent.width,
				.height = (uint32_t) self->swapchain_extent.height,
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

	uint32_t width  = self->swapchain_extent.width;
	uint32_t height = self->swapchain_extent.height;
	VkRenderPassBeginInfo rp_info =
	{
		.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext           = NULL,
		.renderPass      = self->render_pass,
		.framebuffer     = framebuffer,
		.renderArea      = { { .x=0, .y=0 },
		                     { .width=width,
		                       .height=height } },
		.clearValueCount = cv_count,
		.pClearValues    = cv
	};

	VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE;
	if(mode == VKK_RENDERER_MODE_EXECUTE)
	{
		contents = VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;
	}

	vkCmdBeginRenderPass(cb, &rp_info, contents);

	// success
	return 1;

	// failure
	fail_begin_cb:
	fail_reset_cb:
	fail_resize:
	fail_caps:
	fail_acquire:
		vkk_defaultRenderer_endSemaphore(base,
		                                 &semaphore_acquire,
		                                 &semaphore_submit);
	return 0;
}

void vkk_defaultRenderer_end(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_defaultRenderer_t* self;
	self = (vkk_defaultRenderer_t*) base;

	vkk_engine_t* engine = base->engine;

	VkSemaphore semaphore_acquire;
	VkSemaphore semaphore_submit;
	vkk_defaultRenderer_endSemaphore(base,
	                                 &semaphore_acquire,
	                                 &semaphore_submit);
	vkk_renderer_addWaitSemaphore(base, semaphore_acquire);

	VkCommandBuffer cb;
	cb = vkk_commandBuffer_get(self->cmd_buffers,
	                           self->swapchain_frame);
	vkCmdEndRenderPass(cb);
	vkEndCommandBuffer(cb);

	VkFence sc_fence;
	sc_fence = self->swapchain_fences[self->swapchain_frame];

	if(vkk_engine_queueSubmit(engine, VKK_QUEUE_FOREGROUND, &cb,
	                          base->wait_count,
	                          base->wait_array,
	                          &semaphore_submit,
	                          base->wait_flags,
	                          sc_fence) == 0)
	{
		return;
	}

	VkPresentInfoKHR p_info =
	{
		.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext              = NULL,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores    = &semaphore_submit,
		.swapchainCount     = 1,
		.pSwapchains        = &self->swapchain,
		.pImageIndices      = &self->swapchain_frame,
		.pResults           = NULL
	};

	VkResult present;
	present = vkQueuePresentKHR(engine->queue[VKK_QUEUE_FOREGROUND],
	                            &p_info);
	if((present == VK_SUCCESS) ||
	   (present == VK_SUBOPTIMAL_KHR))
	{
		// ignore
	}
	else if(present == VK_ERROR_OUT_OF_DATE_KHR)
	{
		self->resize = 1;
	}
	else
	{
		LOGW("present=%i", (int) present);
	}
}

void
vkk_defaultRenderer_surfaceSize(vkk_renderer_t* base,
                                uint32_t* _width,
                                uint32_t* _height)
{
	ASSERT(base);
	ASSERT(_width);
	ASSERT(_height);

	vkk_defaultRenderer_t* self;
	self = (vkk_defaultRenderer_t*) base;

	*_width  = self->swapchain_extent.width;
	*_height = self->swapchain_extent.height;
}

VkRenderPass
vkk_defaultRenderer_renderPass(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_defaultRenderer_t* self;
	self = (vkk_defaultRenderer_t*) base;

	return self->render_pass;
}

VkFramebuffer
vkk_defaultRenderer_framebuffer(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_defaultRenderer_t* self;
	self = (vkk_defaultRenderer_t*) base;

	return self->framebuffers[self->swapchain_frame];
}

VkCommandBuffer
vkk_defaultRenderer_commandBuffer(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_defaultRenderer_t* self;
	self = (vkk_defaultRenderer_t*) base;

	return vkk_commandBuffer_get(self->cmd_buffers,
	                             self->swapchain_frame);
}

uint32_t
vkk_defaultRenderer_frame(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_defaultRenderer_t* self;
	self = (vkk_defaultRenderer_t*) base;

	return self->swapchain_frame;
}

uint32_t
vkk_defaultRenderer_imageCount(vkk_renderer_t* base)
{
	ASSERT(base);

	vkk_defaultRenderer_t* self;
	self = (vkk_defaultRenderer_t*) base;

	return self->swapchain_image_count;
}
