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

#ifdef ANDROID
	#include <vulkan_wrapper.h>
#else
	#include <vulkan/vulkan.h>
#endif

#include "vkk.h"

// function pointers
typedef int
(*vkk_renderer_beginFn)(vkk_renderer_t* self,
                        float* clear_color);
typedef void
(*vkk_renderer_endFn)(vkk_renderer_t* self);
typedef void
(*vkk_renderer_surfaceSizeFn)(vkk_renderer_t* self,
                              uint32_t* _width,
                              uint32_t* _height);
typedef void
(*vkk_renderer_updateBufferFn)(vkk_renderer_t* self,
                               vkk_buffer_t* buffer,
                               const void* buf);
typedef void
(*vkk_renderer_bindGraphicsPipelineFn)(vkk_renderer_t* self,
                                       vkk_graphicsPipeline_t* gp);
typedef void
(*vkk_renderer_bindUniformSetsFn)(vkk_renderer_t* self,
                                  vkk_pipelineLayout_t* pl,
                                  uint32_t us_count,
                                  vkk_uniformSet_t** us_array);
typedef void
(*vkk_renderer_clearDepthFn)(vkk_renderer_t* self);
typedef void
(*vkk_renderer_viewportFn)(vkk_renderer_t* self,
                           float x, float y,
                           float width, float height);
typedef void
(*vkk_renderer_scissorFn)(vkk_renderer_t* self,
                          uint32_t x, uint32_t y,
                          uint32_t width, uint32_t height);
typedef void
(*vkk_renderer_drawFn)(vkk_renderer_t* self,
                       uint32_t vertex_count,
                       uint32_t vertex_buffer_count,
                       vkk_buffer_t** vertex_buffers);
typedef void
(*vkk_renderer_drawIndexedFn)(vkk_renderer_t* self,
                              uint32_t vertex_count,
                              uint32_t vertex_buffer_count,
                              int index_type,
                              vkk_buffer_t* index_buffer,
                              vkk_buffer_t** vertex_buffers);
typedef VkRenderPass
(*vkk_renderer_renderPassFn)(vkk_renderer_t* self);
typedef uint32_t
(*vkk_renderer_swapchainImageCountFn)(vkk_renderer_t* self);

typedef struct vkk_renderer_s
{
	vkk_engine_t* engine;

	// function pointers
	vkk_renderer_beginFn                beginFn;
	vkk_renderer_endFn                  endFn;
	vkk_renderer_surfaceSizeFn          surfaceSizeFn;
	vkk_renderer_updateBufferFn         updateBufferFn;
	vkk_renderer_bindGraphicsPipelineFn bindGraphicsPipelineFn;
	vkk_renderer_bindUniformSetsFn      bindUniformSetsFn;
	vkk_renderer_clearDepthFn           clearDepthFn;
	vkk_renderer_viewportFn             viewportFn;
	vkk_renderer_scissorFn              scissorFn;
	vkk_renderer_drawFn                 drawFn;
	vkk_renderer_drawIndexedFn          drawIndexedFn;
	vkk_renderer_renderPassFn           renderPassFn;
	vkk_renderer_swapchainImageCountFn  swapchainImageCountFn;
} vkk_renderer_t;

void         vkk_renderer_init(vkk_renderer_t* self,
                               vkk_engine_t* engine,
                               vkk_renderer_beginFn beginFn,
                               vkk_renderer_endFn endFn,
                               vkk_renderer_surfaceSizeFn surfaceSizeFn,
                               vkk_renderer_updateBufferFn updateBufferFn,
                               vkk_renderer_bindGraphicsPipelineFn bindGraphicsPipelineFn,
                               vkk_renderer_bindUniformSetsFn bindUniformSetsFn,
                               vkk_renderer_clearDepthFn clearDepthFn,
                               vkk_renderer_viewportFn viewportFn,
                               vkk_renderer_scissorFn scissorFn,
                               vkk_renderer_drawFn drawFn,
                               vkk_renderer_drawIndexedFn drawIndexedFn,
                               vkk_renderer_renderPassFn renderPassFn,
                               vkk_renderer_swapchainImageCountFn swapchainImageCountFn);
VkRenderPass vkk_renderer_renderPass(vkk_renderer_t* self);
uint32_t     vkk_renderer_swapchainImageCount(vkk_renderer_t* self);

#endif
