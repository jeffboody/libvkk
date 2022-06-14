/*
 * Copyright (c) 2021 Jeff Boody
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

#ifndef vkk_imageStreamRenderer_H
#define vkk_imageStreamRenderer_H

#include "../vkk.h"
#include "vkk_commandBuffer.h"
#include "vkk_renderer.h"

typedef struct
{
	vkk_renderer_t       base;
	vkk_renderer_t*      consumer;
	vkk_image_t**        images;
	VkImageView*         image_views;
	VkRenderPass         render_pass;
	vkk_image_t*         depth_image;
	VkFramebuffer*       framebuffers;
	vkk_commandBuffer_t* cmd_buffers;

	double ts;
} vkk_imageStreamRenderer_t;

/*
 * image renderer API
 */

vkk_renderer_t* vkk_imageStreamRenderer_new(vkk_renderer_t* consumer,
                                            uint32_t width,
                                            uint32_t height,
                                            vkk_imageFormat_e format,
                                            int mipmap,
                                            vkk_stage_e stage);
void            vkk_imageStreamRenderer_delete(vkk_renderer_t** _base);

/*
 * renderer callback API
 */

vkk_image_t*    vkk_imageStreamRenderer_begin(vkk_renderer_t* base,
                                              vkk_rendererMode_e mode,
                                              float* clear_color);
void            vkk_imageStreamRenderer_end(vkk_renderer_t* base);
void            vkk_imageStreamRenderer_surfaceSize(vkk_renderer_t* base,
                                                    uint32_t* _width,
                                                    uint32_t* _height);
VkRenderPass    vkk_imageStreamRenderer_renderPass(vkk_renderer_t* base);
VkFramebuffer   vkk_imageStreamRenderer_framebuffer(vkk_renderer_t* base);
VkCommandBuffer vkk_imageStreamRenderer_commandBuffer(vkk_renderer_t* base);

#endif
