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

#ifndef vkk_memory_H
#define vkk_memory_H

#include <vulkan/vulkan.h>

#include "../../libcc/cc_list.h"
#include "../vkk.h"

typedef struct vkk_memory_s        vkk_memory_t;
typedef struct vkk_memoryChunk_s   vkk_memoryChunk_t;
typedef struct vkk_memoryManager_s vkk_memoryManager_t;
typedef struct vkk_memoryPool_s    vkk_memoryPool_t;

typedef struct vkk_memory_s
{
	vkk_memoryChunk_t* chunk;

	VkDeviceSize offset;
} vkk_memory_t;

vkk_memory_t* vkk_memory_new(vkk_memoryChunk_t* chunk,
                             VkDeviceSize offset);
void          vkk_memory_delete(vkk_memory_t** _self);

#endif
