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

#ifndef vkk_engine_H
#define vkk_engine_H

#ifdef ANDROID
	#include <vulkan_wrapper.h>
	#include <android_native_app_glue.h>
#else
	#include <SDL2/SDL.h>
	#include <SDL2/SDL_vulkan.h>
	#include <vulkan/vulkan.h>
#endif

#include "../libcc/cc_list.h"
#include "../libcc/cc_map.h"
#include "vkk.h"

typedef struct vkk_buffer_s
{
	int             dynamic;
	size_t          size;
	VkBuffer*       buffer;
	VkDeviceMemory* memory;
} vkk_buffer_t;

typedef struct vkk_image_s
{
	uint32_t       width;
	uint32_t       height;
	int            format;
	int            transition;
	VkImage        image;
	VkDeviceMemory memory;
	VkImageView    image_view;
} vkk_image_t;

typedef struct vkk_sampler_s
{
	VkSampler sampler;
} vkk_sampler_t;

typedef struct vkk_uniformSetFactory_s
{
	int                   dynamic;
	uint32_t              ds_available;
	VkDescriptorSetLayout ds_layout;
	cc_list_t*            dp_list;
	cc_list_t*            us_list;
	char                  type_count[VKK_UNIFORM_TYPE_COUNT];
} vkk_uniformSetFactory_t;

typedef struct vkk_uniformSet_s
{
	VkDescriptorSet*         ds_array;
	vkk_uniformSetFactory_t* usf;
} vkk_uniformSet_t;

typedef struct vkk_pipelineLayout_s
{
	VkPipelineLayout pl;
} vkk_pipelineLayout_t;

typedef struct vkk_graphicsPipeline_s
{
	VkPipeline pipeline;
} vkk_graphicsPipeline_t;

typedef struct vkk_engine_s
{
	// window state
	#ifdef ANDROID
		struct android_app* app;
	#else
		SDL_Window* window;
	#endif

	char resource[256];

	VkInstance       instance;
	VkSurfaceKHR     surface;
	VkPhysicalDevice physical_device;

	// device state
	VkDevice device;
	uint32_t queue_family_index;
	VkQueue  queue;

	// cache and pool state (optimizers)
	VkPipelineCache pipeline_cache;
	VkCommandPool   command_pool;

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
	VkCommandBuffer* command_buffers;

	// synchronization
	// one per swapchain image
	uint32_t     semaphore_index;
	VkSemaphore* semaphore_acquire;
	VkSemaphore* semaphore_submit;

	cc_map_t* shader_modules;
} vkk_engine_t;

#endif
