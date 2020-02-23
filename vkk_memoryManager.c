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
#include <pthread.h>

#define LOG_TAG "vkk"
#include "../libcc/cc_log.h"
#include "../libcc/cc_memory.h"
#include "vkk_engine.h"
#include "vkk_memory.h"
#include "vkk_memoryChunk.h"
#include "vkk_memoryManager.h"
#include "vkk_memoryPool.h"

/***********************************************************
* private                                                  *
***********************************************************/

static void
vkk_memoryManager_lock(vkk_memoryManager_t* self)
{
	ASSERT(self);

	pthread_mutex_lock(&self->manager_mutex);
	TRACE_BEGIN();
}

static void
vkk_memoryManager_unlock(vkk_memoryManager_t* self)
{
	ASSERT(self);

	TRACE_END();
	pthread_mutex_unlock(&self->manager_mutex);
}

static void
vkk_memoryManager_chunkLock(vkk_memoryManager_t* self,
                            vkk_memoryChunk_t* chunk)
{
	ASSERT(self);
	ASSERT(chunk);

	pthread_mutex_lock(&self->chunk_mutex);
	TRACE_BEGIN();

	while(chunk->locked)
	{
		TRACE_END();
		pthread_cond_wait(&self->chunk_cond,
		                  &self->chunk_mutex);
		TRACE_BEGIN();
	}

	chunk->locked = 1;
}

static void
vkk_memoryManager_chunkUnlock(vkk_memoryManager_t* self,
                              vkk_memoryChunk_t* chunk)
{
	ASSERT(self);
	ASSERT(chunk);

	chunk->locked = 0;

	TRACE_END();
	pthread_mutex_unlock(&self->chunk_mutex);
	pthread_cond_broadcast(&self->chunk_cond);
}

static void
vkk_memoryManager_freeLocked(vkk_memoryManager_t* self,
                             vkk_memory_t** _memory)
{
	ASSERT(self);
	ASSERT(_memory);

	vkk_memory_t* memory = *_memory;
	if(memory)
	{
		// free the memory and delete pool (if necessary);
		vkk_memoryChunk_t* chunk = memory->chunk;
		vkk_memoryPool_t*  pool  = chunk->pool;
		if(vkk_memoryPool_free(pool, self->shutdown, _memory))
		{
			cc_mapIter_t  miterator;
			cc_mapIter_t* miter = &miterator;

			vkk_memoryPool_t* tmp;
			tmp = (vkk_memoryPool_t*)
			      cc_map_findf(self->pools, miter, "%u/%u",
			                   (uint32_t) pool->mt_index,
			                   (uint32_t) pool->stride);
			assert(tmp == pool);

			cc_map_remove(self->pools, &miter);
			vkk_memoryPool_delete(&pool);
		}
	}
}

static size_t computePoolCount(size_t stride)
{
	ASSERT(stride > 0);

	// enforce allocation size
	size_t MB    = 1024*1024;
	size_t count = 256;
	size_t size  = stride*count;
	if(size >= 16*MB)
	{
		// allocate at most 16MB
		count = 16*MB/stride;
	}
	else if(size < 2*MB)
	{
		// allocate at least 2MB
		count = 2*MB/stride;
	}

	// enforce minimum count
	if(count == 0)
	{
		count = 1;
	}

	return count;
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_memoryManager_t*
vkk_memoryManager_new(vkk_engine_t* engine)
{
	ASSERT(engine);

	vkk_memoryManager_t* self;
	self = (vkk_memoryManager_t*)
	       CALLOC(1, sizeof(vkk_memoryManager_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->engine = engine;

	self->pools = cc_map_new();
	if(self->pools == NULL)
	{
		goto fail_pools;
	}

	if(pthread_mutex_init(&self->manager_mutex, NULL) != 0)
	{
		LOGE("pthread_mutex_init failed");
		goto fail_manager_mutex;
	}

	if(pthread_mutex_init(&self->chunk_mutex, NULL) != 0)
	{
		LOGE("pthread_mutex_init failed");
		goto fail_chunk_mutex;
	}

	if(pthread_cond_init(&self->chunk_cond, NULL) != 0)
	{
		LOGE("pthread_cond_init failed");
		goto fail_chunk_cond;
	}

	// success
	return self;

	// failure
	fail_chunk_cond:
		pthread_mutex_destroy(&self->chunk_mutex);
	fail_chunk_mutex:
		pthread_mutex_destroy(&self->manager_mutex);
	fail_manager_mutex:
		cc_map_delete(&self->pools);
	fail_pools:
		FREE(self);
	return NULL;
}

void vkk_memoryManager_delete(vkk_memoryManager_t** _self)
{
	ASSERT(_self);

	vkk_memoryManager_t* self = *_self;
	if(self)
	{
		ASSERT(cc_map_size(self->pools) == 0);

		pthread_cond_destroy(&self->chunk_cond);
		pthread_mutex_destroy(&self->chunk_mutex);
		pthread_mutex_destroy(&self->manager_mutex);
		cc_map_delete(&self->pools);
		FREE(self);
		*_self = NULL;
	}
}

void vkk_memoryManager_shutdown(vkk_memoryManager_t* self)
{
	ASSERT(self);

	vkk_memoryManager_lock(self);
	self->shutdown = 1;
	vkk_memoryManager_unlock(self);
}

vkk_memory_t*
vkk_memoryManager_allocBuffer(vkk_memoryManager_t* self,
                              VkBuffer buffer,
                              size_t size,
                              const void* buf)
{
	ASSERT(self);

	vkk_memoryManager_lock(self);

	vkk_engine_t* engine = self->engine;

	VkMemoryRequirements mr;
	vkGetBufferMemoryRequirements(engine->device,
	                              buffer, &mr);

	VkFlags mp_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
	                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	uint32_t mt_index;
	if(vkk_engine_getMemoryTypeIndex(engine,
	                                 mr.memoryTypeBits,
	                                 mp_flags,
	                                 &mt_index) == 0)
	{
		vkk_memoryManager_unlock(self);
		return NULL;
	}

	// compute the pool stride
	VkDeviceSize stride = mr.alignment;
	while(stride < mr.size)
	{
		stride *= 2;
	}

	// find an existing pool
	int pool_created = 0;
	cc_mapIter_t      miterator;
	cc_mapIter_t*     miter = &miterator;
	vkk_memoryPool_t* pool;
	pool = (vkk_memoryPool_t*)
	       cc_map_findf(self->pools, miter, "%u/%u",
	                    (uint32_t) mt_index,
	                    (uint32_t) stride);
	if(pool == NULL)
	{
		// otherwise create a new pool
		uint32_t count = computePoolCount((size_t) stride);
		pool = vkk_memoryPool_new(self, count, stride, mt_index);
		if(pool == NULL)
		{
			vkk_memoryManager_unlock(self);
			return NULL;
		}
		pool_created = 1;

		if(cc_map_addf(self->pools, (const void*) pool, "%u/%u",
		               (uint32_t) mt_index,
		               (uint32_t) stride) == 0)
		{
			goto fail_pool;
		}
	}

	// allocate memory
	vkk_memory_t* memory = vkk_memoryPool_alloc(pool);
	if(memory == NULL)
	{
		goto fail_alloc;
	}

	vkk_memoryChunk_t* chunk = memory->chunk;
	vkk_memoryManager_chunkLock(self, chunk);
	if(buf)
	{
		if(size > pool->stride)
		{
			LOGE("invalid size=%u, stride=%u",
			     (uint32_t) size, (uint32_t) pool->stride);
			goto fail_size;
		}

		void* data;
		if(vkMapMemory(engine->device, chunk->memory,
		               memory->offset, size, 0,
		               &data) == VK_SUCCESS)
		{
			memcpy(data, buf, size);
			vkUnmapMemory(engine->device, chunk->memory);
		}
		else
		{
			LOGE("vkMapMemory failed");
			goto fail_map;
		}
	}

	if(vkBindBufferMemory(engine->device, buffer,
	                      chunk->memory,
	                      memory->offset) != VK_SUCCESS)
	{
		LOGE("vkBindBufferMemory failed");
		goto fail_bind;
	}

	vkk_memoryManager_chunkUnlock(self, chunk);
	vkk_memoryManager_unlock(self);

	// success
	return memory;

	// failure
	fail_bind:
	fail_map:
	fail_size:
		vkk_memoryManager_chunkUnlock(self, chunk);
		vkk_memoryManager_freeLocked(self, &memory);
	fail_alloc:
	{
		if(pool_created)
		{
			cc_map_findf(self->pools, miter, "%u/%u",
			             (uint32_t) mt_index,
			             (uint32_t) stride);
			cc_map_remove(self->pools, &miter);
		}
	}
	fail_pool:
	{
		if(pool_created)
		{
			vkk_memoryPool_delete(&pool);
		}
		vkk_memoryManager_unlock(self);
	}
	return NULL;
}

vkk_memory_t*
vkk_memoryManager_allocImage(vkk_memoryManager_t* self,
                             VkImage image)
{
	ASSERT(self);

	vkk_memoryManager_lock(self);

	vkk_engine_t* engine = self->engine;

	VkMemoryRequirements mr;
	vkGetImageMemoryRequirements(engine->device,
	                             image, &mr);

	VkFlags mp_flags = 0;
	uint32_t mt_index;
	if(vkk_engine_getMemoryTypeIndex(engine,
	                                 mr.memoryTypeBits,
	                                 mp_flags,
	                                 &mt_index) == 0)
	{
		vkk_memoryManager_unlock(self);
		return NULL;
	}

	// compute the pool stride
	VkDeviceSize stride = mr.alignment;
	while(stride < mr.size)
	{
		stride *= 2;
	}

	// find an existing pool
	int pool_created = 0;
	cc_mapIter_t      miterator;
	cc_mapIter_t*     miter = &miterator;
	vkk_memoryPool_t* pool;
	pool = (vkk_memoryPool_t*)
	       cc_map_findf(self->pools, miter, "%u/%u",
	                    (uint32_t) mt_index,
	                    (uint32_t) stride);
	if(pool == NULL)
	{
		// otherwise create a new pool
		uint32_t count = computePoolCount((size_t) stride);
		pool = vkk_memoryPool_new(self, count, stride, mt_index);
		if(pool == NULL)
		{
			vkk_memoryManager_unlock(self);
			return NULL;
		}
		pool_created = 1;

		if(cc_map_addf(self->pools, (const void*) pool, "%u/%u",
		               (uint32_t) mt_index,
		               (uint32_t) stride) == 0)
		{
			goto fail_pool;
		}
	}

	// allocate memory
	vkk_memory_t* memory = vkk_memoryPool_alloc(pool);
	if(memory == NULL)
	{
		goto fail_alloc;
	}

	vkk_memoryChunk_t* chunk = memory->chunk;
	if(vkBindImageMemory(engine->device, image,
	                     chunk->memory,
	                     memory->offset) != VK_SUCCESS)
	{
		LOGE("vkBindBufferMemory failed");
		goto fail_bind;
	}

	vkk_memoryManager_unlock(self);

	// success
	return memory;

	// failure
	fail_bind:
		vkk_memoryManager_freeLocked(self, &memory);
	fail_alloc:
	{
		if(pool_created)
		{
			cc_map_findf(self->pools, miter, "%u/%u",
			             (uint32_t) mt_index,
			             (uint32_t) stride);
			cc_map_remove(self->pools, &miter);
		}
	}
	fail_pool:
	{
		if(pool_created)
		{
			vkk_memoryPool_delete(&pool);
		}
		vkk_memoryManager_unlock(self);
	}
	return NULL;
}

void vkk_memoryManager_free(vkk_memoryManager_t* self,
                            vkk_memory_t** _memory)
{
	ASSERT(self);
	ASSERT(_memory);

	vkk_memoryManager_lock(self);
	vkk_memoryManager_freeLocked(self, _memory);
	vkk_memoryManager_unlock(self);
}

void vkk_memoryManager_update(vkk_memoryManager_t* self,
                              vkk_memory_t* memory,
                              size_t size,
                              const void* buf)
{
	ASSERT(self);
	ASSERT(memory);
	ASSERT(size > 0);
	ASSERT(buf);

	vkk_memoryChunk_t*   chunk  = memory->chunk;
	vkk_memoryPool_t*    pool   = chunk->pool;
	vkk_memoryManager_t* mm     = pool->mm;
	vkk_engine_t*        engine = mm->engine;

	vkk_memoryManager_chunkLock(self, chunk);

	if(size > pool->stride)
	{
		LOGE("invalid size=%u, stride=%u",
		     (uint32_t) size, (uint32_t) pool->stride);
		vkk_memoryManager_chunkUnlock(self, chunk);
		return;
	}

	void* data;
	if(vkMapMemory(engine->device, chunk->memory,
	               memory->offset, size, 0,
	               &data) == VK_SUCCESS)
	{
		memcpy(data, buf, size);
		vkUnmapMemory(engine->device, chunk->memory);
	}
	else
	{
		LOGW("vkMapMemory failed");
	}

	vkk_memoryManager_chunkUnlock(self, chunk);
}

void vkk_memoryManager_meminfo(vkk_memoryManager_t* self,
                               size_t* _count_chunks,
                               size_t* _count_slots,
                               size_t* _size_chunks,
                               size_t* _size_slots)
{
	ASSERT(self);
	ASSERT(_count_chunks);
	ASSERT(_count_slots);
	ASSERT(_size_chunks);
	ASSERT(_size_slots);

	vkk_memoryManager_lock(self);
	*_count_chunks = self->count_chunks;
	*_count_slots  = self->count_slots;
	*_size_chunks  = self->size_chunks;
	*_size_slots   = self->size_slots;

	// optionally dump debug information
	#if 0
		LOGI("MEMINFO: count_chunks=%i, count_slots=%i, size_chunks=%i, size_slots=%i",
		     (int) self->count_chunks, (int) self->count_slots,
		     (int) self->size_chunks, (int) self->size_slots);

		cc_mapIter_t  miterator;
		cc_mapIter_t* miter;
		miter = cc_map_head(self->pools, &miterator);
		while(miter)
		{
			vkk_memoryPool_t* pool;
			pool = (vkk_memoryPool_t*) cc_map_val(miter);
			vkk_memoryPool_meminfo(pool);
			miter = cc_map_next(miter);
		}
	#endif
	vkk_memoryManager_unlock(self);
}
