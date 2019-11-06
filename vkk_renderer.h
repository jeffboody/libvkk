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
(*vkk_renderer_beginDefaultFn)(vkk_renderer_t* self,
                               float* clear_color);
typedef int
(*vkk_renderer_beginOffscreenFn)(vkk_renderer_t* self,
                                 vkk_image_t* image,
                                 float* clear_color);
typedef void
(*vkk_renderer_endFn)(vkk_renderer_t* self);
typedef void
(*vkk_renderer_surfaceSizeFn)(vkk_renderer_t* self,
                              uint32_t* _width,
                              uint32_t* _height);
typedef VkRenderPass
(*vkk_renderer_renderPassFn)(vkk_renderer_t* self);
typedef VkCommandBuffer
(*vkk_renderer_commandBufferFn)(vkk_renderer_t* self);
typedef uint32_t
(*vkk_renderer_swapchainFrameFn)(vkk_renderer_t* self);

typedef struct vkk_renderer_s
{
	vkk_engine_t* engine;
} vkk_renderer_t;

void         vkk_renderer_init(vkk_renderer_t* self,
                               vkk_engine_t* engine);
VkRenderPass vkk_renderer_renderPass(vkk_renderer_t* self);

#endif
