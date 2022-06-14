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

#ifndef vkk_memoryPool_H
#define vkk_memoryPool_H

#include "vkk_memory.h"

typedef struct vkk_memoryPool_s
{
	vkk_memoryManager_t* mm;

	int          locked;
	uint32_t     count;
	VkDeviceSize stride;
	uint32_t     mt_index;

	// memory chunks
	cc_list_t* chunks;
} vkk_memoryPool_t;

vkk_memoryPool_t* vkk_memoryPool_new(vkk_memoryManager_t* mm,
                                     uint32_t count,
                                     VkDeviceSize stride,
                                     uint32_t mt_index);
void              vkk_memoryPool_delete(vkk_memoryPool_t** _self);
vkk_memory_t*     vkk_memoryPool_alloc(vkk_memoryPool_t* self);
int               vkk_memoryPool_free(vkk_memoryPool_t* self,
                                      int shutdown,
                                      vkk_memory_t** _memory,
                                      vkk_memoryChunk_t** _chunk);
void              vkk_memoryPool_meminfo(vkk_memoryPool_t* self);

#endif
