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

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define LOG_TAG "vkk"
#include "../../libcc/cc_log.h"
#include "../../libcc/cc_memory.h"
#include "vkk_engine.h"
#include "vkk_memory.h"
#include "vkk_memoryChunk.h"
#include "vkk_memoryManager.h"
#include "vkk_memoryPool.h"

typedef struct
{
	uint32_t mt_index;
	uint32_t stride;
} vkk_memoryPoolKey_t;

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

	int u = chunk->updater;

	pthread_mutex_lock(&self->chunk_mutex[u]);
	TRACE_BEGIN();

	while(chunk->locked)
	{
		TRACE_END();
		pthread_cond_wait(&self->chunk_cond[u],
		                  &self->chunk_mutex[u]);
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

	int u = chunk->updater;

	chunk->locked = 0;

	TRACE_END();
	pthread_mutex_unlock(&self->chunk_mutex[u]);
	pthread_cond_broadcast(&self->chunk_cond[u]);
}

static int
vkk_memoryManager_poolLock(vkk_memoryManager_t* self,
                           vkk_memoryPool_t* pool)
{
	ASSERT(self);
	ASSERT(pool);

	// check if the pool is already locked
	// note that the pool may not exist after waiting
	// so the caller must check if the pool exists
	// before retrying the lock
	if(pool->locked)
	{
		TRACE_END();
		pthread_cond_wait(&self->pool_cond,
		                  &self->manager_mutex);
		TRACE_BEGIN();
		return 0;
	}

	pool->locked = 1;

	TRACE_END();
	pthread_mutex_unlock(&self->manager_mutex);

	return 1;
}

static void
vkk_memoryManager_poolUnlock(vkk_memoryManager_t* self,
                             vkk_memoryPool_t* pool)
{
	ASSERT(self);
	ASSERT(pool);

	pthread_mutex_lock(&self->manager_mutex);
	TRACE_BEGIN();

	pool->locked = 0;

	pthread_cond_broadcast(&self->pool_cond);
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

static vkk_memory_t*
vkk_memoryManager_alloc(vkk_memoryManager_t* self,
                        VkMemoryRequirements* mr,
                        VkFlags mp_flags)
{
	ASSERT(self);
	ASSERT(mr);

	vkk_memoryManager_lock(self);

	vkk_engine_t* engine = self->engine;

	uint32_t mt_index;
	if(vkk_engine_getMemoryTypeIndex(engine,
	                                 mr->memoryTypeBits,
	                                 mp_flags,
	                                 &mt_index) == 0)
	{
		LOGE("invalid memory type");
		goto fail_memory_type;
	}

	// compute the pool stride
	VkDeviceSize stride = mr->alignment;
	while(stride < mr->size)
	{
		stride *= 2;
	}

	vkk_memoryPoolKey_t key =
	{
		.mt_index = (uint32_t) mt_index,
		.stride   = (uint32_t) stride
	};

	// find an existing pool
	int locked       = 0;
	int pool_created = 0;
	cc_mapIter_t*     miter;
	vkk_memoryPool_t* pool;
	while(locked == 0)
	{
		miter = cc_map_findp(self->pools,
		                     sizeof(vkk_memoryPoolKey_t), &key);
		if(miter == NULL)
		{
			// otherwise create a new pool
			uint32_t count = computePoolCount((size_t) stride);
			pool = vkk_memoryPool_new(self, count, stride, mt_index);
			if(pool == NULL)
			{
				goto fail_pool;
			}
			pool_created = 1;

			miter = cc_map_addp(self->pools, (const void*) pool,
			                    sizeof(vkk_memoryPoolKey_t),
			                    &key);
			if(miter == NULL)
			{
				goto fail_add;
			}
		}
		else
		{
			pool = (vkk_memoryPool_t*) cc_map_val(miter);
		}

		locked = vkk_memoryManager_poolLock(self, pool);
	}

	// allocate memory
	vkk_memory_t* memory = vkk_memoryPool_alloc(pool);
	if(memory == NULL)
	{
		vkk_memoryManager_poolUnlock(self, pool);
		goto fail_alloc;
	}
	vkk_memoryManager_poolUnlock(self, pool);
	vkk_memoryManager_unlock(self);

	// success
	return memory;

	// failure
	fail_alloc:
	{
		if(pool_created)
		{
			cc_map_remove(self->pools, &miter);
		}
	}
	fail_add:
	{
		if(pool_created)
		{
			vkk_memoryPool_delete(&pool);
		}
	}
	fail_pool:
	fail_memory_type:
		vkk_memoryManager_unlock(self);
	return NULL;
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

	int u;
	for(u = 0; u < VKK_CHUNK_UPDATERS; ++u)
	{
		if(pthread_mutex_init(&self->chunk_mutex[u], NULL) != 0)
		{
			LOGE("pthread_mutex_init failed");
			goto fail_chunk_mutex;
		}
	}

	int v;
	for(v = 0; v < VKK_CHUNK_UPDATERS; ++v)
	{
		if(pthread_cond_init(&self->chunk_cond[v], NULL) != 0)
		{
			LOGE("pthread_cond_init failed");
			goto fail_chunk_cond;
		}
	}

	if(pthread_cond_init(&self->pool_cond, NULL) != 0)
	{
		LOGE("pthread_cond_init failed");
		goto fail_pool_cond;
	}

	// success
	return self;

	// failure
	fail_pool_cond:
	{
		int i;
		for(i = 0; i < v; ++i)
		{
			pthread_cond_destroy(&self->chunk_cond[i]);
		}
	}
	fail_chunk_cond:
	{
		int i;
		for(i = 0; i < u; ++i)
		{
			pthread_mutex_destroy(&self->chunk_mutex[i]);
		}
	}
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

		pthread_cond_destroy(&self->pool_cond);

		int u;
		for(u = 0; u < VKK_CHUNK_UPDATERS; ++u)
		{
			pthread_cond_destroy(&self->chunk_cond[u]);
			pthread_mutex_destroy(&self->chunk_mutex[u]);
		}
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
                              int local_memory,
                              size_t size,
                              const void* buf)
{
	// buf may be NULL
	ASSERT(self);

	vkk_engine_t* engine = self->engine;

	VkMemoryRequirements mr;
	vkGetBufferMemoryRequirements(engine->device,
	                              buffer, &mr);

	VkFlags mp_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
	                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	if(local_memory)
	{
		mp_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	}

	vkk_memory_t* memory;
	memory = vkk_memoryManager_alloc(self, &mr, mp_flags);
	if(memory == NULL)
	{
		return NULL;
	}

	if(buf)
	{
		vkk_memoryManager_write(self, memory, size, 0, buf);
	}

	vkk_memoryManager_chunkLock(self, memory->chunk);
	if(vkBindBufferMemory(engine->device, buffer,
	                      memory->chunk->memory,
	                      memory->offset) != VK_SUCCESS)
	{
		LOGE("vkBindBufferMemory failed");
		vkk_memoryManager_chunkUnlock(self, memory->chunk);
		goto fail_bind;
	}
	vkk_memoryManager_chunkUnlock(self, memory->chunk);

	// success
	return memory;

	// failure
	fail_bind:
		vkk_memoryManager_free(self, &memory);
	return NULL;
}

vkk_memory_t*
vkk_memoryManager_allocImage(vkk_memoryManager_t* self,
                             VkImage image,
                             int local_memory)
{
	ASSERT(self);

	vkk_engine_t* engine = self->engine;

	VkMemoryRequirements mr;
	vkGetImageMemoryRequirements(engine->device,
	                             image, &mr);

	// note that the local_memory flag allows the allocation
	// to be performed in tiled memory
	// this is used for cases where the image memory is used
	// for rendering and is not visible to the user
	// e.g. the depth and MSAA images
	VkFlags mp_flags = 0;
	if(local_memory)
	{
		mp_flags = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;

		uint32_t mt_index;
		if(vkk_engine_getMemoryTypeIndex(engine,
		                                 mr.memoryTypeBits,
		                                 mp_flags,
		                                 &mt_index) == 0)
		{
			mp_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		}
	}

	vkk_memory_t* memory;
	memory = vkk_memoryManager_alloc(self, &mr, mp_flags);
	if(memory == NULL)
	{
		return NULL;
	}

	vkk_memoryManager_chunkLock(self, memory->chunk);
	if(vkBindImageMemory(engine->device, image,
	                     memory->chunk->memory,
	                     memory->offset) != VK_SUCCESS)
	{
		LOGE("vkBindImageMemory failed");
		vkk_memoryManager_chunkUnlock(self, memory->chunk);
		goto fail_bind;
	}
	vkk_memoryManager_chunkUnlock(self, memory->chunk);

	// success
	return memory;

	// failure
	fail_bind:
		vkk_memoryManager_free(self, &memory);
	return NULL;
}

void vkk_memoryManager_free(vkk_memoryManager_t* self,
                            vkk_memory_t** _memory)
{
	ASSERT(self);
	ASSERT(_memory);

	vkk_memory_t* memory = *_memory;
	if(memory)
	{
		vkk_memoryManager_lock(self);

		// free the memory and delete pool (if necessary);
		vkk_memoryPool_t* pool = memory->chunk->pool;

		int locked = 0;
		while(locked == 0)
		{
			locked = vkk_memoryManager_poolLock(self, pool);
		}

		vkk_memoryPoolKey_t key =
		{
			.mt_index = (uint32_t) pool->mt_index,
			.stride   = (uint32_t) pool->stride
		};

		vkk_memoryChunk_t* chunk = NULL;
		if(vkk_memoryPool_free(pool, self->shutdown,
		                       _memory, &chunk))
		{
			cc_mapIter_t* miter;
			miter = cc_map_findp(self->pools,
			                     sizeof(vkk_memoryPoolKey_t), &key);

			vkk_memoryPool_t* tmp;
			tmp = (vkk_memoryPool_t*) cc_map_val(miter);
			assert(tmp == pool);

			cc_map_remove(self->pools, &miter);
			vkk_memoryManager_poolUnlock(self, pool);
		}
		else
		{
			vkk_memoryManager_poolUnlock(self, pool);
			pool = NULL;
		}
		vkk_memoryManager_unlock(self);

		// chunk and pool may be deleted while unlocked
		// since they have been detached from the manager
		vkk_memoryChunk_delete(&chunk);
		vkk_memoryPool_delete(&pool);
	}
}

void vkk_memoryManager_write(vkk_memoryManager_t* self,
                             vkk_memory_t* memory,
                             size_t size,
                             size_t offset,
                             const void* buf)
{
	ASSERT(self);
	ASSERT(memory);
	ASSERT(buf);

	vkk_memoryChunk_t*   chunk  = memory->chunk;
	vkk_memoryPool_t*    pool   = chunk->pool;
	vkk_memoryManager_t* mm     = pool->mm;
	vkk_engine_t*        engine = mm->engine;

	vkk_memoryManager_chunkLock(self, chunk);

	if((size == 0) || (size + offset > pool->stride))
	{
		LOGE("invalid size=%" PRIu64 ", offset=%" PRIu64
		     ", stride=%" PRIu64,
		     (uint64_t) size, (uint64_t) offset,
		     (uint64_t) pool->stride);
		vkk_memoryManager_chunkUnlock(self, chunk);
		return;
	}

	void* data;
	if(vkMapMemory(engine->device, chunk->memory,
	               memory->offset + offset, size, 0,
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

void vkk_memoryManager_read(vkk_memoryManager_t* self,
                            vkk_memory_t* memory,
                            size_t size, size_t offset,
                            void* buf)
{
	ASSERT(self);
	ASSERT(memory);
	ASSERT(buf);

	vkk_memoryChunk_t*   chunk  = memory->chunk;
	vkk_memoryPool_t*    pool   = chunk->pool;
	vkk_memoryManager_t* mm     = pool->mm;
	vkk_engine_t*        engine = mm->engine;

	vkk_memoryManager_chunkLock(self, chunk);

	if((size == 0) || (size + offset > pool->stride))
	{
		LOGE("invalid size=%" PRIu64 ", offset=%" PRIu64
		     ", stride=%" PRIu64,
		     (uint64_t) size, (uint64_t) offset,
		     (uint64_t) pool->stride);
		vkk_memoryManager_chunkUnlock(self, chunk);
		return;
	}

	void* data;
	if(vkMapMemory(engine->device, chunk->memory,
	               memory->offset + offset, size, 0,
	               &data) == VK_SUCCESS)
	{
		memcpy(buf, data, size);
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

		cc_mapIter_t* miter = cc_map_head(self->pools);
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
