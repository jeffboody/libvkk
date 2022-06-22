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

#include <math.h>
#include <stdlib.h>

#define LOG_TAG "vkk"
#include "../../libcc/math/cc_vec2f.h"
#include "../../libcc/cc_log.h"
#include "../../libcc/cc_memory.h"
#include "../../libtess2/Include/tesselator.h"
#include "vkk_vgBuffer.h"
#include "vkk_vgPolygonBuilder.h"
#include "vkk_vgPolygon.h"

/***********************************************************
* public                                                   *
***********************************************************/

vkk_vgPolygonBuilder_t*
vkk_vgPolygonBuilder_new(vkk_engine_t* engine)
{
	ASSERT(engine);

	vkk_vgPolygonBuilder_t* self;
	self = (vkk_vgPolygonBuilder_t*)
	       CALLOC(1, sizeof(vkk_vgPolygonBuilder_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->engine = engine;

	self->contours = cc_list_new();
	if(self->contours == NULL)
	{
		goto fail_contours;
	}

	self->buffers = cc_list_new();
	if(self->buffers == NULL)
	{
		goto fail_buffers;
	}

	// success
	return self;

	// failure
	fail_buffers:
		cc_list_delete(&self->contours);
	fail_contours:
		FREE(self);
	return NULL;
}

void vkk_vgPolygonBuilder_delete(vkk_vgPolygonBuilder_t** _self)
{
	ASSERT(_self);

	vkk_vgPolygonBuilder_t* self = *_self;
	if(self)
	{
		cc_list_appendList(self->buffers, self->contours);

		cc_listIter_t* iter = cc_list_head(self->buffers);
		while(iter)
		{
			vkk_vgBuffer_t* contour;
			contour = (vkk_vgBuffer_t*)
			          cc_list_remove(self->buffers, &iter);
			vkk_vgBuffer_delete(&contour);
		}

		cc_list_delete(&self->buffers);
		cc_list_delete(&self->contours);
		FREE(self);
		*_self = NULL;
	}
}

void vkk_vgPolygonBuilder_reset(vkk_vgPolygonBuilder_t* self)
{
	ASSERT(self);

	cc_list_appendList(self->buffers, self->contours);
}

vkk_vgPolygon_t*
vkk_vgPolygonBuilder_build(vkk_vgPolygonBuilder_t* self)
{
	ASSERT(self);

	vkk_engine_t* engine = self->engine;

	// TODO - convert LOGD to LOGE
	// some polygon build errors are difficult to detect at
	// an earlier stage that are related to the tiling of
	// OpenStreetMap data

	if(cc_list_size(self->contours) == 0)
	{
		LOGD("invalid contours");
		vkk_vgPolygonBuilder_reset(self);
		return NULL;
	}

	TESStesselator* tess = tessNewTess(NULL);
	if(tess == NULL)
	{
		LOGE("tessNewTess failed");
		vkk_vgPolygonBuilder_reset(self);
		return NULL;
	}

	// add contours
	cc_listIter_t* iter = cc_list_head(self->contours);
	while(iter)
	{
		vkk_vgBuffer_t* contour;
		contour = (vkk_vgBuffer_t*) cc_list_peekIter(iter);

		// at least 3 points required
		uint32_t count = vkk_vgBuffer_count(contour);
		if(count >= 3)
		{
			tessAddContour(tess, 2,
			               vkk_vgBuffer_get(contour, 0),
			               2*sizeof(float),
			               vkk_vgBuffer_count(contour));
		}

		iter = cc_list_next(iter);
	}

	// build polygon(s)
	int polySize = 32;
	if(tessTesselate(tess, TESS_WINDING_ODD,
	                 TESS_POLYGONS, polySize, 2, NULL) == 0)
	{
		LOGD("tessTesselate failed");
		goto fail_tesselate;
	}

	// check vertices
	float*   vertices = (float*) tessGetVertices(tess);
	uint32_t vc       = tessGetVertexCount(tess);
	size_t   size     = 2*sizeof(float)*vc;
	if((vertices == NULL) || (size == 0))
	{
		LOGD("invalid vertices=%p, size=%u",
		     vertices, (uint32_t) size);
		goto fail_vertices;
	}

	vkk_vgPolygon_t* polygon;
	polygon = vkk_vgPolygon_new(engine, size, vertices);
	if(polygon == NULL)
	{
		goto fail_polygon;
	}

	// buffer indices
	int i;
	int ele_count = tessGetElementCount(tess);
	const TESSindex* elems = tessGetElements(tess);
	for(i = 0; i < ele_count; ++i)
	{
		// count the number of indices in polyi
		const TESSindex* poly = &elems[i*polySize];
		uint32_t idx_count;
		for(idx_count = 0; idx_count < polySize; ++idx_count)
		{
			if(poly[idx_count] == TESS_UNDEF)
			{
				break;
			}
		}

		if(vkk_vgPolygon_addIdx(polygon, engine,
		                        idx_count, poly) == 0)
		{
			goto fail_idx;
		}
	}

	tessDeleteTess(tess);
	vkk_vgPolygonBuilder_reset(self);

	// success
	return polygon;

	// failure
	fail_idx:
		vkk_vgPolygon_delete(&polygon);
	fail_polygon:
	fail_vertices:
	fail_tesselate:
		tessDeleteTess(tess);
		vkk_vgPolygonBuilder_reset(self);
	return NULL;
}

int
vkk_vgPolygonBuilder_point(vkk_vgPolygonBuilder_t* self,
                           int first, float x, float y)
{
	ASSERT(self);

	vkk_vgBuffer_t* contour;
	if(first)
	{
		cc_listIter_t* iter;
		iter = cc_list_head(self->buffers);
		if(iter)
		{
			// add pt to an existing contour buffer
			contour = (vkk_vgBuffer_t*)
			          cc_list_peekIter(iter);

			vkk_vgBuffer_reset(contour);
			if(vkk_vgBuffer_add2(contour, x, y) == NULL)
			{
				return 0;
			}

			cc_list_swapn(self->buffers, self->contours,
			              iter, NULL);
		}
		else
		{
			// add pt to a new contour buffer
			contour = vkk_vgBuffer_new(2);
			if(contour == NULL)
			{
				return 0;
			}

			if(vkk_vgBuffer_add2(contour, x, y) == NULL)
			{
				vkk_vgBuffer_delete(&contour);
				return 0;
			}

			if(cc_list_append(self->contours, NULL,
			                  (const void*) contour) == NULL)
			{
				vkk_vgBuffer_delete(&contour);
				return 0;
			}
		}
	}
	else
	{
		// add pt to the last contour
		contour = (vkk_vgBuffer_t*)
		          cc_list_peekTail(self->contours);
		if(contour == NULL)
		{
			LOGE("invalid contour");
			return 0;
		}

		// eliminate duplicate points
		uint32_t count = vkk_vgBuffer_count(contour);
		if(count > 0)
		{
			cc_vec2f_t* last;
			last = (cc_vec2f_t*)
			       vkk_vgBuffer_get(contour, count - 1);
			if((last->x == x) && (last->y == y))
			{
				// ignore point
				return 1;
			}
		}

		if(vkk_vgBuffer_add2(contour, x, y) == NULL)
		{
			return 0;
		}
	}

	return 1;
}
