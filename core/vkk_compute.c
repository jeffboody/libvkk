/*
 * Copyright (c) 2023 Jeff Boody
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
#include "vkk_buffer.h"
#include "vkk_computePipeline.h"
#include "vkk_compute.h"
#include "vkk_engine.h"
#include "vkk_memoryManager.h"
#include "vkk_pipelineLayout.h"
#include "vkk_uniformSetFactory.h"
#include "vkk_uniformSet.h"
#include "vkk_util.h"

/***********************************************************
* private                                                  *
***********************************************************/

static void
vkk_compute_updateUniformBufferRef(vkk_compute_t* self,
                                   vkk_uniformSet_t* us,
                                   vkk_uniformAttachment_t* ua)
{
	ASSERT(self);
	ASSERT(us);
	ASSERT(ua);

	vkk_engine_t* engine = self->engine;

	VkDescriptorType dt_map[VKK_UNIFORM_TYPE_COUNT] =
	{
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	};

	VkDescriptorBufferInfo db_info =
	{
		.buffer  = ua->buffer->buffer[0],
		.offset  = 0,
		.range   = ua->buffer->size
	};

	VkWriteDescriptorSet writes =
	{
		.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext            = NULL,
		.dstSet           = us->ds_array[0],
		.dstBinding       = ua->binding,
		.dstArrayElement  = 0,
		.descriptorCount  = 1,
		.descriptorType   = dt_map[ua->type],
		.pImageInfo       = NULL,
		.pBufferInfo      = &db_info,
		.pTexelBufferView = NULL,
	};

	vkUpdateDescriptorSets(engine->device, 1, &writes,
	                       0, NULL);
}

// debug functions only used by ASSERT
#ifdef ASSERT_DEBUG

static int
vkk_compute_checkUpdate(vkk_buffer_t* buffer)
{
	ASSERT(buffer);

	if((buffer->usage == VKK_BUFFER_USAGE_UNIFORM) ||
	   (buffer->usage == VKK_BUFFER_USAGE_STORAGE))
	{
		// ok
	}
	else
	{
		LOGW("invalid usage=%i", (int) buffer->usage);
		return 0;
	}

	// update must be SYNCHRONOUS
	if(buffer->update != VKK_UPDATE_MODE_SYNCHRONOUS)
	{
		LOGW("invalid update=%i", buffer->update);
		return 0;
	}

	return 1;
}

#endif

static uint32_t uceil(uint32_t count, uint32_t local_size)
{
	ASSERT(count > 0);
	ASSERT(local_size > 0);

	return (count + local_size - 1)/local_size;
}

/***********************************************************
* protected                                                *
***********************************************************/

void vkk_compute_destruct(vkk_compute_t** _self)
{
	ASSERT(_self);

	vkk_compute_t* self = *_self;
	if(self)
	{
		vkk_engine_t* engine = self->engine;

		vkDestroyFence(engine->device, self->fence, NULL);
		vkk_commandBuffer_delete(&self->cmd_buffer);
		FREE(self);
		*_self = NULL;
	}
}

VkCommandBuffer
vkk_compute_commandBuffer(vkk_compute_t* self)
{
	ASSERT(self);

	return vkk_commandBuffer_get(self->cmd_buffer, 0);
}

double vkk_compute_tsCurrent(vkk_compute_t* self)
{
	ASSERT(self);

	return 0.0;
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_compute_t* vkk_compute_new(vkk_engine_t* engine)
{
	ASSERT(engine);

	vkk_compute_t* self;
	self = (vkk_compute_t*)
	       CALLOC(1, sizeof(vkk_compute_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->engine = engine;

	self->cmd_buffer = vkk_commandBuffer_new(engine, 1, 0);
	if(self->cmd_buffer == NULL)
	{
		goto fail_cmd_buffer;
	}

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

	// success
	return self;

	// failure
	fail_create_fence:
		vkk_commandBuffer_delete(&self->cmd_buffer);
	fail_cmd_buffer:
		FREE(self);
	return NULL;
}

void vkk_compute_delete(vkk_compute_t** _self)
{
	ASSERT(_self);

	vkk_compute_t* self = *_self;
	if(self)
	{
		vkk_engine_t* engine = self->engine;

		// engine calls vkk_compute_destruct
		vkk_engine_deleteObject(engine, VKK_OBJECT_TYPE_COMPUTE,
		                        (void*) self);
		*_self = NULL;
	}
}

int vkk_compute_begin(vkk_compute_t* self)
{
	ASSERT(self);

	vkk_engine_t* engine = self->engine;

	vkk_engine_rendererLock(engine);
	if(engine->shutdown)
	{
		LOGE("invalid");
		vkk_engine_rendererUnlock(engine);
		return 0;
	}
	vkk_engine_rendererUnlock(engine);

	VkCommandBuffer cb;
	cb = vkk_commandBuffer_get(self->cmd_buffer, 0);
	if(vkResetCommandBuffer(cb, 0) != VK_SUCCESS)
	{
		LOGE("vkResetCommandBuffer failed");
		return 0;
	}

	VkCommandBufferBeginInfo cb_info =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	};

	if(vkBeginCommandBuffer(cb, &cb_info) != VK_SUCCESS)
	{
		LOGE("vkBeginCommandBuffer failed");
		return 0;
	}

	return 1;
}

void vkk_compute_end(vkk_compute_t* self)
{
	ASSERT(self);

	vkk_engine_t* engine = self->engine;

	VkCommandBuffer cb;
	cb = vkk_commandBuffer_get(self->cmd_buffer, 0);
	vkEndCommandBuffer(cb);

	vkResetFences(engine->device, 1, &self->fence);
	if(vkk_engine_queueSubmit(engine, VKK_QUEUE_BACKGROUND,
	                          &cb, 0, NULL, NULL, NULL,
	                          self->fence) == 0)
	{
		LOGW("vkk_engine_queueSubmit failed");
		return;
	}

	uint64_t timeout = UINT64_MAX;
	if(vkWaitForFences(engine->device, 1, &self->fence, VK_TRUE,
	                   timeout) != VK_SUCCESS)
	{
		LOGW("vkWaitForFences failed");
		vkk_engine_queueWaitIdle(engine, VKK_QUEUE_BACKGROUND);
	}

	self->cp = NULL;
}

vkk_updateMode_e
vkk_compute_updateMode(vkk_compute_t* self)
{
	ASSERT(self);

	return VKK_UPDATE_MODE_SYNCHRONOUS;
}

int
vkk_compute_writeBuffer(vkk_compute_t* self,
                        vkk_buffer_t* buffer,
                        size_t size,
                        size_t offset,
                        const void* data)
{
	ASSERT(self);
	ASSERT(vkk_compute_checkUpdate(buffer));
	ASSERT(data);

	vkk_engine_t* engine = self->engine;

	#ifndef ANDROID
	if(buffer->usage == VKK_BUFFER_USAGE_STORAGE)
	{
		// cast away const since blitStorage is read/write
		return vkk_xferManager_blitStorage(engine->xfer,
		                                   VKK_XFER_MODE_WRITE,
		                                   buffer, size, offset,
		                                   (void*) data);
	}
	else
	#endif
	{
		uint32_t idx = 0;
		vkk_memoryManager_write(engine->mm, buffer->memory[idx],
		                        size, offset, data);
		return 1;
	}
}

int
vkk_compute_readBuffer(vkk_compute_t* self,
                       vkk_buffer_t* buffer,
                       size_t size, size_t offset,
                       void* data)
{
	ASSERT(self);
	ASSERT(vkk_compute_checkUpdate(buffer));
	ASSERT(data);

	vkk_engine_t* engine = self->engine;

	#ifndef ANDROID
	if(buffer->usage == VKK_BUFFER_USAGE_STORAGE)
	{
		return vkk_xferManager_blitStorage(engine->xfer,
		                                   VKK_XFER_MODE_READ,
		                                   buffer, size, offset,
		                                   data);
	}
	else
	#endif
	{
		uint32_t idx = 0;
		vkk_memoryManager_read(engine->mm, buffer->memory[idx],
		                       size, offset, data);
		return 1;
	}
}

void
vkk_compute_updateUniformSetRefs(vkk_compute_t* self,
                                 vkk_uniformSet_t* us,
                                 uint32_t ua_count,
                                 vkk_uniformAttachment_t* ua_array)
{
	ASSERT(self);
	ASSERT(us);
	ASSERT(ua_array);

	vkk_uniformSetFactory_t* usf = us->usf;

	vkk_util_fillUniformAttachmentArray(us->ua_array,
	                                    ua_count, ua_array,
	                                    usf);

	// attach buffers
	uint32_t i;
	for(i = 0; i < ua_count; ++i)
	{
		if((ua_array[i].type == VKK_UNIFORM_TYPE_BUFFER_REF) ||
		   (ua_array[i].type == VKK_UNIFORM_TYPE_STORAGE_REF))
		{
			vkk_compute_updateUniformBufferRef(self, us,
			                                   &ua_array[i]);
		}
	}
}

void
vkk_compute_bindComputePipeline(vkk_compute_t* self,
                                vkk_computePipeline_t* cp)
{
	ASSERT(self);
	ASSERT(self == cp->compute);

	VkCommandBuffer cb = vkk_compute_commandBuffer(self);

	vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE,
	                  cp->pipeline);
	self->cp = cp;

	// update timestamp
	cp->ts = vkk_compute_tsCurrent(self);
}

void vkk_compute_bindUniformSets(vkk_compute_t* self,
                                 uint32_t us_count,
                                 vkk_uniformSet_t** us_array)
{
	ASSERT(self);
	ASSERT(self->cp);
	ASSERT(us_array);

	double ts = vkk_compute_tsCurrent(self);

	if(us_count > VKK_ENGINE_MAX_USF_COUNT)
	{
		LOGE("invalid us_count=%u", us_count);
		return;
	}

	// fill descriptor set array
	int             i;
	uint32_t        idx = 0;
	VkDescriptorSet ds[VKK_ENGINE_MAX_USF_COUNT];
	for(i = 0; i < us_count; ++i)
	{
		ds[i] = us_array[i]->ds_array[idx];

		// update timestamps
		int j;
		for(j = 0; j < us_array[i]->ua_count; ++j)
		{
			vkk_uniformAttachment_t* ua;
			ua = &(us_array[i]->ua_array[j]);

			if(ts != 0.0)
			{
				if((ua->type == VKK_UNIFORM_TYPE_BUFFER)     ||
				   (ua->type == VKK_UNIFORM_TYPE_STORAGE)    ||
				   (ua->type == VKK_UNIFORM_TYPE_BUFFER_REF) ||
				   (ua->type == VKK_UNIFORM_TYPE_STORAGE_REF))
				{
					ua->buffer->ts = ts;
				}
			}

			us_array[i]->ts = ts;
		}
	}

	uint32_t first = us_array[0]->set;

	VkCommandBuffer cb = vkk_compute_commandBuffer(self);
	vkCmdBindDescriptorSets(cb,
	                        VK_PIPELINE_BIND_POINT_COMPUTE,
	                        self->cp->pl->pl, first, us_count, ds,
	                        0, NULL);
}

void vkk_compute_dispatch(vkk_compute_t* self,
                          vkk_hazzard_e hazzard,
                          uint32_t count_x,
                          uint32_t count_y,
                          uint32_t count_z,
                          uint32_t local_size_x,
                          uint32_t local_size_y,
                          uint32_t local_size_z)
{
	ASSERT(self);

	// See Compute to Compute Dependencies
	// https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples

	VkCommandBuffer      cb    = vkk_compute_commandBuffer(self);
	VkPipelineStageFlags stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

	if((hazzard == VKK_HAZZARD_ANY) ||
	   (hazzard == VKK_HAZZARD_RAW))
	{
		VkMemoryBarrier mb =
		{
			.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
			.pNext         = NULL,
			.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		};

		vkCmdPipelineBarrier(cb, stage, stage, 0,
		                     1, &mb, 0, NULL, 0, NULL);
	}
	else if(hazzard == VKK_HAZZARD_WAR)
	{
		vkCmdPipelineBarrier(cb, stage, stage, 0,
		                     0, NULL, 0, NULL, 0, NULL);
	}

	uint32_t groupCountX = uceil(count_x, local_size_x);
	uint32_t groupCountY = uceil(count_y, local_size_y);
	uint32_t groupCountZ = uceil(count_z, local_size_z);
	vkCmdDispatch(cb, groupCountX, groupCountY, groupCountZ);
}
