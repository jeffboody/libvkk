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
#include "vkk_commandBuffer.h"
#include "vkk_engine.h"
#include "vkk_renderer.h"

/***********************************************************
* public                                                   *
***********************************************************/

vkk_commandBuffer_t*
vkk_commandBuffer_new(vkk_engine_t* engine,
                      uint32_t cb_count,
                      int secondary)
{
	ASSERT(engine);

	vkk_commandBuffer_t* self;
	self = (vkk_commandBuffer_t*)
	       CALLOC(1, sizeof(vkk_commandBuffer_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->engine   = engine;
	self->cb_count = cb_count;

	self->cb_array = (VkCommandBuffer*)
	                 CALLOC(cb_count,
	                        sizeof(VkCommandBuffer));
	if(self->cb_array == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_cb_array;
	}

	VkCommandPoolCreateInfo cpc_info =
	{
		.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext            = NULL,
		.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = engine->queue_family_index
	};

	if(vkCreateCommandPool(engine->device, &cpc_info, NULL,
	                       &self->command_pool) != VK_SUCCESS)
	{
		LOGE("vkCreateCommandPool failed");
		goto fail_command_pool;
	}

	VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	if(secondary)
	{
		level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	}

	VkCommandBufferAllocateInfo cba_info =
	{
		.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext              = NULL,
		.commandPool        = self->command_pool,
		.level              = level,
		.commandBufferCount = cb_count
	};

	if(vkAllocateCommandBuffers(engine->device, &cba_info,
	                            self->cb_array) != VK_SUCCESS)
	{
		LOGE("vkAllocateCommandBuffers failed");
		goto fail_allocate;
	}

	// success
	return self;

	// failure
	fail_allocate:
		vkDestroyCommandPool(engine->device,
		                     self->command_pool, NULL);
	fail_command_pool:
		FREE(self->cb_array);
	fail_cb_array:
		FREE(self);
	return NULL;
}

void vkk_commandBuffer_delete(vkk_commandBuffer_t** _self)
{
	ASSERT(_self);

	vkk_commandBuffer_t* self = *_self;
	if(self)
	{
		vkk_engine_t* engine = self->engine;

		vkFreeCommandBuffers(engine->device,
		                     self->command_pool,
		                     self->cb_count,
		                     self->cb_array);
		vkDestroyCommandPool(engine->device,
		                     self->command_pool, NULL);
		FREE(self->cb_array);
		FREE(self);
		*_self = NULL;
	}
}

VkCommandBuffer
vkk_commandBuffer_get(vkk_commandBuffer_t* self,
                      uint32_t index)
{
	ASSERT(self);
	ASSERT(index < self->cb_count);

	return self->cb_array[index];
}
