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

#define LOG_TAG "vkk"
#include "../libcc/cc_log.h"
#include "../libcc/cc_memory.h"
#include "vkk_engine.h"
#include "vkk_memory.h"
#include "vkk_memoryChunk.h"
#include "vkk_memoryManager.h"
#include "vkk_memoryPool.h"

/***********************************************************
* public                                                   *
***********************************************************/

vkk_memoryChunk_t*
vkk_memoryChunk_new(vkk_memoryPool_t* pool)
{
	assert(pool);

	vkk_engine_t* engine = pool->mm->engine;

	vkk_memoryChunk_t* self;
	self = (vkk_memoryChunk_t*)
	       CALLOC(1, sizeof(vkk_memoryChunk_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->pool = pool;

	VkMemoryAllocateInfo ma_info =
	{
		.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext           = NULL,
		.allocationSize  = pool->stride*pool->count,
		.memoryTypeIndex = pool->mt_index
	};

	if(vkAllocateMemory(engine->device, &ma_info, NULL,
	                    &self->memory) != VK_SUCCESS)
	{
		LOGE("vkAllocateMemory failed");
		goto fail_allocate;
	}

	self->slots = cc_list_new();
	if(self->slots == NULL)
	{
		goto fail_slots;
	}

	// success
	return self;

	// failure
	fail_slots:
		vkFreeMemory(engine->device, self->memory, NULL);
	fail_allocate:
		FREE(self);
	return NULL;
}

void vkk_memoryChunk_delete(vkk_memoryChunk_t** _self)
{
	assert(_self);

	vkk_memoryChunk_t* self = *_self;
	if(self)
	{
		assert(self->usecount == 0);

		vkk_memoryPool_t* pool   = self->pool;
		vkk_engine_t*     engine = pool->mm->engine;

		cc_listIter_t* iter;
		iter = cc_list_head(self->slots);
		while(iter)
		{
			vkk_memory_t* memory;
			memory = (vkk_memory_t*)
			         cc_list_remove(self->slots, &iter);
			vkk_memory_delete(&memory);
		}

		vkFreeMemory(engine->device, self->memory, NULL);
		FREE(self);
		*_self = NULL;
	}
}

int vkk_memoryChunk_slots(vkk_memoryChunk_t* self)
{
	assert(self);

	vkk_memoryPool_t* pool = self->pool;

	// return slots available
	return cc_list_size(self->slots) + (pool->count - self->slot);
}

vkk_memory_t*
vkk_memoryChunk_alloc(vkk_memoryChunk_t* self)
{
	assert(self);

	vkk_memoryPool_t* pool = self->pool;

	// try to reuse an existing slot
	vkk_memory_t* memory;
	cc_listIter_t* iter = cc_list_head(self->slots);
	if(iter)
	{
		memory = (vkk_memory_t*)
		         cc_list_remove(self->slots, &iter);
		++self->usecount;
		return memory;
	}

	// check if the chunk is full
	if(self->slot == pool->count)
	{
		return NULL;
	}

	memory = vkk_memory_new(self, self->slot*pool->stride);
	if(memory == NULL)
	{
		return NULL;
	}

	++self->slot;
	++self->usecount;

	return memory;
}

int vkk_memoryChunk_free(vkk_memoryChunk_t* self,
                         int shutdown,
                         vkk_memory_t** _memory)
{
	assert(self);
	assert(_memory);

	vkk_memory_t* memory = *_memory;
	if(memory)
	{
		vkk_memoryPool_t* pool = self->pool;

		--self->usecount;

		if(shutdown)
		{
			vkk_memory_delete(&memory);
		}
		else if(cc_list_append(self->slots, NULL,
		                       (const void*) memory) == NULL)
		{
			LOGW("slot lost %u bytes", (uint32_t) pool->stride);
			vkk_memory_delete(&memory);
		}
		*_memory = NULL;
	}

	return (self->usecount == 0) ? 1 : 0;
}