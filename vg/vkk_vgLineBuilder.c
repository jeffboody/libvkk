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
#include "../../libcc/cc_log.h"
#include "../../libcc/cc_memory.h"
#include "vkk_vgLineBuilder.h"
#include "vkk_vgLine.h"

/***********************************************************
* private                                                  *
***********************************************************/

static void
vkk_vgLineBuilder_intersect(const cc_vec2f_t* p1,
                            const cc_vec2f_t* p2,
                            const cc_vec2f_t* p3,
                            const cc_vec2f_t* p4,
                            cc_vec2f_t* p, float* _t)
{
	ASSERT(p1);
	ASSERT(p2);
	ASSERT(p3);
	ASSERT(p4);
	ASSERT(p);
	ASSERT(_t);

	// https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection

	// check for parallel lines
	float x1 = p1->x;
	float x2 = p2->x;
	float x3 = p3->x;
	float x4 = p4->x;
	float y1 = p1->y;
	float y2 = p2->y;
	float y3 = p3->y;
	float y4 = p4->y;
	float d = (x1 - x2)*(y3 - y4) - (y1 - y2)*(x3 - x4);
	if(fabs(d) < 0.001)
	{
		p->x = x2;
		p->y = y2;
		return;
	}

	// compute intersection point
	float n = (x1 - x3)*(y3 - y4) - (y1 - y3)*(x3 - x4);
	float t = n/d;
	p->x = x1 + t*(x2 - x1);
	p->y = y1 + t*(y2 - y1);
	*_t = t;
}

static void
vkk_vgLineBuilder_buildContour(vkk_vgLineBuilder_t* self,
                               int loop)
{
	ASSERT(self);

	cc_vec3f_t* xyd;
	float*      xyst;
	float*      dxdy;
	xyd  = (cc_vec3f_t*)
	       vkk_vgBuffer_get(self->xyd, 0);
	xyst = vkk_vgBuffer_get(self->xyst, 0);
	dxdy = vkk_vgBuffer_get(self->dxdy, 0);

	uint32_t count = vkk_vgBuffer_count(self->xyd);

	/*
	 * Corners Diagram
	 *
	 *    q----g----r----------f    i    c    k
	 *    |         |               |         |
	 *    |         |               |         |  RIGHT TURN
	 *    j    b    h          a    |         |  tp > 1
	 *    |         |               |         |
	 *    |         |               |         |
	 *    s----e----p----------d    r----g----q----------f
	 *    |         |               |         |
	 *    |         |               |         |
	 *    |         |               h    b    j          a
	 *    |         |  LEFT TURN    |         |
	 *    |         |  tp < 1       |         |
	 *    k    c    i               p----e----s----------d
	 *
	 * Triangle Strips:
	 * LEFT:  df-pr-pq-ps-ik
	 * RIGHT: df-sq-pq-rq-ik
	 *
	 */

	int idx0 = 0;
	int idx1 = 0;

	// points are vec3 (x, y, dist) but also cast as vec2
	cc_vec2f_t* a2 = NULL;
	cc_vec2f_t* b2 = NULL;
	cc_vec2f_t* c2 = NULL;
	cc_vec3f_t* a3 = NULL;
	cc_vec3f_t* b3 = NULL;
	cc_vec3f_t* c3 = NULL;

	// build line
	uint32_t   i;
	cc_vec2f_t p;
	cc_vec2f_t q;
	cc_vec2f_t r;
	cc_vec2f_t s;
	for(i = 0; i < count; ++i)
	{
		// initialize points
		if(i == 0)
		{
			a3 = NULL;
		}
		else
		{
			a3 = &xyd[i - 1];
		}

		b3 = &xyd[i];

		if((i + 1) < count)
		{
			c3 = &xyd[i + 1];
		}
		else
		{
			c3 = NULL;
		}

		a2 = (cc_vec2f_t*) a3;
		b2 = (cc_vec2f_t*) b3;
		c2 = (cc_vec2f_t*) c3;

		// connect the loop
		// note that loops have at least 3 points
		if(loop)
		{
			if(a2 == NULL)
			{
				a3 = &xyd[count - 2];
				a2 = (cc_vec2f_t*) a3;
			}
			else if(c2 == NULL)
			{
				c3 = &xyd[1];
				c2 = (cc_vec2f_t*) c3;
			}
		}

		// handle endpoints
		if((a2 == NULL) || (c2 == NULL))
		{
			// compute the direction to the end point
			cc_vec2f_t e;
			float se; // direction to vtx
			if(a2 == NULL)
			{
				se = -1.0f;
				cc_vec2f_subv_copy(c2, b2, &e);
			}
			else
			{
				se = 1.0f;
				cc_vec2f_subv_copy(b2, a2, &e);
			}
			cc_vec2f_normalize(&e);

			// compute p,q tangent to the end point
			cc_vec3f_t e3 = { e.x, e.y, 0.0f  };
			cc_vec3f_t up = { 0.0f, 0.0f, 1.0f };
			cc_vec3f_t v;
			cc_vec3f_cross_copy(&e3, &up, &v);
			cc_vec3f_normalize(&v);
			cc_vec3f_muls(&v, 0.5f);
			cc_vec2f_load(&p, b2->x, b2->y);
			cc_vec2f_load(&q, b2->x, b2->y);
			cc_vec2f_subv(&p, (const cc_vec2f_t*) &v);
			cc_vec2f_addv(&q, (const cc_vec2f_t*) &v);

			// add a cap
			cc_vec2f_muls(&e, 0.5f*se);
			cc_vec2f_addv(&p, (const cc_vec2f_t*) &e);
			cc_vec2f_addv(&q, (const cc_vec2f_t*) &e);

			// add the end point
			xyst[idx0++] = b3->x;
			xyst[idx0++] = b3->y;
			xyst[idx0++] = b3->z;
			xyst[idx0++] = -1.0f;
			xyst[idx0++] = b3->x;
			xyst[idx0++] = b3->y;
			xyst[idx0++] = b3->z;
			xyst[idx0++] = 1.0f;
			dxdy[idx1++] = p.x - b3->x;
			dxdy[idx1++] = p.y - b3->y;
			dxdy[idx1++] = q.x - b3->x;
			dxdy[idx1++] = q.y - b3->y;

			continue;
		}

		// compute the vectors for ab and bc
		cc_vec2f_t ab;
		cc_vec2f_t bc;
		cc_vec2f_t uab;
		cc_vec2f_t ubc;
		cc_vec2f_subv_copy(b2, a2, &ab);
		cc_vec2f_subv_copy(c2, b2, &bc);
		cc_vec2f_normalize_copy(&ab, &uab);
		cc_vec2f_normalize_copy(&bc, &ubc);
		cc_vec3f_t uab3 = { uab.x, uab.y, 0.0f };
		cc_vec3f_t ubc3 = { ubc.x, ubc.y, 0.0f };

		// compute the tangent vectors to ab and bc
		cc_vec3f_t vab;
		cc_vec3f_t vbc;
		cc_vec3f_t up = { 0.0f,  0.0f,  1.0f };
		cc_vec3f_cross_copy(&uab3, &up, &vab);
		cc_vec3f_cross_copy(&ubc3, &up, &vbc);
		cc_vec3f_normalize(&vab);
		cc_vec3f_normalize(&vbc);

		// compute points on parallel lines to ab and bc
		cc_vec2f_t d;
		cc_vec2f_t f;
		cc_vec2f_t e;
		cc_vec2f_t g;
		cc_vec2f_t h;
		cc_vec2f_t j;
		cc_vec2f_t i;
		cc_vec2f_t k;
		cc_vec3f_muls(&vab, 0.5f);
		cc_vec3f_muls(&vbc, 0.5f);
		cc_vec2f_subv_copy(a2, (const cc_vec2f_t*) &vab, &d);
		cc_vec2f_addv_copy(a2, (const cc_vec2f_t*) &vab, &f);
		cc_vec2f_subv_copy(b2, (const cc_vec2f_t*) &vab, &e);
		cc_vec2f_addv_copy(b2, (const cc_vec2f_t*) &vab, &g);
		cc_vec2f_subv_copy(b2, (const cc_vec2f_t*) &vbc, &h);
		cc_vec2f_addv_copy(b2, (const cc_vec2f_t*) &vbc, &j);
		cc_vec2f_subv_copy(c2, (const cc_vec2f_t*) &vbc, &i);
		cc_vec2f_addv_copy(c2, (const cc_vec2f_t*) &vbc, &k);

		// compute intersection points
		float tp       = 1.0f;
		float tq       = 1.0f;
		float dot_abbc = cc_vec2f_dot(&uab, &ubc);
		float cos_75   = 0.26f;
		float cos_105  = -0.26f;
		if(dot_abbc > 0.996f)   // 5 degrees
		{
			// intersection of coincident lines
			cc_vec2f_copy(&g, &q);
			cc_vec2f_copy(&h, &p);
		}
		else
		{
			// intersection of non-coincident lines
			vkk_vgLineBuilder_intersect(&d, &e, &h, &i, &p, &tp);
			vkk_vgLineBuilder_intersect(&f, &g, &j, &k, &q, &tq);
		}
		cc_vec3f_muls(&vab, 2.0f);
		cc_vec3f_muls(&vbc, 2.0f);

		if(tp < 1.0f)
		{
			// LEFT TURN

			// bevel corners except near right angles
			if((dot_abbc > cos_75) || (dot_abbc < cos_105))
			{
				cc_vec2f_t pb;
				cc_vec2f_subv_copy(b2, &p, &pb);
				cc_vec2f_addv_copy(b2, &pb, &q);
			}
			cc_vec2f_addv_copy(&p, (const cc_vec2f_t*) &vab, &r);
			cc_vec2f_addv_copy(&p, (const cc_vec2f_t*) &vbc, &s);

			// add first pair
			xyst[idx0++] = b3->x;
			xyst[idx0++] = b3->y;
			xyst[idx0++] = b3->z;
			xyst[idx0++] = -1.0f;
			xyst[idx0++] = b3->x;
			xyst[idx0++] = b3->y;
			xyst[idx0++] = b3->z;
			xyst[idx0++] = 1.0f;
			dxdy[idx1++] = p.x - b3->x;
			dxdy[idx1++] = p.y - b3->y;
			dxdy[idx1++] = r.x - b3->x;
			dxdy[idx1++] = r.y - b3->y;

			// add second pair
			xyst[idx0++] = b3->x;
			xyst[idx0++] = b3->y;
			xyst[idx0++] = b3->z;
			xyst[idx0++] = -1.0f;
			xyst[idx0++] = b3->x;
			xyst[idx0++] = b3->y;
			xyst[idx0++] = b3->z;
			xyst[idx0++] = 1.0f;
			dxdy[idx1++] = p.x - b3->x;
			dxdy[idx1++] = p.y - b3->y;
			dxdy[idx1++] = q.x - b3->x;
			dxdy[idx1++] = q.y - b3->y;

			// add third pair
			xyst[idx0++] = b3->x;
			xyst[idx0++] = b3->y;
			xyst[idx0++] = b3->z;
			xyst[idx0++] = -1.0f;
			xyst[idx0++] = b3->x;
			xyst[idx0++] = b3->y;
			xyst[idx0++] = b3->z;
			xyst[idx0++] = 1.0f;
			dxdy[idx1++] = p.x - b3->x;
			dxdy[idx1++] = p.y - b3->y;
			dxdy[idx1++] = s.x - b3->x;
			dxdy[idx1++] = s.y - b3->y;
		}
		else
		{
			// RIGHT TURN

			// bevel corners except near right angles
			if((dot_abbc > cos_75) || (dot_abbc < cos_105))
			{
				cc_vec2f_t qb;
				cc_vec2f_subv_copy(b2, &q, &qb);
				cc_vec2f_addv_copy(b2, &qb, &p);
			}
			cc_vec2f_subv_copy(&q, (const cc_vec2f_t*) &vab, &s);
			cc_vec2f_subv_copy(&q, (const cc_vec2f_t*) &vbc, &r);

			// add first pair
			xyst[idx0++] = b3->x;
			xyst[idx0++] = b3->y;
			xyst[idx0++] = b3->z;
			xyst[idx0++] = -1.0f;
			xyst[idx0++] = b3->x;
			xyst[idx0++] = b3->y;
			xyst[idx0++] = b3->z;
			xyst[idx0++] = 1.0f;
			dxdy[idx1++] = s.x - b3->x;
			dxdy[idx1++] = s.y - b3->y;
			dxdy[idx1++] = q.x - b3->x;
			dxdy[idx1++] = q.y - b3->y;

			// add second pair
			xyst[idx0++] = b3->x;
			xyst[idx0++] = b3->y;
			xyst[idx0++] = b3->z;
			xyst[idx0++] = -1.0f;
			xyst[idx0++] = b3->x;
			xyst[idx0++] = b3->y;
			xyst[idx0++] = b3->z;
			xyst[idx0++] = 1.0f;
			dxdy[idx1++] = p.x - b3->x;
			dxdy[idx1++] = p.y - b3->y;
			dxdy[idx1++] = q.x - b3->x;
			dxdy[idx1++] = q.y - b3->y;

			// add third pair
			xyst[idx0++] = b3->x;
			xyst[idx0++] = b3->y;
			xyst[idx0++] = b3->z;
			xyst[idx0++] = -1.0f;
			xyst[idx0++] = b3->x;
			xyst[idx0++] = b3->y;
			xyst[idx0++] = b3->z;
			xyst[idx0++] = 1.0f;
			dxdy[idx1++] = r.x - b3->x;
			dxdy[idx1++] = r.y - b3->y;
			dxdy[idx1++] = q.x - b3->x;
			dxdy[idx1++] = q.y - b3->y;
		}
	}

	// close loop
	if(loop)
	{
		// add final pair
		xyst[idx0++] = xyst[0];
		xyst[idx0++] = xyst[1];
		xyst[idx0++] = c3->z;
		xyst[idx0++] = xyst[3];
		xyst[idx0++] = xyst[4];
		xyst[idx0++] = xyst[5];
		xyst[idx0++] = c3->z;
		xyst[idx0++] = xyst[7];
		dxdy[idx1++] = dxdy[0];
		dxdy[idx1++] = dxdy[1];
		dxdy[idx1++] = dxdy[4];
		dxdy[idx1++] = dxdy[5];
	}
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_vgLineBuilder_t*
vkk_vgLineBuilder_new(vkk_engine_t* engine)
{
	ASSERT(engine);

	vkk_vgLineBuilder_t* self;
	self = (vkk_vgLineBuilder_t*)
	       CALLOC(1, sizeof(vkk_vgLineBuilder_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->engine = engine;

	self->xyd = vkk_vgBuffer_new(3);
	if(self->xyd == NULL)
	{
		goto fail_xyd;
	}

	self->xyst = vkk_vgBuffer_new(4);
	if(self->xyst == NULL)
	{
		goto fail_xyst;
	}

	self->dxdy = vkk_vgBuffer_new(2);
	if(self->dxdy == NULL)
	{
		goto fail_dxdy;
	}

	// success
	return self;

	// failure
	fail_dxdy:
		vkk_vgBuffer_delete(&self->xyst);
	fail_xyst:
		vkk_vgBuffer_delete(&self->xyd);
	fail_xyd:
		FREE(self);
	return NULL;
}

void vkk_vgLineBuilder_delete(vkk_vgLineBuilder_t** _self)
{
	ASSERT(_self);

	vkk_vgLineBuilder_t* self = *_self;
	if(self)
	{
		vkk_vgBuffer_delete(&self->dxdy);
		vkk_vgBuffer_delete(&self->xyst);
		vkk_vgBuffer_delete(&self->xyd);
		FREE(self);
		*_self = NULL;
	}
}

void vkk_vgLineBuilder_reset(vkk_vgLineBuilder_t* self)
{
	ASSERT(self);

	self->dist = 0.0f;
	vkk_vgBuffer_reset(self->xyd);
	vkk_vgBuffer_reset(self->xyst);
	vkk_vgBuffer_reset(self->dxdy);
}

vkk_vgLine_t*
vkk_vgLineBuilder_build(vkk_vgLineBuilder_t* self)
{
	ASSERT(self);

	// lines require at least 2 points
	uint32_t count = vkk_vgBuffer_count(self->xyd);
	if(count < 2)
	{
		vkk_vgLineBuilder_reset(self);
		return NULL;
	}

	// detect loops and determine vc
	// loops require at least 3 points
	int         loop = 0;
	uint32_t    vc;
	cc_vec3f_t* head;
	cc_vec3f_t* tail;
	head = (cc_vec3f_t*)
	       vkk_vgBuffer_get(self->xyd, 0);
	tail = (cc_vec3f_t*)
	       vkk_vgBuffer_get(self->xyd, count - 1);
	if((count >= 3)         &&
	   (head->x == tail->x) &&
	   (head->y == tail->y))
	{
		// 6 for interior points
		// 2 to connect loop
		loop = 1;
		vc   = 6*count + 2;
	}
	else
	{
		// 6 for interior points
		// 2 for end points
		vc = 6*(count - 2) + 4;
	}

	// resize xyst and dxdy
	if((vkk_vgBuffer_resize(self->xyst, vc) == 0) ||
	   (vkk_vgBuffer_resize(self->dxdy, vc) == 0))
	{
		vkk_vgLineBuilder_reset(self);
		return NULL;
	}

	vkk_vgLineBuilder_buildContour(self, loop);

	cc_vec4f_t* xyst;
	cc_vec2f_t* dxdy;
	xyst = (cc_vec4f_t*)
	       vkk_vgBuffer_get(self->xyst, 0);
	dxdy = (cc_vec2f_t*)
	       vkk_vgBuffer_get(self->dxdy, 0);

	vkk_vgLine_t* line;
	line = vkk_vgLine_new(self->engine,
	                      self->dist, vc,
	                      xyst, dxdy);
	vkk_vgLineBuilder_reset(self);
	return line;
}

uint32_t vkk_vgLineBuilder_count(vkk_vgLineBuilder_t* self)
{
	ASSERT(self);

	return vkk_vgBuffer_count(self->xyd);
}

int
vkk_vgLineBuilder_isDup(vkk_vgLineBuilder_t* self,
                        float x, float y)
{
	ASSERT(self);

	uint32_t count = vkk_vgBuffer_count(self->xyd);
	if(count == 0)
	{
		return 0;
	}

	cc_vec3f_t* pt;
	pt = (cc_vec3f_t*)
	     vkk_vgBuffer_get(self->xyd, count - 1);
	if((pt->x == x) && (pt->y == y))
	{
		return 1;
	}

	return 0;
}

int vkk_vgLineBuilder_point(vkk_vgLineBuilder_t* self,
                            float x, float y)
{
	ASSERT(self);

	if(vkk_vgLineBuilder_isDup(self, x, y))
	{
		return 1;
	}

	cc_vec3f_t* pt;
	pt = (cc_vec3f_t*)
	     vkk_vgBuffer_add3(self->xyd, x, y, 0.0f);
	if(pt == NULL)
	{
		return 0;
	}

	// update dist if multiple points exist
	float    dist  = self->dist;
	uint32_t count = vkk_vgBuffer_count(self->xyd);
	if(count > 1)
	{
		cc_vec2f_t* a;
		a = (cc_vec2f_t*)
		    vkk_vgBuffer_get(self->xyd, count - 2);

		cc_vec2f_t b =
		{
			.x = x,
			.y = y,
		};

		cc_vec2f_subv(&b, a);
		dist += cc_vec2f_mag(&b);
	}

	// update dist
	pt->z      = dist;
	self->dist = dist;

	return 1;
}
