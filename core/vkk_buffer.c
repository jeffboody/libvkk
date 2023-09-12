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
#include "vkk_buffer.h"
#include "vkk_engine.h"
#include "vkk_memoryManager.h"

/***********************************************************
* public                                                   *
***********************************************************/

vkk_buffer_t*
vkk_buffer_new(vkk_engine_t* engine,
               vkk_updateMode_e update,
               vkk_bufferUsage_e usage, size_t size,
               const void* buf)
{
	// buf may be NULL
	ASSERT(engine);

	uint32_t count = 1;
	if(update == VKK_UPDATE_MODE_ASYNCHRONOUS)
	{
		if(usage == VKK_BUFFER_USAGE_STORAGE)
		{
			LOGE("invalid");
			return NULL;
		}

		count = vkk_engine_imageCount(engine);
	}

	int local_memory = 0;
	#ifndef ANDROID
	if(usage == VKK_BUFFER_USAGE_STORAGE)
	{
		local_memory = 1;
	}
	#endif

	VkBufferUsageFlags usage_map[VKK_BUFFER_USAGE_COUNT] =
	{
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	};

	vkk_buffer_t* self;
	self = (vkk_buffer_t*) CALLOC(1, sizeof(vkk_buffer_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->engine = engine;
	self->update = update;
	self->usage  = usage;
	self->size   = size;

	self->buffer = (VkBuffer*)
	               CALLOC(count, sizeof(VkBuffer));
	if(self->buffer == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_buffer;
	}

	self->memory = (vkk_memory_t**)
	               CALLOC(count, sizeof(vkk_memory_t*));
	if(self->memory == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_memory;
	}

	VkBufferCreateInfo b_info =
	{
		.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext                 = NULL,
		.flags                 = 0,
		.size                  = size,
		.usage                 = usage_map[usage],
		.sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices   = &engine->queue_family_index
	};

	#ifndef ANDROID
	if(usage == VKK_BUFFER_USAGE_STORAGE)
	{
		b_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		               VK_BUFFER_USAGE_TRANSFER_DST_BIT   |
		               VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	}
	#endif

	int i;
	for(i = 0; i < count; ++i)
	{
		if(vkCreateBuffer(engine->device, &b_info, NULL,
		                  &self->buffer[i]) != VK_SUCCESS)
		{
			LOGE("vkCreateBuffer failed");
			goto fail_create_buffer;
		}

		#ifndef ANDROID
		if(usage == VKK_BUFFER_USAGE_STORAGE)
		{
			self->memory[i] = vkk_memoryManager_allocBuffer(engine->mm,
			                                                self->buffer[i],
			                                                local_memory,
			                                                size,
			                                                NULL);
			if(self->memory[i] == NULL)
			{
				goto fail_alloc;
			}

			// optionally blit buf
			if(buf)
			{
				// cast away const since blitStorage is read/write
				if(vkk_xferManager_blitStorage(engine->xfer,
				                               VKK_XFER_MODE_WRITE,
				                               self, size, 0,
				                               (void*) buf) == 0)
				{
					goto fail_alloc;
				}
			}
		}
		else
		#endif
		{
			self->memory[i] = vkk_memoryManager_allocBuffer(engine->mm,
			                                                self->buffer[i],
			                                                local_memory,
			                                                size,
			                                                buf);
			if(self->memory[i] == NULL)
			{
				goto fail_alloc;
			}
		}
	}

	// success
	return self;

	// failure
	fail_alloc:
	fail_create_buffer:
	{
		int j;
		for(j = 0; j <= i; ++j)
		{
			vkk_memoryManager_free(engine->mm, &self->memory[j]);
			vkDestroyBuffer(engine->device,
			                self->buffer[j],
			                NULL);
		}
		FREE(self->memory);
	}
	fail_memory:
		FREE(self->buffer);
	fail_buffer:
		FREE(self);
	return NULL;
}

void vkk_buffer_delete(vkk_buffer_t** _self)
{
	ASSERT(_self);

	vkk_buffer_t* self = *_self;
	if(self)
	{
		vkk_engine_deleteObject(self->engine,
		                        VKK_OBJECT_TYPE_BUFFER,
		                        (void*) self);
		*_self = NULL;
	}
}

size_t vkk_buffer_size(vkk_buffer_t* self)
{
	ASSERT(self);

	return self->size;
}
