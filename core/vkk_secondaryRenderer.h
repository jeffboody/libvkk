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

#ifndef vkk_secondaryRenderer_H
#define vkk_secondaryRenderer_H

#include "../vkk.h"
#include "vkk_commandBuffer.h"
#include "vkk_renderer.h"

typedef struct
{
	vkk_renderer_t base;

	vkk_renderer_t* executor;

	// command buffers
	// one per swapchain image
	vkk_commandBuffer_t* cmd_buffers;

	double ts;
} vkk_secondaryRenderer_t;


/*
 * secondary renderer API
 */

vkk_renderer_t* vkk_secondaryRenderer_new(vkk_renderer_t* executor);
void            vkk_secondaryRenderer_delete(vkk_renderer_t** _base);

/*
 * renderer callback API
 */

int             vkk_secondaryRenderer_begin(vkk_renderer_t* base);
void            vkk_secondaryRenderer_end(vkk_renderer_t* base);
void            vkk_secondaryRenderer_surfaceSize(vkk_renderer_t* base,
                                                uint32_t* _width,
                                                uint32_t* _height);
VkRenderPass    vkk_secondaryRenderer_renderPass(vkk_renderer_t* base);
VkFramebuffer   vkk_secondaryRenderer_framebuffer(vkk_renderer_t* base);
VkCommandBuffer vkk_secondaryRenderer_commandBuffer(vkk_renderer_t* base);

#endif
