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
#include "vkk_memoryPool.h"

/***********************************************************
* public                                                   *
***********************************************************/

vkk_memory_t*
vkk_memory_new(vkk_memoryChunk_t* chunk,
               VkDeviceSize offset)
{
	ASSERT(chunk);

	vkk_memory_t* self;
	self = (vkk_memory_t*)
	       CALLOC(1, sizeof(vkk_memory_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->chunk  = chunk;
	self->offset = offset;

	return self;
}

void vkk_memory_delete(vkk_memory_t** _self)
{
	ASSERT(_self);

	vkk_memory_t* self = *_self;
	if(self)
	{
		FREE(self);
		*_self = NULL;
	}
}
