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

#include <pthread.h>
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

#define VKK_ENGINE_MAX_USF_COUNT 16

typedef struct vkk_buffer_s
{
	double          ts;
	int             update;
	size_t          size;
	VkBuffer*       buffer;
	VkDeviceMemory* memory;
} vkk_buffer_t;

typedef struct vkk_image_s
{
	double         ts;
	uint32_t       width;
	uint32_t       height;
	int            format;
	int            stage;
	uint32_t       mip_levels;
	VkImageLayout* layout_array;
	VkImage        image;
	VkDeviceMemory memory;
	VkImageView    image_view;
} vkk_image_t;

typedef struct vkk_sampler_s
{
	double    ts;
	VkSampler sampler;
} vkk_sampler_t;

typedef struct vkk_uniformSetFactory_s
{
	int                   update;
	uint32_t              ub_count;
	vkk_uniformBinding_t* ub_array;
	uint32_t              ds_available;
	VkDescriptorSetLayout ds_layout;
	cc_list_t*            dp_list;
	cc_list_t*            us_list;
	char                  type_count[VKK_UNIFORM_TYPE_COUNT];
} vkk_uniformSetFactory_t;

typedef struct vkk_uniformSet_s
{
	double                   ts;
	uint32_t                 set;
	uint32_t                 ua_count;
	vkk_uniformAttachment_t* ua_array;
	VkDescriptorSet*         ds_array;
	vkk_uniformSetFactory_t* usf;
} vkk_uniformSet_t;

typedef struct vkk_pipelineLayout_s
{
	uint32_t usf_count;
	VkPipelineLayout pl;
} vkk_pipelineLayout_t;

typedef struct vkk_graphicsPipeline_s
{
	double     ts;
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
	char cache[256];

	// 1) Vulkan synchronization - 2.6. Threading Behavior
	// * The queue parameter in vkQueueSubmit
	//   (renderer_mutex)
	// * The queue parameter in vkQueueWaitIdle
	//   (renderer_mutex)
	// * The descriptorPool the pAllocateInfo parameter in
	//   vkAllocateDescriptorSets (usf_mutex)
	// * The commandPool the pAllocateInfo parameter in
	//   vkAllocateCommandBuffers (cmd_mutex)
	// * The commandPool parameter in vkFreeCommandBuffers
	//   (cmd_mutex)
	// 2) Implicit Externally Synchronized Parameters
	// * All VkQueue objects created from device in
	//   vkDeviceWaitIdle (renderer_mutex)
	// * The VkCommandPool that commandBuffer was allocated
	//   from in vkBeginCommandBuffer, vkEndCommandBuffer,
	//   vkResetCommandBuffer and vkCmdFunctions (cmd_mutex)
	// 3) usf synchronization
	// * ds_available, dp_list and us_list
	// 4) shader module synchronization
	// * shader_modules
	// 5) renderer/ts synchronization
	// * shutdown and ts_expired
	pthread_mutex_t cmd_mutex;
	pthread_mutex_t usf_mutex;
	pthread_mutex_t sm_mutex;
	pthread_mutex_t renderer_mutex;
	pthread_cond_t  renderer_cond;

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

	// shaders
	cc_map_t* shader_modules;

	// image capabilities
	int image_caps_array[VKK_IMAGE_FORMAT_COUNT];

	// default renderer
	int             shutdown;
	vkk_renderer_t* renderer;
} vkk_engine_t;

/*
 * engine util function
 */

void vkk_engine_mipmapImage(vkk_engine_t* self,
                            vkk_image_t* image,
                            VkCommandBuffer cb);

/*
 * engine synchronization
 */

int  vkk_engine_queueSubmit(vkk_engine_t* self,
                            VkCommandBuffer* cb,
                            VkSemaphore* semaphore_acquire,
                            VkSemaphore* semaphore_submit,
                            VkPipelineStageFlags* wait_dst_stage_mask,
                            VkFence fence);
void vkk_engine_queueWaitIdle(vkk_engine_t* self);
int  vkk_engine_allocateDescriptorSetsLocked(vkk_engine_t* self,
                                             VkDescriptorPool dp,
                                             const VkDescriptorSetLayout* dsl_array,
                                             uint32_t ds_count,
                                             VkDescriptorSet* ds_array);
int  vkk_engine_allocateCommandBuffers(vkk_engine_t* self,
                                       int cb_count,
                                       VkCommandBuffer* cb_array);
void vkk_engine_freeCommandBuffers(vkk_engine_t* self,
                                   uint32_t cb_count,
                                   const VkCommandBuffer* cb_array);
void vkk_engine_cmdLock(vkk_engine_t* self);
void vkk_engine_cmdUnlock(vkk_engine_t* self);
void vkk_engine_usfLock(vkk_engine_t* self);
void vkk_engine_usfUnlock(vkk_engine_t* self);
void vkk_engine_smLock(vkk_engine_t* self);
void vkk_engine_smUnlock(vkk_engine_t* self);
void vkk_engine_rendererLock(vkk_engine_t* self);
void vkk_engine_rendererUnlock(vkk_engine_t* self);
void vkk_engine_rendererSignal(vkk_engine_t* self);
void vkk_engine_rendererWait(vkk_engine_t* self);
void vkk_engine_rendererWaitForTimestamp(vkk_engine_t* self,
                                         double ts);

#endif
