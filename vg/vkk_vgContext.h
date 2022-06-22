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

#ifndef vkk_vgContext_H
#define vkk_vgContext_H

#include "../../libcc/cc_list.h"
#include "../../libcc/cc_map.h"
#include "../vkk_vg.h"

typedef struct vkk_vgContext_s
{
	vkk_renderer_t*          rend;
	vkk_uniformSetFactory_t* usf0;
	vkk_uniformSetFactory_t* usf1;
	vkk_uniformSetFactory_t* usf2;
	vkk_pipelineLayout_t*    pl;
	vkk_graphicsPipeline_t*  gp_line;
	vkk_graphicsPipeline_t*  gp_poly;
	vkk_buffer_t*            ub00_mvp;
	vkk_uniformSet_t*        us0;

	// us1: static color buffer
	// us2: non-static dist
	//      static brush12WidthCap reference
	cc_list_t* list_us2[2];
	cc_map_t*  map_us1_color;
	cc_map_t*  map_ub21_brush12WidthCap;
} vkk_vgContext_t;

// protected
int vkk_vgContext_bindLine(vkk_vgContext_t* self,
                           float dist,
                           vkk_vgLineStyle_t* style);
int vkk_vgContext_bindPolygon(vkk_vgContext_t* self,
                              vkk_vgPolygonStyle_t* style);

#endif
