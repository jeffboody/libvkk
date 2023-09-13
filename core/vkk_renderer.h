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

#ifndef vkk_renderer_H
#define vkk_renderer_H

#include <vulkan/vulkan.h>

#include "../vkk.h"

typedef enum
{
	VKK_RENDERER_TYPE_DEFAULT     = 0,
	VKK_RENDERER_TYPE_IMAGE       = 1,
	VKK_RENDERER_TYPE_IMAGESTREAM = 2,
	VKK_RENDERER_TYPE_SECONDARY   = 3,
} vkk_rendererType_e;

#define VKK_RENDERER_TYPE_COUNT 4

typedef struct vkk_renderer_s
{
	vkk_engine_t* engine;

	vkk_rendererType_e type;
	vkk_rendererMode_e mode;

	// currently bound graphics pipeline
	vkk_graphicsPipeline_t* gp;

	// semaphore and flag references are allocated on demand
	// and must be deleted by the renderer which inherits
	// from the base
	uint32_t              wait_count;
	VkSemaphore*          wait_array;
	VkPipelineStageFlags* wait_flags;

	// fps state
	int    fps;
	double fps_t0;
	int    fps_frames;
} vkk_renderer_t;

// protected functions

void            vkk_renderer_init(vkk_renderer_t* self,
                                  vkk_rendererType_e type,
                                  vkk_engine_t* engine);
void            vkk_renderer_addWaitSemaphore(vkk_renderer_t* self,
                                              VkSemaphore semaphore);
VkRenderPass    vkk_renderer_renderPass(vkk_renderer_t* self);
VkFramebuffer   vkk_renderer_framebuffer(vkk_renderer_t* self);
VkCommandBuffer vkk_renderer_commandBuffer(vkk_renderer_t* self);
uint32_t        vkk_renderer_frame(vkk_renderer_t* self);
uint32_t        vkk_renderer_imageCount(vkk_renderer_t* self);
double          vkk_renderer_tsCurrent(vkk_renderer_t* self);

#endif
