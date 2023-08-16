/*
 * Copyright (c) 2020 Jeff Boody
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
#include "vkk_buffer.h"
#include "vkk_commandBuffer.h"
#include "vkk_engine.h"
#include "vkk_xferManager.h"
#include "vkk_image.h"
#include "vkk_memoryChunk.h"
#include "vkk_memoryManager.h"
#include "vkk_memoryPool.h"
#include "vkk_memory.h"
#include "vkk_renderer.h"
#include "vkk_util.h"

typedef struct vkk_xferBuffer_s
{
	VkBuffer      buffer;
	vkk_memory_t* memory;
} vkk_xferBuffer_t;

typedef struct vkk_xferInstance_s
{
	VkFence              fence;
	vkk_commandBuffer_t* cmd_buffer;
} vkk_xferInstance_t;

/***********************************************************
* private                                                  *
***********************************************************/

static vkk_xferBuffer_t*
vkk_xferBuffer_new(vkk_engine_t* engine, size_t size,
                   const void* data)
{
	// data may be NULL
	ASSERT(engine);

	vkk_xferBuffer_t* self;
	self = (vkk_xferBuffer_t*)
	       CALLOC(1, sizeof(vkk_xferBuffer_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	// create a transfer buffer
	VkBufferCreateInfo b_info =
	{
		.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext                 = NULL,
		.flags                 = 0,
		.size                  = size,
		.usage                 = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		.sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices   = &engine->queue_family_index
	};

	if(vkCreateBuffer(engine->device, &b_info, NULL,
	                  &self->buffer) != VK_SUCCESS)
	{
		LOGE("vkCreateBuffer failed");
		goto fail_buffer;
	}

	self->memory = vkk_memoryManager_allocBuffer(engine->mm,
	                                             self->buffer,
	                                             0, size, data);
	if(self->memory == NULL)
	{
		goto fail_alloc;
	}

	// success
	return self;

	// failure
	fail_alloc:
		vkDestroyBuffer(engine->device, self->buffer, NULL);
	fail_buffer:
		FREE(self);
	return NULL;
}

static void
vkk_xferBuffer_delete(vkk_xferBuffer_t** _self)
{
	ASSERT(_self);

	vkk_xferBuffer_t* self = *_self;
	if(self)
	{
		vkk_engine_t* engine;
		engine = self->memory->chunk->pool->mm->engine;

		vkk_memoryManager_free(engine->mm, &self->memory);
		vkDestroyBuffer(engine->device, self->buffer, NULL);
		FREE(self);
		*_self = NULL;
	}
}

static vkk_xferInstance_t*
vkk_xferInstance_new(vkk_engine_t* engine)
{
	ASSERT(engine);

	vkk_xferInstance_t* self;
	self = (vkk_xferInstance_t*)
	       CALLOC(1, sizeof(vkk_xferInstance_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	VkFenceCreateInfo f_info =
	{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0
	};

	if(vkCreateFence(engine->device, &f_info, NULL,
	                 &self->fence) != VK_SUCCESS)
	{
		LOGE("vkCreateFence failed");
		goto fail_fence;
	}

	// allocate a transfer command buffer
	self->cmd_buffer = vkk_commandBuffer_new(engine, 1, 0);
	if(self->cmd_buffer == NULL)
	{
		goto fail_cmd_buffer;
	}

	// success
	return self;

	// failure
	fail_cmd_buffer:
		vkDestroyFence(engine->device, self->fence, NULL);
	fail_fence:
		FREE(self);
	return NULL;
}

static void
vkk_xferInstance_delete(vkk_xferInstance_t** _self)
{
	ASSERT(_self);

	vkk_xferInstance_t* self = *_self;
	if(self)
	{
		vkk_engine_t* engine;
		engine = self->cmd_buffer->engine;

		vkk_commandBuffer_delete(&self->cmd_buffer);
		vkDestroyFence(engine->device, self->fence, NULL);
		FREE(self);
		*_self = NULL;
	}
}

static void
vkk_xferManager_lock(vkk_xferManager_t* self)
{
	ASSERT(self);

	pthread_mutex_lock(&self->mutex);
	TRACE_BEGIN();
}

static void
vkk_xferManager_unlock(vkk_xferManager_t* self)
{
	ASSERT(self);

	TRACE_END();
	pthread_mutex_unlock(&self->mutex);
}

static int
vkk_xferManager_writeImageF16(vkk_xferManager_t* self,
                              vkk_image_t* image,
                              const void* pixels)
{
	ASSERT(self);
	ASSERT(image);
	ASSERT(pixels);

	vkk_engine_t* engine = self->engine;

	// tmp image is F32 format (F16 format - 1)
	vkk_image_t* tmp;
	tmp = vkk_image_new(engine,
	                    image->width, image->height,
	                    image->depth, image->format - 1,
	                    image->mipmap, image->stage,
	                    pixels);
	if(tmp == NULL)
	{
		return 0;
	}

	vkk_xferManager_lock(self);

	vkk_xferInstance_t* xi;
	cc_listIter_t* iter = cc_list_head(self->instance_list);
	if(iter)
	{
		xi = (vkk_xferInstance_t*)
		     cc_list_remove(self->instance_list, &iter);
	}
	else
	{
		xi = vkk_xferInstance_new(engine);
		if(xi == NULL)
		{
			vkk_xferManager_unlock(self);
			goto fail_xi;
		}
	}
	vkk_xferManager_unlock(self);

	VkCommandBuffer cb;
	cb = vkk_commandBuffer_get(xi->cmd_buffer, 0);

	vkResetFences(engine->device, 1, &xi->fence);
	if(vkResetCommandBuffer(cb, 0) != VK_SUCCESS)
	{
		LOGE("vkResetCommandBuffer failed");
		goto fail_cb;
	}

	// begin the transfer commands
	VkCommandBufferInheritanceInfo cbi_info =
	{
		.sType                = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		.pNext                = NULL,
		.renderPass           = VK_NULL_HANDLE,
		.subpass              = 0,
		.framebuffer          = VK_NULL_HANDLE,
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

	// transition tmp to src for blitting
	vkk_util_imageMemoryBarrier(tmp, cb,
	                            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	                            0, tmp->mip_levels);

	// transition image to dst for blitting
	vkk_util_imageMemoryBarrier(image, cb,
	                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	                            0, image->mip_levels);

	// blit each mip_level from tmp to image
	int i;
	uint32_t w = image->width;
	uint32_t h = image->height;
	uint32_t d = image->depth;
	for(i = 0; i < image->mip_levels; ++i)
	{
		VkImageBlit ib =
		{
			.srcSubresource =
			{
				.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel       = i,
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
					.x = (uint32_t) (w >> i),
					.y = (uint32_t) (h >> i),
					.z = (uint32_t) (d >> i),
				}
			},
			.dstSubresource =
			{
				.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel       = i,
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
					.x = (uint32_t) (w >> i),
					.y = (uint32_t) (h >> i),
					.z = (uint32_t) (d >> i),
				}
			}
		};

		// enforce the minimum size
		if(ib.srcOffsets[1].x == 0)
		{
			ib.srcOffsets[1].x = 1;
		}
		if(ib.srcOffsets[1].y == 0)
		{
			ib.srcOffsets[1].y = 1;
		}
		if(ib.srcOffsets[1].z == 0)
		{
			ib.srcOffsets[1].z = 1;
		}
		if(ib.dstOffsets[1].x == 0)
		{
			ib.dstOffsets[1].x = 1;
		}
		if(ib.dstOffsets[1].y == 0)
		{
			ib.dstOffsets[1].y = 1;
		}
		if(ib.dstOffsets[1].z == 0)
		{
			ib.dstOffsets[1].z = 1;
		}

		vkCmdBlitImage(cb,
		               tmp->image,
		               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		               image->image,
		               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		               1, &ib, VK_FILTER_NEAREST);
	}

	// transition image shading mode
	vkk_util_imageMemoryBarrier(image, cb,
	                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	                            0, image->mip_levels);

	// end the transfer commands
	vkEndCommandBuffer(cb);

	// submit the commands
	if(vkk_engine_queueSubmit(engine, VKK_QUEUE_BACKGROUND, &cb,
	                          0, NULL, NULL, NULL,
	                          xi->fence) == 0)
	{
		goto fail_submit;
	}

	uint64_t timeout = UINT64_MAX;
	if(vkWaitForFences(engine->device, 1, &xi->fence, VK_TRUE,
	                   timeout) != VK_SUCCESS)
	{
		LOGW("vkWaitForFences failed");
		vkk_engine_queueWaitIdle(engine, VKK_QUEUE_BACKGROUND);
	}

	vkk_xferManager_lock(self);
	if(cc_list_append(self->instance_list, NULL,
	                  (const void*) xi) == NULL)
	{
		vkk_xferInstance_delete(&xi);
	}
	vkk_xferManager_unlock(self);

	vkk_image_delete(&tmp);

	// success
	return 1;

	// failure
	fail_submit:
	fail_begin_cb:
	fail_cb:
		vkk_xferInstance_delete(&xi);
	fail_xi:
		vkk_image_delete(&tmp);
	return 0;
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_xferManager_t*
vkk_xferManager_new(vkk_engine_t* engine)
{
	ASSERT(engine);

	vkk_xferManager_t* self;
	self = (vkk_xferManager_t*)
	       CALLOC(1, sizeof(vkk_xferManager_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->engine = engine;

	self->instance_list = cc_list_new();
	if(self->instance_list == NULL)
	{
		goto fail_instance_list;
	}

	self->buffer_map = cc_multimap_new(NULL);
	if(self->buffer_map == NULL)
	{
		goto fail_buffer_map;
	}

	if(pthread_mutex_init(&self->mutex, NULL) != 0)
	{
		LOGE("pthread_mutex_init failed");
		goto fail_mutex;
	}

	// success
	return self;

	// failure
	fail_mutex:
		cc_multimap_delete(&self->buffer_map);
	fail_buffer_map:
		cc_list_delete(&self->instance_list);
	fail_instance_list:
		FREE(self);
	return NULL;
}

void vkk_xferManager_delete(vkk_xferManager_t** _self)
{
	ASSERT(_self);

	vkk_xferManager_t* self = *_self;
	if(self)
	{
		cc_listIter_t* iter = cc_list_head(self->instance_list);
		while(iter)
		{
			vkk_xferInstance_t* xi;
			xi = (vkk_xferInstance_t*)
			     cc_list_remove(self->instance_list, &iter);
			vkk_xferInstance_delete(&xi);
		}

		cc_multimapIter_t  miterator;
		cc_multimapIter_t* miter;
		miter = cc_multimap_head(self->buffer_map, &miterator);
		while(miter)
		{
			vkk_xferBuffer_t* xb;
			xb = (vkk_xferBuffer_t*)
			     cc_multimap_remove(self->buffer_map, &miter);
			vkk_xferBuffer_delete(&xb);
		}

		pthread_mutex_destroy(&self->mutex);
		cc_multimap_delete(&self->buffer_map);
		cc_list_delete(&self->instance_list);
		FREE(self);
		*_self = NULL;
	}
}

void vkk_xferManager_shutdown(vkk_xferManager_t* self)
{
	ASSERT(self);

	vkk_xferManager_lock(self);
	self->shutdown = 1;
	vkk_xferManager_unlock(self);
}

int vkk_xferManager_blitStorage(vkk_xferManager_t* self,
                                vkk_xferMode_e mode,
                                vkk_buffer_t* buffer,
                                size_t size,
                                size_t offset,
                                void* data)
{
	ASSERT(self);
	ASSERT(buffer);
	ASSERT((size + offset) <= vkk_buffer_size(buffer));
	ASSERT(data);

	vkk_engine_t* engine = self->engine;

	vkk_xferManager_lock(self);
	if(self->shutdown)
	{
		vkk_xferManager_unlock(self);
		return 0;
	}

	vkk_xferBuffer_t*  xb;
	cc_multimapIter_t  miterator;
	cc_multimapIter_t* miter = &miterator;
	if(cc_multimap_findp(self->buffer_map, miter,
	                     sizeof(size_t), &size))
	{
		xb = (vkk_xferBuffer_t*)
		     cc_multimap_remove(self->buffer_map, &miter);
		if(mode == VKK_XFER_MODE_WRITE)
		{
			vkk_memoryManager_write(engine->mm, xb->memory,
			                        size, 0, data);
		}
	}
	else
	{
		if(mode == VKK_XFER_MODE_WRITE)
		{
			xb = vkk_xferBuffer_new(engine, size, data);
		}
		else
		{
			xb = vkk_xferBuffer_new(engine, size, NULL);
		}

		if(xb == NULL)
		{
			vkk_xferManager_unlock(self);
			return 0;
		}
	}

	vkk_xferInstance_t* xi;
	cc_listIter_t* iter = cc_list_head(self->instance_list);
	if(iter)
	{
		xi = (vkk_xferInstance_t*)
		     cc_list_remove(self->instance_list, &iter);
	}
	else
	{
		xi = vkk_xferInstance_new(engine);
		if(xi == NULL)
		{
			vkk_xferManager_unlock(self);
			goto fail_xi;
		}
	}
	vkk_xferManager_unlock(self);

	VkCommandBuffer cb;
	cb = vkk_commandBuffer_get(xi->cmd_buffer, 0);

	vkResetFences(engine->device, 1, &xi->fence);
	if(vkResetCommandBuffer(cb, 0) != VK_SUCCESS)
	{
		LOGE("vkResetCommandBuffer failed");
		goto fail_cb;
	}

	// begin the transfer commands
	VkCommandBufferInheritanceInfo cbi_info =
	{
		.sType                = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		.pNext                = NULL,
		.renderPass           = VK_NULL_HANDLE,
		.subpass              = 0,
		.framebuffer          = VK_NULL_HANDLE,
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

	int idx = 0;
	if(mode == VKK_XFER_MODE_WRITE)
	{
		VkBufferCopy bc =
		{
			.srcOffset = 0,
			.dstOffset = offset,
			.size      = size,
		};

		vkCmdCopyBuffer(cb, xb->buffer,
		                buffer->buffer[idx], 1, &bc);
	}
	else
	{
		VkBufferCopy bc =
		{
			.srcOffset = offset,
			.dstOffset = 0,
			.size      = size,
		};

		vkCmdCopyBuffer(cb, buffer->buffer[idx],
		                xb->buffer, 1, &bc);
	}

	// end the transfer commands
	vkEndCommandBuffer(cb);

	// submit the commands
	if(vkk_engine_queueSubmit(engine, VKK_QUEUE_BACKGROUND, &cb,
	                          0, NULL, NULL, NULL,
	                          xi->fence) == 0)
	{
		goto fail_submit;
	}

	uint64_t timeout = UINT64_MAX;
	if(vkWaitForFences(engine->device, 1, &xi->fence, VK_TRUE,
	                   timeout) != VK_SUCCESS)
	{
		LOGW("vkWaitForFences failed");
		vkk_engine_queueWaitIdle(engine, VKK_QUEUE_BACKGROUND);
	}

	if(mode == VKK_XFER_MODE_READ)
	{
		vkk_memoryManager_read(engine->mm, xb->memory,
		                       size, 0, data);
	}

	vkk_xferManager_lock(self);
	if(cc_list_append(self->instance_list, NULL,
	                  (const void*) xi) == NULL)
	{
		vkk_xferInstance_delete(&xi);
	}

	if(cc_multimap_addp(self->buffer_map, (const void*) xb,
	                    sizeof(size_t), &size) == 0)
	{
		vkk_xferBuffer_delete(&xb);
	}
	vkk_xferManager_unlock(self);

	// success
	return 1;

	// failure
	fail_submit:
	fail_begin_cb:
	fail_cb:
		vkk_xferInstance_delete(&xi);
	fail_xi:
		vkk_xferBuffer_delete(&xb);
	return 0;
}

int vkk_xferManager_readImage(vkk_xferManager_t* self,
                              vkk_image_t* image,
                              void* pixels)
{
	ASSERT(self);
	ASSERT(image);
	ASSERT(pixels);

	vkk_engine_t* engine = self->engine;

	vkk_xferManager_lock(self);
	if(self->shutdown)
	{
		vkk_xferManager_unlock(self);
		return 0;
	}

	uint32_t width;
	uint32_t height;
	uint32_t depth;
	size_t   size;
	size = vkk_image_size(image, &width, &height, &depth);

	vkk_xferBuffer_t*  xb;
	cc_multimapIter_t  miterator;
	cc_multimapIter_t* miter = &miterator;
	if(cc_multimap_findp(self->buffer_map, miter,
	                     sizeof(size_t), &size))
	{
		xb = (vkk_xferBuffer_t*)
		     cc_multimap_remove(self->buffer_map, &miter);
	}
	else
	{
		xb = vkk_xferBuffer_new(engine, size, NULL);
		if(xb == NULL)
		{
			vkk_xferManager_unlock(self);
			return 0;
		}
	}

	vkk_xferInstance_t* xi;
	cc_listIter_t* iter = cc_list_head(self->instance_list);
	if(iter)
	{
		xi = (vkk_xferInstance_t*)
		     cc_list_remove(self->instance_list, &iter);
	}
	else
	{
		xi = vkk_xferInstance_new(engine);
		if(xi == NULL)
		{
			vkk_xferManager_unlock(self);
			goto fail_xi;
		}
	}
	vkk_xferManager_unlock(self);

	VkCommandBuffer cb;
	cb = vkk_commandBuffer_get(xi->cmd_buffer, 0);

	vkResetFences(engine->device, 1, &xi->fence);
	if(vkResetCommandBuffer(cb, 0) != VK_SUCCESS)
	{
		LOGE("vkResetCommandBuffer failed");
		goto fail_cb;
	}

	// begin the transfer commands
	VkCommandBufferInheritanceInfo cbi_info =
	{
		.sType                = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		.pNext                = NULL,
		.renderPass           = VK_NULL_HANDLE,
		.subpass              = 0,
		.framebuffer          = VK_NULL_HANDLE,
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

	// transition the image to copy the image to the
	// transfer buffer
	vkk_util_imageMemoryBarrier(image, cb,
	                            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	                            0, image->mip_levels);

	// copy the image to the transfer buffer
	VkBufferImageCopy bic =
	{
		.bufferOffset      = 0,
		.bufferRowLength   = 0,
		.bufferImageHeight = 0,
		.imageSubresource  =
		{
			.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel       = 0,
			.baseArrayLayer = 0,
			.layerCount     = 1
		},
		.imageOffset =
		{
			.x = 0,
			.y = 0,
			.z = 0,
		},
		.imageExtent =
		{
			.width  = image->width,
			.height = image->height,
			.depth  = image->depth
		}
	};

	vkCmdCopyImageToBuffer(cb, image->image,
	                       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	                       xb->buffer, 1, &bic);

	// transition the image from transfer mode to shading mode
	vkk_util_imageMemoryBarrier(image, cb,
	                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	                            0, image->mip_levels);

	// end the transfer commands
	vkEndCommandBuffer(cb);

	// submit the commands
	if(vkk_engine_queueSubmit(engine, VKK_QUEUE_BACKGROUND, &cb,
	                          0, NULL, NULL, NULL,
	                          xi->fence) == 0)
	{
		goto fail_submit;
	}

	uint64_t timeout = UINT64_MAX;
	if(vkWaitForFences(engine->device, 1, &xi->fence, VK_TRUE,
	                   timeout) != VK_SUCCESS)
	{
		LOGW("vkWaitForFences failed");
		vkk_engine_queueWaitIdle(engine, VKK_QUEUE_BACKGROUND);
	}

	vkk_memoryManager_read(engine->mm, xb->memory,
	                       size, 0, pixels);

	vkk_xferManager_lock(self);
	if(cc_list_append(self->instance_list, NULL,
	                  (const void*) xi) == NULL)
	{
		vkk_xferInstance_delete(&xi);
	}

	if(cc_multimap_addp(self->buffer_map, (const void*) xb,
	                    sizeof(size_t), &size) == 0)
	{
		vkk_xferBuffer_delete(&xb);
	}
	vkk_xferManager_unlock(self);

	// success
	return 1;

	// failure
	fail_submit:
	fail_begin_cb:
	fail_cb:
		vkk_xferInstance_delete(&xi);
	fail_xi:
		vkk_xferBuffer_delete(&xb);
	return 0;
}

int vkk_xferManager_writeImage(vkk_xferManager_t* self,
                               vkk_image_t* image,
                               const void* pixels)
{
	ASSERT(self);
	ASSERT(image);
	ASSERT(pixels);

	vkk_engine_t* engine = self->engine;

	vkk_xferManager_lock(self);
	if(self->shutdown)
	{
		vkk_xferManager_unlock(self);
		return 0;
	}

	// F16 images are a special case because the source
	// pixels are in F32 format and must be converted by
	// performing vkCmdBlitImage since there is not a native
	// F16 type in C
	if((image->format == VKK_IMAGE_FORMAT_RGBAF16) ||
	   (image->format == VKK_IMAGE_FORMAT_RGBF16)  ||
	   (image->format == VKK_IMAGE_FORMAT_RGF16)   ||
	   (image->format == VKK_IMAGE_FORMAT_RF16))
	{
		vkk_xferManager_unlock(self);
		return vkk_xferManager_writeImageF16(self, image,
		                                     pixels);
	}

	uint32_t width;
	uint32_t height;
	uint32_t depth;
	size_t   size;
	size = vkk_image_size(image, &width, &height, &depth);

	vkk_xferBuffer_t*  xb;
	cc_multimapIter_t  miterator;
	cc_multimapIter_t* miter = &miterator;
	if(cc_multimap_findp(self->buffer_map, miter,
	                     sizeof(size_t), &size))
	{
		xb = (vkk_xferBuffer_t*)
		     cc_multimap_remove(self->buffer_map, &miter);
		vkk_memoryManager_write(engine->mm, xb->memory,
		                        size, 0, pixels);
	}
	else
	{
		xb = vkk_xferBuffer_new(engine, size, pixels);
		if(xb == NULL)
		{
			vkk_xferManager_unlock(self);
			return 0;
		}
	}

	vkk_xferInstance_t* xi;
	cc_listIter_t* iter = cc_list_head(self->instance_list);
	if(iter)
	{
		xi = (vkk_xferInstance_t*)
		     cc_list_remove(self->instance_list, &iter);
	}
	else
	{
		xi = vkk_xferInstance_new(engine);
		if(xi == NULL)
		{
			vkk_xferManager_unlock(self);
			goto fail_xi;
		}
	}
	vkk_xferManager_unlock(self);

	VkCommandBuffer cb;
	cb = vkk_commandBuffer_get(xi->cmd_buffer, 0);

	vkResetFences(engine->device, 1, &xi->fence);
	if(vkResetCommandBuffer(cb, 0) != VK_SUCCESS)
	{
		LOGE("vkResetCommandBuffer failed");
		goto fail_cb;
	}

	// begin the transfer commands
	VkCommandBufferInheritanceInfo cbi_info =
	{
		.sType                = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		.pNext                = NULL,
		.renderPass           = VK_NULL_HANDLE,
		.subpass              = 0,
		.framebuffer          = VK_NULL_HANDLE,
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

	// transition the image to copy the transfer buffer to
	// the image and generate mip levels if needed
	vkk_util_imageMemoryBarrier(image, cb,
	                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	                            0, image->mip_levels);

	// copy the transfer buffer to the image
	VkBufferImageCopy bic =
	{
		.bufferOffset      = 0,
		.bufferRowLength   = 0,
		.bufferImageHeight = 0,
		.imageSubresource  =
		{
			.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel       = 0,
			.baseArrayLayer = 0,
			.layerCount     = 1
		},
		.imageOffset =
		{
			.x = 0,
			.y = 0,
			.z = 0,
		},
		.imageExtent =
		{
			.width  = image->width,
			.height = image->height,
			.depth  = image->depth
		}
	};

	vkCmdCopyBufferToImage(cb, xb->buffer, image->image,
	                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	                       1, &bic);

	// at this point we may need to generate mip_levels if
	// mipmapping was enabled
	if(image->mip_levels > 1)
	{
		vkk_engine_mipmapImage(engine, image, cb);
	}

	// transition the image from transfer mode to shading mode
	vkk_util_imageMemoryBarrier(image, cb,
	                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	                            0, image->mip_levels);

	// end the transfer commands
	vkEndCommandBuffer(cb);

	// submit the commands
	if(vkk_engine_queueSubmit(engine, VKK_QUEUE_BACKGROUND, &cb,
	                          0, NULL, NULL, NULL,
	                          xi->fence) == 0)
	{
		goto fail_submit;
	}

	uint64_t timeout = UINT64_MAX;
	if(vkWaitForFences(engine->device, 1, &xi->fence, VK_TRUE,
	                   timeout) != VK_SUCCESS)
	{
		LOGW("vkWaitForFences failed");
		vkk_engine_queueWaitIdle(engine, VKK_QUEUE_BACKGROUND);
	}

	vkk_xferManager_lock(self);
	if(cc_list_append(self->instance_list, NULL,
	                  (const void*) xi) == NULL)
	{
		vkk_xferInstance_delete(&xi);
	}

	if(cc_multimap_addp(self->buffer_map, (const void*) xb,
	                    sizeof(size_t), &size) == 0)
	{
		vkk_xferBuffer_delete(&xb);
	}
	vkk_xferManager_unlock(self);

	// success
	return 1;

	// failure
	fail_submit:
	fail_begin_cb:
	fail_cb:
		vkk_xferInstance_delete(&xi);
	fail_xi:
		vkk_xferBuffer_delete(&xb);
	return 0;
}
