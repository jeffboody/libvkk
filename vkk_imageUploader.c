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
#include "../libcc/cc_log.h"
#include "../libcc/cc_memory.h"
#include "vkk_commandBuffer.h"
#include "vkk_engine.h"
#include "vkk_imageUploader.h"
#include "vkk_image.h"
#include "vkk_memoryChunk.h"
#include "vkk_memoryManager.h"
#include "vkk_memoryPool.h"
#include "vkk_memory.h"
#include "vkk_renderer.h"
#include "vkk_util.h"

typedef struct vkk_uploaderBuffer_s
{
	VkBuffer      buffer;
	vkk_memory_t* memory;
} vkk_uploaderBuffer_t;

typedef struct vkk_uploaderInstance_s
{
	VkFence              fence;
	vkk_commandBuffer_t* cmd_buffer;
} vkk_uploaderInstance_t;

/***********************************************************
* private                                                  *
***********************************************************/

static vkk_uploaderBuffer_t*
vkk_uploaderBuffer_new(vkk_engine_t* engine, size_t size,
                       const void* pixels)
{
	ASSERT(engine);
	ASSERT(pixels);

	vkk_uploaderBuffer_t* self;
	self = (vkk_uploaderBuffer_t*)
	       CALLOC(1, sizeof(vkk_uploaderBuffer_t));
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
		.usage                 = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
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
	                                             size, pixels);
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
vkk_uploaderBuffer_delete(vkk_uploaderBuffer_t** _self)
{
	ASSERT(_self);

	vkk_uploaderBuffer_t* self = *_self;
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

static vkk_uploaderInstance_t*
vkk_uploaderInstance_new(vkk_engine_t* engine)
{
	ASSERT(engine);

	vkk_uploaderInstance_t* self;
	self = (vkk_uploaderInstance_t*)
	       CALLOC(1, sizeof(vkk_uploaderInstance_t));
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
	self->cmd_buffer = vkk_commandBuffer_new(engine, 1,
	                                         VKK_RENDERER_TYPE_OFFSCREEN);
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
vkk_uploaderInstance_delete(vkk_uploaderInstance_t** _self)
{
	ASSERT(_self);

	vkk_uploaderInstance_t* self = *_self;
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
vkk_imageUploader_lock(vkk_imageUploader_t* self)
{
	ASSERT(self);

	pthread_mutex_lock(&self->mutex);
	TRACE_BEGIN();
}

static void
vkk_imageUploader_unlock(vkk_imageUploader_t* self)
{
	ASSERT(self);

	TRACE_END();
	pthread_mutex_unlock(&self->mutex);
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_imageUploader_t*
vkk_imageUploader_new(vkk_engine_t* engine)
{
	ASSERT(engine);

	vkk_imageUploader_t* self;
	self = (vkk_imageUploader_t*)
	       CALLOC(1, sizeof(vkk_imageUploader_t));
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

void vkk_imageUploader_delete(vkk_imageUploader_t** _self)
{
	ASSERT(_self);

	vkk_imageUploader_t* self = *_self;
	if(self)
	{
		cc_listIter_t* iter = cc_list_head(self->instance_list);
		while(iter)
		{
			vkk_uploaderInstance_t* ui;
			ui = (vkk_uploaderInstance_t*)
			     cc_list_remove(self->instance_list, &iter);
			vkk_uploaderInstance_delete(&ui);
		}

		cc_multimapIter_t  miterator;
		cc_multimapIter_t* miter;
		miter = cc_multimap_head(self->buffer_map, &miterator);
		while(miter)
		{
			vkk_uploaderBuffer_t* ub;
			ub = (vkk_uploaderBuffer_t*)
			     cc_multimap_remove(self->buffer_map, &miter);
			vkk_uploaderBuffer_delete(&ub);
		}

		pthread_mutex_destroy(&self->mutex);
		cc_multimap_delete(&self->buffer_map);
		cc_list_delete(&self->instance_list);
		FREE(self);
		*_self = NULL;
	}
}

void vkk_imageUploader_shutdown(vkk_imageUploader_t* self)
{
	ASSERT(self);

	vkk_imageUploader_lock(self);
	self->shutdown = 1;
	vkk_imageUploader_unlock(self);
}

int vkk_imageUploader_upload(vkk_imageUploader_t* self,
                             vkk_image_t* image,
                             const void* pixels)
{
	ASSERT(self);
	ASSERT(image);
	ASSERT(pixels);

	vkk_engine_t* engine = self->engine;

	vkk_imageUploader_lock(self);
	if(self->shutdown)
	{
		vkk_imageUploader_unlock(self);
		return 0;
	}

	uint32_t width;
	uint32_t height;
	uint32_t depth;
	size_t   size;
	size = vkk_image_size(image, &width, &height, &depth);

	vkk_uploaderBuffer_t* ub;
	cc_multimapIter_t     miterator;
	cc_multimapIter_t*    miter = &miterator;
	if(cc_multimap_findf(self->buffer_map, miter,
	                     "%u", (uint32_t) size))
	{
		ub = (vkk_uploaderBuffer_t*)
		     cc_multimap_remove(self->buffer_map, &miter);
		vkk_memoryManager_update(engine->mm, ub->memory,
		                         size, pixels);
	}
	else
	{
		ub = vkk_uploaderBuffer_new(engine, size, pixels);
		if(ub == NULL)
		{
			vkk_imageUploader_unlock(self);
			return 0;
		}
	}

	vkk_uploaderInstance_t* ui;
	cc_listIter_t* iter = cc_list_head(self->instance_list);
	if(iter)
	{
		ui = (vkk_uploaderInstance_t*)
		     cc_list_remove(self->instance_list, &iter);
	}
	else
	{
		ui = vkk_uploaderInstance_new(engine);
		if(ui == NULL)
		{
			vkk_imageUploader_unlock(self);
			goto fail_ui;
		}
	}
	vkk_imageUploader_unlock(self);

	VkCommandBuffer cb;
	cb = vkk_commandBuffer_get(ui->cmd_buffer, 0);

	vkResetFences(engine->device, 1, &ui->fence);
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

	vkCmdCopyBufferToImage(cb, ub->buffer, image->image,
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
	if(vkk_engine_queueSubmit(engine, VKK_QUEUE_OFFLINE, &cb,
	                          NULL, NULL, NULL,
	                          ui->fence) == 0)
	{
		goto fail_submit;
	}

	uint64_t timeout = UINT64_MAX;
	if(vkWaitForFences(engine->device, 1, &ui->fence, VK_TRUE,
	                   timeout) != VK_SUCCESS)
	{
		LOGW("vkWaitForFences failed");
		vkk_engine_queueWaitIdle(engine, VKK_QUEUE_OFFLINE);
	}

	vkk_imageUploader_lock(self);
	if(cc_list_append(self->instance_list, NULL,
	                  (const void*) ui) == NULL)
	{
		vkk_uploaderInstance_delete(&ui);
	}

	if(cc_multimap_addf(self->buffer_map, (const void*) ub,
	                    "%u", (uint32_t) size) == 0)
	{
		vkk_uploaderBuffer_delete(&ub);
	}
	vkk_imageUploader_unlock(self);

	// success
	return 1;

	// failure
	fail_submit:
	fail_begin_cb:
	fail_cb:
		vkk_uploaderInstance_delete(&ui);
	fail_ui:
		vkk_uploaderBuffer_delete(&ub);
	return 0;
}
