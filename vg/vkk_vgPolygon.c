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
#include "vkk_vgPolygon.h"
#include "vkk_vgRenderer.h"

/***********************************************************
* protected                                                *
***********************************************************/

vkk_vgPolygon_t*
vkk_vgPolygon_new(vkk_engine_t* engine, size_t size,
                  float* vertices)
{
	ASSERT(engine);
	ASSERT(vertices);

	vkk_vgPolygon_t* self;
	self = (vkk_vgPolygon_t*)
	       CALLOC(1, sizeof(vkk_vgPolygon_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->vb_xy = vkk_buffer_new(engine,
	                             VKK_UPDATE_MODE_STATIC,
	                             VKK_BUFFER_USAGE_VERTEX,
	                             size, vertices);
	if(self->vb_xy == NULL)
	{
		goto fail_vb_xy;
	}

	self->list_idx = cc_list_new();
	if(self->list_idx == NULL)
	{
		goto fail_list_idx;
	}

	// success
	return self;

	// failure
	fail_list_idx:
		vkk_buffer_delete(&self->vb_xy);
	fail_vb_xy:
		FREE(self);
	return NULL;
}

int vkk_vgPolygon_addIdx(vkk_vgPolygon_t* self,
                         vkk_engine_t* engine,
                         uint32_t count,
                         const void* indices)
{
	ASSERT(self);
	ASSERT(engine);
	ASSERT(indices);

	vkk_vgPolygonIdx_t* pi;
	pi = vkk_vgPolygonIdx_new(engine, count, indices);
	if(pi == NULL)
	{
		return 0;
	}

	if(cc_list_append(self->list_idx, NULL,
	                  (const void*) pi) == NULL)
	{
		goto fail_append;
	}

	// success
	return 1;

	// failure
	fail_append:
		vkk_vgPolygonIdx_delete(&pi);
	return 0;
}

/***********************************************************
* public                                                   *
***********************************************************/

void vkk_vgPolygon_delete(vkk_vgPolygon_t** _self)
{
	ASSERT(_self);

	vkk_vgPolygon_t* self = *_self;
	if(self)
	{
		cc_listIter_t* iter;
		iter = cc_list_head(self->list_idx);
		while(iter)
		{
			vkk_vgPolygonIdx_t* pi;
			pi = (vkk_vgPolygonIdx_t*)
			     cc_list_remove(self->list_idx, &iter);
			vkk_vgPolygonIdx_delete(&pi);
		}

		cc_list_delete(&self->list_idx);
		vkk_buffer_delete(&self->vb_xy);
		FREE(self);
		*_self = NULL;
	}
}
