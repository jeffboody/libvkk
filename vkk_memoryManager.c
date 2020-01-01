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
	assert(self);

	pthread_mutex_lock(&self->manager_mutex);
	TRACE_BEGIN();
}

static void
vkk_memoryManager_unlock(vkk_memoryManager_t* self)
{
	assert(self);

	TRACE_END();
	pthread_mutex_unlock(&self->manager_mutex);
}

static void
vkk_memoryManager_chunkLock(vkk_memoryManager_t* self,
                            vkk_memoryChunk_t* chunk)
{
	assert(self);
	assert(chunk);

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
	assert(self);
	assert(chunk);

	chunk->locked = 0;

	TRACE_END();
	pthread_mutex_unlock(&self->chunk_mutex);
	pthread_cond_broadcast(&self->chunk_cond);
}

static void
vkk_memoryManager_freeLocked(vkk_memoryManager_t* self,
                             vkk_memory_t** _memory)
{
	assert(self);
	assert(_memory);

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

/***********************************************************
* public                                                   *
***********************************************************/

vkk_memoryManager_t*
vkk_memoryManager_new(vkk_engine_t* engine)
{
	assert(engine);

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
	assert(_self);

	vkk_memoryManager_t* self = *_self;
	if(self)
	{
		assert(cc_map_size(self->pools) == 0);

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
	assert(self);

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
	assert(self);

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

	// compute the pool count
	const int KB    = 1024;
	const int MB    = 1024*KB;
	uint32_t  count = 1;
	if(stride >= 16*MB)
	{
		count = 1;
	}
	else if(stride >= 4*MB)
	{
		count = 4;
	}
	else if(stride >= MB)
	{
		count = 64;
	}
	else if(stride >= 256*KB)
	{
		count = 256;
	}
	else if(stride >= 64*KB)
	{
		count = 1024;
	}
	else if(stride >= 16*KB)
	{
		count = 4096;
	}
	else
	{
		count = 16384;
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
	assert(self);

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

	// compute the pool count
	const int KB    = 1024;
	const int MB    = 1024*KB;
	uint32_t  count = 1;
	if(stride >= 4*MB)
	{
		count = 1;
	}
	else if(stride >= MB)
	{
		count = 16;
	}
	else if(stride >= 256*KB)
	{
		count = 64;
	}
	else if(stride >= 64*KB)
	{
		count = 256;
	}
	else if(stride >= 16*KB)
	{
		count = 1024;
	}
	else if(stride >= 4*KB)
	{
		count = 4096;
	}
	else
	{
		count = 16384;
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
	assert(self);
	assert(_memory);

	vkk_memoryManager_lock(self);
	vkk_memoryManager_freeLocked(self, _memory);
	vkk_memoryManager_unlock(self);
}

void vkk_memoryManager_update(vkk_memoryManager_t* self,
                              vkk_memory_t* memory,
                              size_t size,
                              const void* buf)
{
	assert(self);
	assert(memory);
	assert(size > 0);
	assert(buf);

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
