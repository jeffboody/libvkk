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

#ifndef vkk_defaultRenderer_H
#define vkk_defaultRenderer_H

#include "../vkk.h"
#include "vkk_commandBuffer.h"
#include "vkk_memory.h"
#include "vkk_renderer.h"

typedef struct
{
	vkk_renderer_t base;

	int resize;

	// swapchain state
	uint32_t        swapchain_frame;
	VkFormat        swapchain_format;
	VkExtent2D      swapchain_extent;
	VkColorSpaceKHR swapchain_color_space;
	uint32_t        swapchain_image_count;
	VkSwapchainKHR  swapchain;
	VkImage*        swapchain_images;
	VkFence*        swapchain_fences;

	// render pass state
	VkRenderPass render_pass;

	// depth buffer
	vkk_image_t* depth_image;

	// msaa buffer
	VkImage       msaa_image;
	vkk_memory_t* msaa_memory;
	VkImageView   msaa_image_view;

	// framebuffer state
	// one per swapchain image
	VkImageView*   framebuffer_image_views;
	VkFramebuffer* framebuffers;

	// command buffers
	// one per swapchain image
	vkk_commandBuffer_t* cmd_buffers;

	// GPU timestamps
	// ts_expired protected by renderer_mutex
	double* ts_array;
	double  ts_expired;

	// synchronization
	// one per swapchain image
	uint32_t     semaphore_index;
	VkSemaphore* semaphore_acquire;
	VkSemaphore* semaphore_submit;
} vkk_defaultRenderer_t;


/*
 * default renderer API
 */

vkk_renderer_t* vkk_defaultRenderer_new(vkk_engine_t* engine);
void            vkk_defaultRenderer_delete(vkk_renderer_t** _base);
int             vkk_defaultRenderer_resize(vkk_renderer_t* base);
int             vkk_defaultRenderer_recreate(vkk_renderer_t* base);
double          vkk_defaultRenderer_tsCurrent(vkk_renderer_t* base);
double          vkk_defaultRenderer_tsExpiredLocked(vkk_renderer_t* base);
void            vkk_defaultRenderer_deviceWaitIdle(vkk_renderer_t* base);

/*
 * renderer callback API
 */

int             vkk_defaultRenderer_begin(vkk_renderer_t* base,
                                          vkk_rendererMode_e mode,
                                          float* clear_color);
void            vkk_defaultRenderer_end(vkk_renderer_t* base);
void            vkk_defaultRenderer_surfaceSize(vkk_renderer_t* base,
                                                uint32_t* _width,
                                                uint32_t* _height);
VkRenderPass    vkk_defaultRenderer_renderPass(vkk_renderer_t* base);
VkFramebuffer   vkk_defaultRenderer_framebuffer(vkk_renderer_t* base);
VkCommandBuffer vkk_defaultRenderer_commandBuffer(vkk_renderer_t* base);
uint32_t        vkk_defaultRenderer_frame(vkk_renderer_t* base);
uint32_t        vkk_defaultRenderer_imageCount(vkk_renderer_t* base);

#endif
