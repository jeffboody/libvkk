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
#include "vkk_vgLine.h"
#include "vkk_vgRenderer.h"

/***********************************************************
* protected                                                *
***********************************************************/

vkk_vgLine_t*
vkk_vgLine_new(vkk_engine_t* engine,
               float dist, uint32_t vc,
               cc_vec4f_t* xyst, cc_vec2f_t* dxdy)
{
	ASSERT(engine);
	ASSERT(vc > 2);
	ASSERT(xyst);
	ASSERT(dxdy);

	vkk_vgLine_t* self;
	self = (vkk_vgLine_t*)
	       CALLOC(1, sizeof(vkk_vgLine_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->dist = dist;
	self->vc   = vc;

	size_t size_xyst = vc*sizeof(cc_vec4f_t);
	self->vb_xyst = vkk_buffer_new(engine,
	                               VKK_UPDATE_MODE_STATIC,
	                               VKK_BUFFER_USAGE_VERTEX,
	                               size_xyst, xyst);
	if(self->vb_xyst == NULL)
	{
		goto fail_vb_xyst;
	}

	size_t size_dxdy = vc*sizeof(cc_vec2f_t);
	self->vb_dxdy = vkk_buffer_new(engine,
	                               VKK_UPDATE_MODE_STATIC,
	                               VKK_BUFFER_USAGE_VERTEX,
	                               size_dxdy, dxdy);
	if(self->vb_dxdy == NULL)
	{
		goto fail_vb_dxdy;
	}

	// success
	return self;

	// failure
	fail_vb_dxdy:
		vkk_buffer_delete(&self->vb_xyst);
	fail_vb_xyst:
		FREE(self);
	return NULL;
}

/***********************************************************
* public                                                   *
***********************************************************/

void vkk_vgLine_delete(vkk_vgLine_t** _self)
{
	ASSERT(_self);

	vkk_vgLine_t* self = *_self;
	if(self)
	{
		vkk_buffer_delete(&self->vb_dxdy);
		vkk_buffer_delete(&self->vb_xyst);
		FREE(self);
		*_self = NULL;
	}
}
