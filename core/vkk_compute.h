/*
 * Copyright (c) 2023 Jeff Boody
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

#ifndef vkk_compute_H
#define vkk_compute_H

#include <vulkan/vulkan.h>

#include "../vkk.h"
#include "vkk_commandBuffer.h"

typedef struct vkk_compute_s
{
	vkk_engine_t* engine;

	// currently bound compute pipeline
	vkk_computePipeline_t* cp;

	// command buffer
	vkk_commandBuffer_t* cmd_buffer;

	// queue fence
	VkFence fence;
} vkk_compute_t;

// protected functions

void            vkk_compute_destruct(vkk_compute_t** _self);
VkCommandBuffer vkk_compute_commandBuffer(vkk_compute_t* self);
double          vkk_compute_tsCurrent(vkk_compute_t* self);

#endif
