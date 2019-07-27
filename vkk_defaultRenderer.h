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

#include "vkk.h"
#include "vkk_renderer.h"

typedef struct
{
	vkk_renderer_t base;

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

	// framebuffer state
	// one per swapchain image
	VkImageView*   framebuffer_image_views;
	VkFramebuffer* framebuffers;

	// command buffers
	// one per swapchain image
	VkCommandBuffer* cb_array;

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

vkk_renderer_t* vkk_defaultRenderer_new(vkk_engine_t* engine);
void            vkk_defaultRenderer_delete(vkk_renderer_t** _base);
int             vkk_defaultRenderer_resize(vkk_renderer_t* base);
int             vkk_defaultRenderer_begin(vkk_renderer_t* base,
                                          float* clear_color);
void            vkk_defaultRenderer_end(vkk_renderer_t* base);
void            vkk_defaultRenderer_surfaceSize(vkk_renderer_t* base,
                                                uint32_t* _width,
                                                uint32_t* _height);
void            vkk_defaultRenderer_updateBuffer(vkk_renderer_t* base,
                                                 vkk_buffer_t* buffer,
                                                 const void* buf);
void            vkk_defaultRenderer_bindGraphicsPipeline(vkk_renderer_t* base,
                                                         vkk_graphicsPipeline_t* gp);
void            vkk_defaultRenderer_bindUniformSets(vkk_renderer_t* base,
                                                    vkk_pipelineLayout_t* pl,
                                                    uint32_t us_count,
                                                    vkk_uniformSet_t** us_array);
void            vkk_defaultRenderer_clearDepth(vkk_renderer_t* base);
void            vkk_defaultRenderer_viewport(vkk_renderer_t* base,
                                             float x, float y,
                                             float width, float height);
void            vkk_defaultRenderer_scissor(vkk_renderer_t* base,
                                            uint32_t x, uint32_t y,
                                            uint32_t width, uint32_t height);
void            vkk_defaultRenderer_draw(vkk_renderer_t* base,
                                         uint32_t vertex_count,
                                         uint32_t vertex_buffer_count,
                                         vkk_buffer_t** vertex_buffers);
void            vkk_defaultRenderer_drawIndexed(vkk_renderer_t* base,
                                                uint32_t vertex_count,
                                                uint32_t vertex_buffer_count,
                                                int index_type,
                                                vkk_buffer_t* index_buffer,
                                                vkk_buffer_t** vertex_buffers);
VkRenderPass    vkk_defaultRenderer_renderPass(vkk_renderer_t* base);
uint32_t        vkk_defaultRenderer_swapchainImageCount(vkk_renderer_t* base);
double          vkk_defaultRenderer_timestampLocked(vkk_renderer_t* base);

#endif
