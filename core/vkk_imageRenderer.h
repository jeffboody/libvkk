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

#ifndef vkk_imageRenderer_H
#define vkk_imageRenderer_H

#include "../vkk.h"
#include "vkk_commandBuffer.h"
#include "vkk_renderer.h"

typedef struct
{
	vkk_renderer_t base;

	// target image
	vkk_image_t* dst_image;

	// queue fence
	VkFence fence;

	// render pass state
	VkRenderPass render_pass;

	// depth buffer
	vkk_image_t* depth_image;

	// framebuffer state
	vkk_image_t*  src_image;
	VkFramebuffer framebuffer;

	// command buffer
	vkk_commandBuffer_t* cmd_buffer;
} vkk_imageRenderer_t;

/*
 * image renderer API
 */

vkk_renderer_t* vkk_imageRenderer_new(vkk_engine_t* engine,
                                      uint32_t width,
                                      uint32_t height,
                                      vkk_imageFormat_e format);
void            vkk_imageRenderer_delete(vkk_renderer_t** _base);

/*
 * renderer callback API
 */

int             vkk_imageRenderer_begin(vkk_renderer_t* base,
                                        vkk_rendererMode_e mode,
                                        vkk_image_t* image,
                                        float* clear_color);
void            vkk_imageRenderer_end(vkk_renderer_t* base);
void            vkk_imageRenderer_surfaceSize(vkk_renderer_t* base,
                                              uint32_t* _width,
                                              uint32_t* _height);
VkRenderPass    vkk_imageRenderer_renderPass(vkk_renderer_t* base);
VkFramebuffer   vkk_imageRenderer_framebuffer(vkk_renderer_t* base);
VkCommandBuffer vkk_imageRenderer_commandBuffer(vkk_renderer_t* base);

#endif
