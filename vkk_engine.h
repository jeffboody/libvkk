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

#define VKK_BUFFER_USAGE_UNIFORM 0
#define VKK_BUFFER_USAGE_VERTEX  1
#define VKK_BUFFER_USAGE_COUNT   2

#define VKK_IMAGE_FORMAT_RGBA4444 0
#define VKK_IMAGE_FORMAT_RGB565   1
#define VKK_IMAGE_FORMAT_RGBA5551 2
#define VKK_IMAGE_FORMAT_R8       3
#define VKK_IMAGE_FORMAT_RG88     4
#define VKK_IMAGE_FORMAT_RGB888   5
#define VKK_IMAGE_FORMAT_RGBA8888 6
#define VKK_IMAGE_FORMAT_DEPTH    7
#define VKK_IMAGE_FORMAT_COUNT    8

#define VKK_INDEX_TYPE_USHORT 0
#define VKK_INDEX_TYPE_UINT   1
#define VKK_INDEX_TYPE_COUNT  2

#define VKK_PRIMITIVE_TRIANGLE_LIST  0
#define VKK_PRIMITIVE_TRIANGLE_STRIP 1
#define VKK_PRIMITIVE_TRIANGLE_FAN   2
#define VKK_PRIMITIVE_TRIANGLE_COUNT 3

#define VKK_STAGE_VS   1
#define VKK_STAGE_FS   2
#define VKK_STAGE_VSFS 3

#define VKK_UNIFORM_TYPE_BUFFER  0
#define VKK_UNIFORM_TYPE_SAMPLER 1
#define VKK_UNIFORM_TYPE_COUNT   2

#define VKK_VERTEX_FORMAT_FLOAT 0
#define VKK_VERTEX_FORMAT_INT   1
#define VKK_VERTEX_FORMAT_SHORT 2
#define VKK_VERTEX_FORMAT_COUNT 3

typedef struct
{
	int             dynamic;
	size_t          size;
	VkBuffer*       buffer;
	VkDeviceMemory* memory;
} vkk_buffer_t;

typedef struct
{
	uint32_t       width;
	uint32_t       height;
	int            format;
	int            transition;
	VkImage        image;
	VkDeviceMemory memory;
	VkImageView    image_view;
} vkk_image_t;

typedef struct
{
	VkSampler sampler;
} vkk_sampler_t;

typedef struct
{
	uint32_t       binding;
	int            type;
	int            stage;
	vkk_sampler_t* sampler;
} vkk_uniformBinding_t;

typedef struct
{
	int                   dynamic;
	uint32_t              ds_available;
	VkDescriptorSetLayout ds_layout;
	cc_list_t*            dp_list;
	cc_list_t*            us_list;
	char                  type_count[VKK_UNIFORM_TYPE_COUNT];
} vkk_uniformSetFactory_t;

typedef struct
{
	VkDescriptorSet*         ds_array;
	vkk_uniformSetFactory_t* usf;
} vkk_uniformSet_t;

typedef struct
{
	VkPipelineLayout pl;
} vkk_pipelineLayout_t;

typedef struct
{
	uint32_t location;
	uint32_t components;
	int      format;
} vkk_vertexBufferInfo_t;

typedef struct
{
	vkk_pipelineLayout_t*   pl;
	const char*             vs;
	const char*             fs;
	uint32_t                vb_count;
	vkk_vertexBufferInfo_t* vbi;
	int                     primitive;
	int                     primitive_restart;
	int                     cull_back;
	int                     depth_test;
	int                     depth_write;
	int                     blend_mode;
} vkk_graphicsPipelineInfo_t;

typedef struct
{
	VkPipeline pipeline;
} vkk_graphicsPipeline_t;

typedef struct
{
	// window state
	#ifdef ANDROID
		struct android_app* app;
	#else
		SDL_Window* window;
	#endif

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

vkk_engine_t*            vkk_engine_new(void* app,
                                        const char* app_name,
                                        uint32_t app_version);
void                     vkk_engine_delete(vkk_engine_t** _self);
void                     vkk_engine_waitForIdle(vkk_engine_t* self);
int                      vkk_engine_resize(vkk_engine_t* self,
                                           uint32_t* _width,
                                           uint32_t* _height);
int                      vkk_engine_beginFrame(vkk_engine_t* self,
                                               float* clear_color);
void                     vkk_engine_endFrame(vkk_engine_t* self);
vkk_buffer_t*            vkk_engine_newBuffer(vkk_engine_t* self,
                                              int dynamic,
                                              int usage,
                                              size_t size,
                                              const void* buf);
void                     vkk_engine_deleteBuffer(vkk_engine_t* self,
                                                 vkk_buffer_t** _buffer);
void                     vkk_engine_updateBuffer(vkk_engine_t* self,
                                                 vkk_buffer_t* buffer,
                                                 const void* buf);
vkk_image_t*             vkk_engine_newImage(vkk_engine_t* self,
                                             uint32_t width,
                                             uint32_t height,
                                             int format,
                                             int mipmap,
                                             const void* pixels);
void                     vkk_engine_deleteImage(vkk_engine_t* self,
                                                vkk_image_t** _image);
vkk_uniformSetFactory_t* vkk_engine_newUniformSetFactory(vkk_engine_t* self,
                                                         int dynamic,
                                                         uint32_t count,
                                                         vkk_uniformBinding_t* ub_array);
void                     vkk_engine_deleteUniformSetFactory(vkk_engine_t* self,
                                                            vkk_uniformSetFactory_t** _usf);
vkk_uniformSet_t*        vkk_engine_newUniformSet(vkk_engine_t* self,
                                                  vkk_uniformSetFactory_t* usf);
void                     vkk_engine_deleteUniformSet(vkk_engine_t* self,
                                                     vkk_uniformSet_t** _us);
void                     vkk_engine_attachUniformBuffer(vkk_engine_t* self,
                                                        vkk_uniformSet_t* us,
                                                        vkk_buffer_t* buffer,
                                                        uint32_t binding);
void                     vkk_engine_bindUniformSet(vkk_engine_t* self,
                                                   vkk_pipelineLayout_t* pl,
                                                   vkk_uniformSet_t* us);
vkk_pipelineLayout_t*    vkk_engine_newPipelineLayout(vkk_engine_t* self,
                                                      uint32_t usf_count,
                                                      vkk_uniformSetFactory_t** usf_array);
void                     vkk_engine_deletePipelineLayout(vkk_engine_t* self,
                                                         vkk_pipelineLayout_t** _pl);
vkk_graphicsPipeline_t*  vkk_engine_newGraphicsPipeline(vkk_engine_t* self,
                                                        vkk_graphicsPipelineInfo_t* gpi);
void                     vkk_engine_deleteGraphicsPipeline(vkk_engine_t* self,
                                                           vkk_graphicsPipeline_t** _gp);
void                     vkk_engine_bindGraphicsPipeline(vkk_engine_t* self,
                                                         vkk_graphicsPipeline_t* gp);
void                     vkk_engine_draw(vkk_engine_t* self,
                                         uint32_t vertex_count,
                                         uint32_t vertex_buffer_count,
                                         vkk_buffer_t** vertex_buffers);
void                     vkk_engine_drawIndexed(vkk_engine_t* self,
                                                uint32_t vertex_count,
                                                uint32_t vertex_buffer_count,
                                                int index_type,
                                                vkk_buffer_t* index_buffer,
                                                vkk_buffer_t** vertex_buffers);

#endif
