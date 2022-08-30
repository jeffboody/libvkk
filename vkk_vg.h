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

#ifndef vkk_vg_H
#define vkk_vg_H

#include "../libcc/math/cc_mat4f.h"
#include "../libcc/math/cc_vec4f.h"
#include "vkk.h"

/*
 * constants
 */

typedef enum
{
	VKK_VG_LINECAP_SQUARE  = 0,
	VKK_VG_LINECAP_ROUNDED = 1,
} vkk_vgLineCap_e;

/*
 * opaque objects
 */

typedef struct vkk_vgContext_s        vkk_vgContext_t;
typedef struct vkk_vgLineBuilder_s    vkk_vgLineBuilder_t;
typedef struct vkk_vgLine_s           vkk_vgLine_t;
typedef struct vkk_vgPolygonBuilder_s vkk_vgPolygonBuilder_t;
typedef struct vkk_vgPolygon_s        vkk_vgPolygon_t;

/*
 * parameter structures
 */

typedef struct vkk_vgLineStyle_s
{
	cc_vec4f_t      color;
	float           brush1;
	float           brush2;
	float           width;
	vkk_vgLineCap_e cap;
} vkk_vgLineStyle_t;

typedef struct vkk_vgPolygonStyle_s
{
	cc_vec4f_t color;
} vkk_vgPolygonStyle_t;

/*
 * context API
 */

vkk_vgContext_t* vkk_vgContext_new(vkk_renderer_t* rend);
void             vkk_vgContext_delete(vkk_vgContext_t** _self);
void             vkk_vgContext_reset(vkk_vgContext_t* self,
                                     cc_mat4f_t* mvp);
void             vkk_vgContext_bindLines(vkk_vgContext_t* self);
void             vkk_vgContext_bindPolygons(vkk_vgContext_t* self);

/*
 * line builder API
 */

vkk_vgLineBuilder_t* vkk_vgLineBuilder_new(vkk_engine_t* engine);
void                 vkk_vgLineBuilder_delete(vkk_vgLineBuilder_t** _self);
void                 vkk_vgLineBuilder_reset(vkk_vgLineBuilder_t* self);
vkk_vgLine_t*        vkk_vgLineBuilder_build(vkk_vgLineBuilder_t* self);
int                  vkk_vgLineBuilder_isDup(vkk_vgLineBuilder_t* self,
                                             float x,
                                             float y);
uint32_t             vkk_vgLineBuilder_count(vkk_vgLineBuilder_t* self);
int                  vkk_vgLineBuilder_point(vkk_vgLineBuilder_t* self,
                                             float x,
                                             float y);

/*
 * line API
 */

void vkk_vgLine_delete(vkk_vgLine_t** _self);
void vkk_vgLine_draw(vkk_vgLine_t* self,
                     vkk_vgContext_t* ctx,
                     vkk_vgLineStyle_t* style);

/*
 * polygon builder API
 */

vkk_vgPolygonBuilder_t* vkk_vgPolygonBuilder_new(vkk_engine_t* engine);
void                    vkk_vgPolygonBuilder_delete(vkk_vgPolygonBuilder_t** _self);
void                    vkk_vgPolygonBuilder_reset(vkk_vgPolygonBuilder_t* self);
vkk_vgPolygon_t*        vkk_vgPolygonBuilder_build(vkk_vgPolygonBuilder_t* self);
int                     vkk_vgPolygonBuilder_point(vkk_vgPolygonBuilder_t* self,
                                                   int first,
                                                   float x,
                                                   float y);

/*
 * polygon API
 */

void vkk_vgPolygon_delete(vkk_vgPolygon_t** _self);
void vkk_vgPolygon_draw(vkk_vgPolygon_t* self,
                        vkk_vgContext_t* ctx,
                        vkk_vgPolygonStyle_t* style);

#endif