/*
 * Copyright (c) 2019 Jeff Boody
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

#ifndef vkk_commandBuffer_H
#define vkk_commandBuffer_H

#include <vulkan/vulkan.h>

#include "../vkk.h"
#include "vkk_renderer.h"

typedef struct vkk_commandBuffer_s
{
	vkk_engine_t* engine;

	VkCommandPool command_pool;

	uint32_t         cb_count;
	VkCommandBuffer* cb_array;
} vkk_commandBuffer_t;

vkk_commandBuffer_t* vkk_commandBuffer_new(vkk_engine_t* engine,
                                           uint32_t cb_count,
                                           int secondary);
void                 vkk_commandBuffer_delete(vkk_commandBuffer_t** _self);
VkCommandBuffer      vkk_commandBuffer_get(vkk_commandBuffer_t* self,
                                           uint32_t index);

#endif
