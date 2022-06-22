/*
 * Copyright (c) 2022 Jeff Boody
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
#include "vkk_vgBuffer.h"

/***********************************************************
* private                                                  *
***********************************************************/

static float* vkk_vgBuffer_addElem(vkk_vgBuffer_t* self)
{
	ASSERT(self);

	if(vkk_vgBuffer_resize(self, self->count + 1) == 0)
	{
		return NULL;
	}

	return vkk_vgBuffer_get(self, self->count - 1);
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_vgBuffer_t* vkk_vgBuffer_new(uint32_t elem)
{
	vkk_vgBuffer_t* self;
	self = (vkk_vgBuffer_t*)
	       CALLOC(1, sizeof(vkk_vgBuffer_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->elem = elem;

	return self;
}

void vkk_vgBuffer_delete(vkk_vgBuffer_t** _self)
{
	ASSERT(_self);

	vkk_vgBuffer_t* self = *_self;
	if(self)
	{
		FREE(self->data);
		FREE(self);
		*_self = NULL;
	}
}

void vkk_vgBuffer_reset(vkk_vgBuffer_t* self)
{
	ASSERT(self);

	self->count = 0;
}

int vkk_vgBuffer_resize(vkk_vgBuffer_t* self,
                        uint32_t count)
{
	ASSERT(self);

	size_t size1 = MEMSIZEPTR(self->data);
	size_t size2 = count*self->elem*sizeof(float);
	if(size2 > size1)
	{
		if(size1 == 0)
		{
			size1 = 32*self->elem*sizeof(float);
		}

		while(size1 < size2)
		{
			size1 *= 2;
		}

		float* data;
		data = (float*)
		       REALLOC(self->data, size1);
		if(data == NULL)
		{
			LOGE("REALLOC failed");
			return 0;
		}
		self->data = data;
	}

	self->count = count;

	return 1;
}

float* vkk_vgBuffer_add2(vkk_vgBuffer_t* self,
                         float x, float y)
{
	ASSERT(self);
	ASSERT(self->elem == 2);

	float* data = vkk_vgBuffer_addElem(self);
	if(data)
	{
		data[0] = x;
		data[1] = y;
	}

	return data;
}

float* vkk_vgBuffer_add3(vkk_vgBuffer_t* self,
                         float x, float y, float z)
{
	ASSERT(self);
	ASSERT(self->elem == 3);

	float* data = vkk_vgBuffer_addElem(self);
	if(data)
	{
		data[0] = x;
		data[1] = y;
		data[2] = z;
	}

	return data;
}

float* vkk_vgBuffer_add4(vkk_vgBuffer_t* self,
                         float x, float y,
                         float z, float w)
{
	ASSERT(self);
	ASSERT(self->elem == 4);

	float* data = vkk_vgBuffer_addElem(self);
	if(data)
	{
		data[0] = x;
		data[1] = y;
		data[2] = z;
		data[3] = w;
	}

	return data;
}

size_t vkk_vgBuffer_size(vkk_vgBuffer_t* self)
{
	ASSERT(self);

	return self->elem*self->count*sizeof(float);
}

uint32_t vkk_vgBuffer_count(vkk_vgBuffer_t* self)
{
	ASSERT(self);

	return self->count;
}

float* vkk_vgBuffer_get(vkk_vgBuffer_t* self,
                        uint32_t idx)
{
	ASSERT(self);

	if(idx >= self->count)
	{
		return NULL;
	}

	return &self->data[idx*self->elem];
}
