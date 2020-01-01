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

#include "../libcc/cc_jobq.h"
#include "../libcc/cc_list.h"
#include "../libcc/cc_map.h"
#include "vkk.h"
#include "vkk_imageUploader.h"
#include "vkk_memory.h"

#define VKK_DESCRIPTOR_POOL_SIZE 64

#define VKK_OBJECT_TYPE_RENDERER          0
#define VKK_OBJECT_TYPE_BUFFER            1
#define VKK_OBJECT_TYPE_IMAGE             2
#define VKK_OBJECT_TYPE_SAMPLER           3
#define VKK_OBJECT_TYPE_UNIFORMSETFACTORY 4
#define VKK_OBJECT_TYPE_UNIFORMSET        5
#define VKK_OBJECT_TYPE_PIPELINELAYOUT    6
#define VKK_OBJECT_TYPE_GRAPHICSPIPELINE  7
#define VKK_OBJECT_TYPE_COUNT             8

typedef struct vkk_object_s
{
	int type;

	union
	{
		void*                    obj;
		vkk_renderer_t*          renderer;
		vkk_buffer_t*            buffer;
		vkk_image_t*             image;
		vkk_sampler_t*           sampler;
		vkk_uniformSetFactory_t* usf;
		vkk_uniformSet_t*        us;
		vkk_pipelineLayout_t*    pl;
		vkk_graphicsPipeline_t*  gp;
	};
} vkk_object_t;

typedef struct vkk_engine_s
{
	// window state
	#ifdef ANDROID
		struct android_app* app;
	#else
		SDL_Window* window;
	#endif

	uint32_t version;

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

	// memory manager
	vkk_memoryManager_t* mm;

	// image uploader
	vkk_imageUploader_t* img_uploader;

	VkPipelineCache pipeline_cache;

	// shaders
	cc_map_t* shader_modules;

	// image capabilities
	int image_caps_array[VKK_IMAGE_FORMAT_COUNT];

	// default renderer
	int             shutdown;
	vkk_renderer_t* renderer;

	// jobq(s)
	cc_jobq_t* jobq_destruct;
} vkk_engine_t;

/*
 * engine util function
 */

void             vkk_engine_mipmapImage(vkk_engine_t* self,
                                        vkk_image_t* image,
                                        VkCommandBuffer cb);
int              vkk_engine_uploadImage(vkk_engine_t* self,
                                        vkk_image_t* image,
                                        const void* pixels);
uint32_t         vkk_engine_swapchainImageCount(vkk_engine_t* self);
int              vkk_engine_queueSubmit(vkk_engine_t* self,
                                        VkCommandBuffer* cb,
                                        VkSemaphore* semaphore_acquire,
                                        VkSemaphore* semaphore_submit,
                                        VkPipelineStageFlags* wait_dst_stage_mask,
                                        VkFence fence);
void             vkk_engine_queueWaitIdle(vkk_engine_t* self);
int              vkk_engine_allocateDescriptorSetsLocked(vkk_engine_t* self,
                                                         VkDescriptorPool dp,
                                                         const VkDescriptorSetLayout* dsl_array,
                                                         uint32_t ds_count,
                                                         VkDescriptorSet* ds_array);
VkDescriptorPool vkk_engine_newDescriptorPoolLocked(vkk_engine_t* self,
                                                    vkk_uniformSetFactory_t* usf);
void             vkk_engine_attachUniformBuffer(vkk_engine_t* self,
                                                vkk_uniformSet_t* us,
                                                vkk_buffer_t* buffer,
                                                uint32_t binding);
void             vkk_engine_attachUniformSampler(vkk_engine_t* self,
                                                 vkk_uniformSet_t* us,
                                                 vkk_sampler_t* sampler,
                                                 vkk_image_t* image,
                                                 uint32_t binding);
int              vkk_engine_getMemoryTypeIndex(vkk_engine_t* self,
                                               uint32_t mt_bits,
                                               VkFlags mp_flags,
                                               uint32_t* mt_index);
VkShaderModule   vkk_engine_getShaderModule(vkk_engine_t* self,
                                            const char* fname);
void             vkk_engine_cmdLock(vkk_engine_t* self);
void             vkk_engine_cmdUnlock(vkk_engine_t* self);
void             vkk_engine_usfLock(vkk_engine_t* self);
void             vkk_engine_usfUnlock(vkk_engine_t* self);
void             vkk_engine_smLock(vkk_engine_t* self);
void             vkk_engine_smUnlock(vkk_engine_t* self);
void             vkk_engine_rendererLock(vkk_engine_t* self);
void             vkk_engine_rendererUnlock(vkk_engine_t* self);
void             vkk_engine_rendererSignal(vkk_engine_t* self);
void             vkk_engine_rendererWait(vkk_engine_t* self);
void             vkk_engine_rendererWaitForTimestamp(vkk_engine_t* self,
                                                     double ts);
void             vkk_engine_deleteDefaultDepthImage(vkk_engine_t* self,
                                                    vkk_image_t** _image);

void             vkk_engine_deleteObject(vkk_engine_t* self, int type,
                                         void* obj);

#endif
