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

#ifndef vkk_memoryChunk_H
#define vkk_memoryChunk_H

#include "vkk_memory.h"

typedef struct vkk_memoryChunk_s
{
	vkk_memoryPool_t* pool;

	int            locked;
	int            updater;
	uint32_t       slot;
	uint32_t       usecount;
	VkDeviceMemory memory;

	// freed memory slots
	cc_list_t* slots;
} vkk_memoryChunk_t;

vkk_memoryChunk_t* vkk_memoryChunk_new(vkk_memoryPool_t* pool);
void               vkk_memoryChunk_delete(vkk_memoryChunk_t** _self);
int                vkk_memoryChunk_slots(vkk_memoryChunk_t* self);
vkk_memory_t*      vkk_memoryChunk_alloc(vkk_memoryChunk_t* self);
int                vkk_memoryChunk_free(vkk_memoryChunk_t* self,
                                        int shutdown,
                                        vkk_memory_t** _memory);
void               vkk_memoryChunk_meminfo(vkk_memoryChunk_t* self);

#endif
