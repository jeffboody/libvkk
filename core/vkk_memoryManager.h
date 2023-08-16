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

#ifndef vkk_memoryManager_H
#define vkk_memoryManager_H

#include <pthread.h>

#include "../../libcc/cc_map.h"
#include "vkk_memory.h"

#define VKK_CHUNK_UPDATERS 8

typedef struct vkk_memoryManager_s
{
	vkk_engine_t* engine;

	int shutdown;

	// map from mt_index/stride to memory pool
	cc_map_t* pools;

	size_t count_chunks;
	size_t count_slots;
	size_t size_chunks;
	size_t size_slots;

	pthread_mutex_t manager_mutex;
	pthread_mutex_t chunk_mutex[VKK_CHUNK_UPDATERS];
	pthread_cond_t  chunk_cond[VKK_CHUNK_UPDATERS];
	pthread_cond_t  pool_cond;
} vkk_memoryManager_t;

vkk_memoryManager_t* vkk_memoryManager_new(vkk_engine_t* engine);
void                 vkk_memoryManager_delete(vkk_memoryManager_t** _self);
void                 vkk_memoryManager_shutdown(vkk_memoryManager_t* self);
vkk_memory_t*        vkk_memoryManager_allocBuffer(vkk_memoryManager_t* self,
                                                   VkBuffer buffer,
                                                   int local_memory,
                                                   size_t size,
                                                   const void* buf);
vkk_memory_t*        vkk_memoryManager_allocImage(vkk_memoryManager_t* self,
                                                  VkImage image,
                                                  int local_memory);
void                 vkk_memoryManager_free(vkk_memoryManager_t* self,
                                            vkk_memory_t** _memory);
void                 vkk_memoryManager_write(vkk_memoryManager_t* self,
                                             vkk_memory_t* memory,
                                             size_t size,
                                             size_t offset,
                                             const void* buf);
void                 vkk_memoryManager_read(vkk_memoryManager_t* self,
                                            vkk_memory_t* memory,
                                            size_t size,
                                            size_t offset,
                                            void* buf);
void                 vkk_memoryManager_meminfo(vkk_memoryManager_t* self,
                                               size_t* _count_chunks,
                                               size_t* _count_slots,
                                               size_t* _size_chunks,
                                               size_t* _size_slots);

#endif
