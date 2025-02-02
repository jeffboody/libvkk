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

#define LOG_TAG "vkk"
#include "../../libcc/cc_log.h"
#include "../../libcc/cc_memory.h"
#include "vkk_memory.h"
#include "vkk_memoryChunk.h"
#include "vkk_memoryManager.h"
#include "vkk_memoryPool.h"

/***********************************************************
* public                                                   *
***********************************************************/

vkk_memoryPool_t*
vkk_memoryPool_new(vkk_memoryManager_t* mm,
                   uint32_t count,
                   VkDeviceSize stride,
                   uint32_t mt_index,
                   vkk_memoryType_e type)
{
	ASSERT(mm);
	ASSERT(type != VKK_MEMORY_TYPE_ANY);

	vkk_memoryPool_t* self;
	self = (vkk_memoryPool_t*)
	       CALLOC(1, sizeof(vkk_memoryPool_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->mm       = mm;
	self->count    = count;
	self->stride   = stride;
	self->mt_index = mt_index;
	self->type     = type;

	self->chunks = cc_list_new();
	if(self->chunks == NULL)
	{
		goto fail_chunks;
	}

	// success
	return self;

	// failure
	fail_chunks:
		FREE(self);
	return NULL;
}

void vkk_memoryPool_delete(vkk_memoryPool_t** _self)
{
	ASSERT(_self);

	vkk_memoryPool_t* self = *_self;
	if(self)
	{
		ASSERT(cc_list_size(self->chunks) == 0);

		cc_list_delete(&self->chunks);
		FREE(self);
		*_self = NULL;
	}
}

vkk_memory_t*
vkk_memoryPool_alloc(vkk_memoryPool_t* self,
                     vkk_memoryInfo_t* info)
{
	ASSERT(self);
	ASSERT(info);

	// try to allocate from an existing slot
	vkk_memoryChunk_t* chunk;
	cc_listIter_t*     iter = cc_list_head(self->chunks);
	while(iter)
	{
		chunk = (vkk_memoryChunk_t*)
		        cc_list_peekIter(iter);

		if(vkk_memoryChunk_slots(chunk))
		{
			return vkk_memoryChunk_alloc(chunk, info);
		}

		iter = cc_list_next(iter);
	}

	// create a new chunk
	chunk = vkk_memoryChunk_new(self, info);
	if(chunk == NULL)
	{
		return NULL;
	}

	cc_listIter_t* chunk_iter;
	chunk_iter = cc_list_append(self->chunks, NULL,
	                            (const void*) chunk);
	if(chunk_iter == NULL)
	{
		goto fail_chunk_iter;
	}

	// info invalid if alloc fails
	vkk_memory_t* memory;
	memory = vkk_memoryChunk_alloc(chunk, info);
	if(memory == NULL)
	{
		goto fail_memory;
	}

	// success
	return memory;

	// failure
	fail_memory:
		cc_list_remove(self->chunks, &chunk_iter);
	fail_chunk_iter:
		vkk_memoryChunk_delete(&chunk, info);
	return NULL;
}

int vkk_memoryPool_free(vkk_memoryPool_t* self,
                        int shutdown,
                        vkk_memory_t** _memory,
                        vkk_memoryChunk_t** _chunk,
                        vkk_memoryInfo_t* info)
{
	ASSERT(self);
	ASSERT(_memory);
	ASSERT(_chunk);
	ASSERT(info);

	vkk_memory_t* memory = *_memory;
	if(memory)
	{
		// free memory and delete chunk (if needed)
		vkk_memoryChunk_t* chunk = memory->chunk;
		if(vkk_memoryChunk_free(chunk, shutdown, _memory, info))
		{
			cc_listIter_t* iter = cc_list_head(self->chunks);
			while(iter)
			{
				vkk_memoryChunk_t* tmp;
				tmp = (vkk_memoryChunk_t*)
				      cc_list_peekIter(iter);
				if(tmp == chunk)
				{
					cc_list_remove(self->chunks, &iter);
					break;
				}

				iter = cc_list_next(iter);
			}
			*_chunk = chunk;
		}
	}

	return (cc_list_size(self->chunks) == 0) ? 1 : 0;
}

void vkk_memoryPool_memoryInfo(vkk_memoryPool_t* self,
                               vkk_memoryType_e type)
{
	ASSERT(self);

	if((type == VKK_MEMORY_TYPE_ANY) ||
	   (type == self->type))
	{
		// continue
	}
	else
	{
		// ignore
		return;
	}

	const char* type_name[VKK_MEMORY_TYPE_COUNT] =
	{
		"system",
		"device",
		"transient",
	};

	uint32_t chunk_count = cc_list_size(self->chunks);
	size_t   chunk_size  = self->count*self->stride*chunk_count;
	LOGI("POOL: type=%s, count=%u, stride=%u, chunk_count=%i, chunk_size=%" PRIu64,
	     type_name[self->type], (uint32_t) self->count,
	     (uint32_t) self->stride, chunk_count,
	     (uint64_t) chunk_size);

	cc_listIter_t* iter = cc_list_head(self->chunks);
	while(iter)
	{
		vkk_memoryChunk_t* chunk;
		chunk = (vkk_memoryChunk_t*) cc_list_peekIter(iter);
		vkk_memoryChunk_memoryInfo(chunk);
		iter = cc_list_next(iter);
	}
}
