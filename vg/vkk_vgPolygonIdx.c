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
#include "vkk_vgPolygonIdx.h"

/***********************************************************
* public                                                   *
***********************************************************/

vkk_vgPolygonIdx_t*
vkk_vgPolygonIdx_new(vkk_engine_t* engine,
                     uint32_t count,
                     const void* indices)
{
	ASSERT(engine);
	ASSERT(indices);

	vkk_vgPolygonIdx_t* self;
	self = (vkk_vgPolygonIdx_t*)
	       CALLOC(1, sizeof(vkk_vgPolygonIdx_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	// buffer data
	size_t size = count*sizeof(short);
	self->count = count;
	self->ib    = vkk_buffer_new(engine,
	                             VKK_UPDATE_MODE_STATIC,
	                             VKK_BUFFER_USAGE_INDEX,
	                             size, indices);
	if(self->ib == NULL)
	{
		goto fail_ib;
	}

	// success
	return self;

	// failure
	fail_ib:
		FREE(self);
	return NULL;
}

void vkk_vgPolygonIdx_delete(vkk_vgPolygonIdx_t** _self)
{
	ASSERT(_self);

	vkk_vgPolygonIdx_t* self = *_self;
	if(self)
	{
		vkk_buffer_delete(&self->ib);
		FREE(self);
		*_self = NULL;
	}
}
