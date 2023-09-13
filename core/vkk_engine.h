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
#include <vulkan/vulkan.h>
#ifdef ANDROID
	#include <android_native_app_glue.h>
	#include "../platform/vkk_platformAndroid.h"
#else
	#include <SDL2/SDL.h>
	#include <SDL2/SDL_vulkan.h>
	#include "../platform/vkk_platformLinux.h"
#endif

#include "../../libcc/cc_jobq.h"
#include "../../libcc/cc_list.h"
#include "../../libcc/cc_map.h"
#include "../vkk.h"
#include "vkk_xferManager.h"
#include "vkk_memory.h"

#define VKK_DESCRIPTOR_POOL_SIZE 64

typedef enum
{
	VKK_OBJECT_TYPE_RENDERER          = 0,
	VKK_OBJECT_TYPE_COMPUTE           = 1,
	VKK_OBJECT_TYPE_BUFFER            = 2,
	VKK_OBJECT_TYPE_IMAGE             = 3,
	VKK_OBJECT_TYPE_UNIFORMSETFACTORY = 4,
	VKK_OBJECT_TYPE_UNIFORMSET        = 5,
	VKK_OBJECT_TYPE_PIPELINELAYOUT    = 6,
	VKK_OBJECT_TYPE_GRAPHICSPIPELINE  = 7,
	VKK_OBJECT_TYPE_COMPUTEPIPELINE   = 8,
} vkk_objectType_e;

#define VKK_OBJECT_TYPE_COUNT 9

typedef struct vkk_object_s
{
	vkk_objectType_e type;

	union
	{
		void*                    obj;
		vkk_renderer_t*          renderer;
		vkk_compute_t*           compute;
		vkk_buffer_t*            buffer;
		vkk_image_t*             image;
		vkk_uniformSetFactory_t* usf;
		vkk_uniformSet_t*        us;
		vkk_pipelineLayout_t*    pl;
		vkk_graphicsPipeline_t*  gp;
		vkk_computePipeline_t*   cp;
	};
} vkk_object_t;

#define VKK_QUEUE_FOREGROUND 0
#define VKK_QUEUE_BACKGROUND 1
#define VKK_QUEUE_COUNT      2

typedef struct vkk_engine_s
{
	vkk_platform_t* platform;

	// SDL window state
	#ifndef ANDROID
		SDL_Window* window;
	#endif

	vkk_version_t version;

	// app info
	char app_name[256];
	vkk_version_t app_version;

	char internal_path[256];
	char external_path[256];

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
	// 4) utility synchronization
	// * shader_modules
	// * samplers
	// 5) renderer/ts synchronization
	// * shutdown and ts_expired
	pthread_mutex_t cmd_mutex;
	pthread_mutex_t usf_mutex;
	pthread_mutex_t utility_mutex;
	pthread_mutex_t renderer_mutex;
	pthread_cond_t  renderer_cond;

	VkInstance       instance;
	VkSurfaceKHR     surface;
	VkPhysicalDevice physical_device;

	// device capabilities
	float    max_anisotropy;
	uint32_t msaa_sample_count;

	// device state
	VkDevice device;
	uint32_t queue_family_index;
	VkQueue  queue[VKK_QUEUE_COUNT];

	// memory manager
	vkk_memoryManager_t* mm;

	// transfer manager
	vkk_xferManager_t* xfer;

	VkPipelineCache pipeline_cache;

	// shaders
	cc_map_t* shader_modules;

	// samplers
	cc_map_t* samplers;

	// image capabilities
	vkk_imageCaps_t image_caps_array[VKK_IMAGE_FORMAT_COUNT];

	// default renderer
	int             shutdown;
	vkk_renderer_t* renderer;

	// jobq(s)
	cc_jobq_t* jobq_destruct;
} vkk_engine_t;

vkk_engine_t* vkk_engine_new(vkk_platform_t* platform,
                             const char* app_name,
                             const char* app_dir,
                             vkk_version_t* app_version,
                             const char* internal_path,
                             const char* external_path);
void          vkk_engine_delete(vkk_engine_t** _self);
void          vkk_engine_shutdown(vkk_engine_t* self);
void          vkk_engine_deviceWaitIdle(vkk_engine_t* self);
int           vkk_engine_recreate(vkk_engine_t* self);

/*
 * engine util function
 */

void             vkk_engine_mipmapImage(vkk_engine_t* self,
                                        vkk_image_t* image,
                                        VkCommandBuffer cb);
uint32_t         vkk_engine_imageCount(vkk_engine_t* self);
int              vkk_engine_queueSubmit(vkk_engine_t* self,
                                        uint32_t queue,
                                        VkCommandBuffer* cb,
                                        uint32_t wait_count,
                                        VkSemaphore* semaphore_wait,
                                        VkSemaphore* semaphore_submit,
                                        VkPipelineStageFlags* wait_dst_stage_mask,
                                        VkFence fence);
void             vkk_engine_queueWaitIdle(vkk_engine_t* self,
                                          uint32_t queue);
int              vkk_engine_allocateDescriptorSetsLocked(vkk_engine_t* self,
                                                         VkDescriptorPool dp,
                                                         const VkDescriptorSetLayout* dsl_array,
                                                         uint32_t ds_count,
                                                         VkDescriptorSet* ds_array);
VkDescriptorPool vkk_engine_newDescriptorPoolLocked(vkk_engine_t* self,
                                                    vkk_uniformSetFactory_t* usf);
void             vkk_engine_attachUniformBuffer(vkk_engine_t* self,
                                                vkk_uniformSet_t* us,
                                                vkk_uniformAttachment_t* ua);
void             vkk_engine_attachUniformSampler(vkk_engine_t* self,
                                                 vkk_uniformSet_t* us,
                                                 vkk_samplerInfo_t* si,
                                                 vkk_image_t* image,
                                                 uint32_t binding);
int              vkk_engine_getMemoryTypeIndex(vkk_engine_t* self,
                                               uint32_t mt_bits,
                                               VkFlags mp_flags,
                                               uint32_t* mt_index);
VkShaderModule   vkk_engine_getShaderModule(vkk_engine_t* self,
                                            const char* fname);
VkSampler*       vkk_engine_getSamplerp(vkk_engine_t* self,
                                        vkk_samplerInfo_t* si);
void             vkk_engine_cmdLock(vkk_engine_t* self);
void             vkk_engine_cmdUnlock(vkk_engine_t* self);
void             vkk_engine_usfLock(vkk_engine_t* self);
void             vkk_engine_usfUnlock(vkk_engine_t* self);
void             vkk_engine_utilityLock(vkk_engine_t* self);
void             vkk_engine_utilityUnlock(vkk_engine_t* self);
void             vkk_engine_rendererLock(vkk_engine_t* self);
void             vkk_engine_rendererUnlock(vkk_engine_t* self);
void             vkk_engine_rendererSignal(vkk_engine_t* self);
void             vkk_engine_rendererWait(vkk_engine_t* self);
int              vkk_engine_rendererCheckTimestamp(vkk_engine_t* self,
                                                   double ts);
void             vkk_engine_rendererWaitForTimestamp(vkk_engine_t* self,
                                                     double ts);
int              vkk_engine_newSurface(vkk_engine_t* self);
void             vkk_engine_deleteSurface(vkk_engine_t* self);
void             vkk_engine_deleteDefaultDepthImage(vkk_engine_t* self,
                                                    vkk_image_t** _image);

void             vkk_engine_deleteObject(vkk_engine_t* self,
                                         vkk_objectType_e type,
                                         void* obj);

#endif
