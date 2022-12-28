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

#ifndef vkk_vgRenderer_H
#define vkk_vgRenderer_H

#include "../../libcc/cc_list.h"
#include "../../libcc/cc_map.h"
#include "../vkk_vg.h"

typedef struct vkk_vgRendererImageState_s vkk_vgRendererImageState_t;

typedef struct vkk_vgRenderer_s
{
	vkk_renderer_t*          rend;
	vkk_uniformSetFactory_t* usf0;
	vkk_uniformSetFactory_t* usf1;
	vkk_uniformSetFactory_t* usf2_line;
	vkk_uniformSetFactory_t* usf2_poly;
	vkk_uniformSetFactory_t* usf3_line;
	vkk_pipelineLayout_t*    pl_line;
	vkk_pipelineLayout_t*    pl_poly;
	vkk_graphicsPipeline_t*  gp_line;
	vkk_graphicsPipeline_t*  gp_poly;

	// primitive pm
	// updated once per frame
	// layout(std140, set=0, binding=0) uniform uniformPm
	vkk_buffer_t*     ub00_pm;
	vkk_uniformSet_t* us0;

	// primitive mvm
	// updated zero or more times per frame
	// default identity matrix is static
	// layout(std140, set=1, binding=0) uniform uniformMvm
	vkk_buffer_t*     ub10_mvm_identity;
	vkk_uniformSet_t* us1;
	cc_list_t*        list_usb1[2];
	cc_list_t*        stack_usb1; // references

	// static line style
	// layout(std140, set=2, binding=0) uniform uniformColor
	// layout(std140, set=2, binding=1) uniform uniformBrush12WidthCap
	cc_map_t* map_usb2_line;

	// static poly style
	// layout(std140, set=2, binding=0) uniform uniformColor
	cc_map_t* map_usb2_poly;

	// map from sampler info to dynamic image state
	vkk_vgRendererImageState_t* image_state;
	cc_map_t*                   map_image;

	// line state updated per draw
	// layout(std140, set=3, binding=0) uniform uniformDist
	cc_list_t* list_usb3_line[2];

	// static image state
	int image_yup;
	vkk_buffer_t* vb_xyuv_image[2];
} vkk_vgRenderer_t;

#endif
