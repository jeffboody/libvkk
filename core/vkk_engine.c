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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LOG_TAG "vkk"
#include "../../libcc/cc_log.h"
#include "../../libcc/cc_memory.h"
#include "../../libbfs/bfs_file.h"
#include "../platform/vkk_platformCmdInfo.h"
#include "../vkk.h"
#include "../vkk_platform.h"
#include "vkk_buffer.h"
#include "vkk_commandBuffer.h"
#include "vkk_computePipeline.h"
#include "vkk_compute.h"
#include "vkk_defaultRenderer.h"
#include "vkk_engine.h"
#include "vkk_graphicsPipeline.h"
#include "vkk_imageRenderer.h"
#include "vkk_imageStreamRenderer.h"
#include "vkk_image.h"
#include "vkk_memoryManager.h"
#include "vkk_pipelineLayout.h"
#include "vkk_secondaryRenderer.h"
#include "vkk_uniformSet.h"
#include "vkk_uniformSetFactory.h"
#include "vkk_util.h"

/***********************************************************
* private                                                  *
***********************************************************/

static int
vkk_engine_hasDeviceExtensions(vkk_engine_t* self,
                               uint32_t count,
                               const char** names)
{
	ASSERT(self);
	ASSERT(count > 0);
	ASSERT(names);

	uint32_t pCount = 0;
	if(vkEnumerateDeviceExtensionProperties(self->physical_device, NULL,
	                                        &pCount, NULL) != VK_SUCCESS)
	{
		LOGE("vkEnumerateDeviceExtensionProperties failed");
		return 0;
	}

	VkExtensionProperties* properties;
	properties = (VkExtensionProperties*)
	             CALLOC(pCount, sizeof(VkExtensionProperties));
	if(properties == NULL)
	{
		LOGE("CALLOC failed");
		return 0;
	}

	if(vkEnumerateDeviceExtensionProperties(self->physical_device, NULL,
	                                        &pCount, properties) != VK_SUCCESS)
	{
		LOGE("vkEnumerateDeviceExtensionProperties failed");
		goto fail_properties;
	}

	// check for enabled extensions
	int i;
	for(i = 0; i < count; ++i)
	{
		int found = 0;
		int j;
		for(j = 0; j < pCount; ++j)
		{
			if(strcmp(names[i],
			          properties[j].extensionName) == 0)
			{
				found = 1;
				break;
			}
		}

		if(found == 0)
		{
			LOGE("%s not found", names[i]);
			goto fail_enabled;
		}
	}

	FREE(properties);

	// success
	return 1;

	// failure
	fail_enabled:
	fail_properties:
		FREE(properties);
	return 0;
}

static int
vkk_engine_noDisplay(void)
{
	#ifndef ANDROID
		vkk_platformOnMain_fn onMain;
		onMain = VKK_PLATFORM_INFO.onMain;
		if(onMain)
		{
			return 1;
		}
	#endif

	return 0;
}

#ifndef ANDROID

static int
vkk_engine_initSDL(vkk_engine_t* self, const char* app_name)
{
	ASSERT(self);
	ASSERT(app_name);

	if(vkk_engine_noDisplay())
	{
		// skip SDL
		return 1;
	}

	SDL_version version;
	SDL_VERSION(&version);

	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK) < 0)
	{
		LOGE("SDL_Init failed %s", SDL_GetError());
		return 0;
	}

	if(SDL_GetNumVideoDisplays() <= 0)
	{
		LOGE("SDL_GetNumVideoDisplays failed %s",
		     SDL_GetError());
		return 0;
	}

	// set the default screen size
	int width      = 1920;
	int height     = 1080;
	int fullscreen = 1;
	SDL_DisplayMode dpy;
	if(SDL_GetCurrentDisplayMode(0, &dpy) == 0)
	{
		width  = dpy.w;
		height = dpy.h;
	}

	// override the default screen size
	FILE* f = fopen("sdl.cfg", "r");
	if(f)
	{
		float density;
		if(fscanf(f, "%i %i %f %i",
		          &width, &height, &density, &fullscreen) != 4)
		{
			LOGW("fscanf failed");
		}
		fclose(f);
	}

	int flags = (fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) |
		        SDL_WINDOW_RESIZABLE |
	            SDL_WINDOW_VULKAN    |
	            SDL_WINDOW_SHOWN;
	self->window = SDL_CreateWindow(app_name,
	                                SDL_WINDOWPOS_UNDEFINED,
	                                SDL_WINDOWPOS_UNDEFINED,
	                                width, height, flags);
	if(self->window == NULL)
	{
		LOGE("SDL_CreateWindow failed: %s", SDL_GetError());
		goto fail_window;
	}

	// success
	return 1;

	// failure
	fail_window:
		SDL_Quit();
	return 0;
}

static void
vkk_engine_shutdownSDL(vkk_engine_t* self)
{
	if(vkk_engine_noDisplay())
	{
		// skip SDL
		return;
	}

	SDL_DestroyWindow(self->window);
	SDL_Quit();
}
#endif

static int
vkk_engine_newInstance(vkk_engine_t* self,
                       const char* app_name,
                       vkk_version_t* app_version)
{
	ASSERT(self);
	ASSERT(app_name);
	ASSERT(app_version);

	uint32_t    extension_count   = 2;
	const char* extension_names[] =
	{
		"VK_KHR_surface",
		#ifdef ANDROID
			"VK_KHR_android_surface"
		#else
			"VK_KHR_xlib_surface"
		#endif
	};

	uint32_t av = VK_MAKE_VERSION(app_version->major,
	                              app_version->minor,
	                              app_version->patch);
	uint32_t ev = VK_MAKE_VERSION(self->version.major,
	                              self->version.minor,
	                              self->version.patch);
	VkApplicationInfo a_info =
	{
		.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext              = NULL,
		.pApplicationName   = app_name,
		.applicationVersion = av,
		.pEngineName        = "vkk",
		.engineVersion      = ev,
		.apiVersion         = VK_MAKE_VERSION(1,0,0)
	};

	#if defined(DEBUG_LAYERS1)
		uint32_t layer_count = 1;
		const char* layer_names[] =
		{
			"VK_LAYER_KHRONOS_validation"
		};
	#elif defined(DEBUG_LAYERS5)
		uint32_t layer_count = 5;
		const char* layer_names[] =
		{
			"VK_LAYER_LUNARG_core_validation",
			"VK_LAYER_GOOGLE_unique_objects",
			"VK_LAYER_LUNARG_object_tracker",
			"VK_LAYER_LUNARG_parameter_validation",
			"VK_LAYER_GOOGLE_threading"
		};
	#elif defined(DEBUG_DUMP)
		uint32_t layer_count = 1;
		const char* layer_names[] =
		{
			"VK_LAYER_LUNARG_api_dump"
		};
	#else
		uint32_t layer_count = 0;
		const char** layer_names = NULL;
	#endif
	VkInstanceCreateInfo ic_info =
	{
		.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext                   = NULL,
		.flags                   = 0,
		.pApplicationInfo        = &a_info,
		.enabledLayerCount       = layer_count,
		.ppEnabledLayerNames     = layer_names,
		.enabledExtensionCount   = extension_count,
		.ppEnabledExtensionNames = extension_names
	};

	if(vkCreateInstance(&ic_info, NULL,
	                    &self->instance) != VK_SUCCESS)
	{
		LOGE("vkCreateInstance failed");
		return 0;
	}

	return 1;
}

static int vkk_engine_getPhysicalDevice(vkk_engine_t* self)
{
	ASSERT(self);

	uint32_t physical_device_count;
	if(vkEnumeratePhysicalDevices(self->instance,
	                              &physical_device_count,
	                              NULL) != VK_SUCCESS)
	{
		LOGE("vkEnumeratePhysicalDevices failed");
		return 0;
	}

	if(physical_device_count < 1)
	{
		LOGE("physical_device_count=%u", physical_device_count);
		return 0;
	}

	// select the first physical device
	VkResult result;
	physical_device_count = 1;
	result = vkEnumeratePhysicalDevices(self->instance,
	                                    &physical_device_count,
	                                    &self->physical_device);
	if((result == VK_SUCCESS) || (result == VK_INCOMPLETE))
	{
		// ok
	}
	else
	{
		LOGE("vkEnumeratePhysicalDevices failed");
		return 0;
	}

	VkFormat format[4*VKK_VERTEX_FORMAT_COUNT] =
	{
		VK_FORMAT_R32_SFLOAT,
		VK_FORMAT_R32G32_SFLOAT,
		VK_FORMAT_R32G32B32_SFLOAT,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_FORMAT_R32_SINT,
		VK_FORMAT_R32G32_SINT,
		VK_FORMAT_R32G32B32_SINT,
		VK_FORMAT_R32G32B32A32_SINT,
		VK_FORMAT_R16_SINT,
		VK_FORMAT_R16G16_SINT,
		VK_FORMAT_R16G16B16_SINT,
		VK_FORMAT_R16G16B16A16_SINT,
		VK_FORMAT_R32_UINT,
		VK_FORMAT_R32G32_UINT,
		VK_FORMAT_R32G32B32_UINT,
		VK_FORMAT_R32G32B32A32_UINT,
		VK_FORMAT_R16_UINT,
		VK_FORMAT_R16G16_UINT,
		VK_FORMAT_R16G16B16_UINT,
		VK_FORMAT_R16G16B16A16_UINT,
	};

	// warn if vertex formats are unsupported
	int i;
	VkFormatProperties fp;
	for(i = 0; i < 4*VKK_VERTEX_FORMAT_COUNT; ++i)
	{
		vkGetPhysicalDeviceFormatProperties(self->physical_device,
			                                format[i], &fp);
		if((fp.bufferFeatures &
		    VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT) == 0)
		{
			LOGW("unsupported format=%u", (uint32_t) format[i]);
		}
	}

	// query device capabilities
	VkPhysicalDeviceFeatures   pdf;
	VkPhysicalDeviceProperties pdp;
	vkGetPhysicalDeviceFeatures(self->physical_device, &pdf);
	vkGetPhysicalDeviceProperties(self->physical_device, &pdp);
	if(pdf.samplerAnisotropy)
	{
		self->max_anisotropy = pdp.limits.maxSamplerAnisotropy;
	}

	// query MSAA sample count
	#ifdef VKK_ENGINE_DISABLE_MSAA
		self->msaa_sample_count = 1;
	#else
		VkSampleCountFlags scf;
		scf = pdp.limits.framebufferColorSampleCounts &
		      pdp.limits.framebufferDepthSampleCounts;
		if(scf & VK_SAMPLE_COUNT_4_BIT)
		{
			self->msaa_sample_count = 4;
		}
		else
		{
			self->msaa_sample_count = 1;
		}
	#endif

	return 1;
}

static int vkk_engine_newDevice(vkk_engine_t* self)
{
	ASSERT(self);

	uint32_t    extension_count   = 1;
	const char* extension_names[] =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	if(vkk_engine_hasDeviceExtensions(self,
	                                  extension_count,
	                                  extension_names) == 0)
	{
		return 0;
	}

	uint32_t qfp_count;
	vkGetPhysicalDeviceQueueFamilyProperties(self->physical_device,
	                                         &qfp_count,
	                                         NULL);

	VkQueueFamilyProperties* qfp;
	qfp = (VkQueueFamilyProperties*)
	      CALLOC(qfp_count,
	             sizeof(VkQueueFamilyProperties));
	if(qfp == NULL)
	{
		LOGE("CALLOC failed");
		return 0;
	}

	vkGetPhysicalDeviceQueueFamilyProperties(self->physical_device,
	                                         &qfp_count,
	                                         qfp);

	int      i;
	int      has_index   = 0;
	int      queue_count = 1;
	VkBool32 supported   = VK_TRUE;
	self->queue_family_index = 0;
	for(i = 0; i < qfp_count; ++i)
	{
		if((qfp[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
		   (qfp[i].queueFlags & VK_QUEUE_COMPUTE_BIT))
		{
			if(vkk_engine_noDisplay())
			{
				// skip surface check
			}
			else
			{
				if(vkGetPhysicalDeviceSurfaceSupportKHR(self->physical_device,
				                                        i, self->surface,
				                                        &supported) != VK_SUCCESS)
				{
					LOGW("vkGetPhysicalDeviceSurfaceSupportKHR failed");
					continue;
				}
			}

			if(supported && (has_index == 0))
			{
				// select the first supported device queue
				self->queue_family_index = i;
				has_index = 1;

				if(qfp[i].queueCount > 1)
				{
					queue_count = 2;
				}

				break;
			}
		}
	}

	if(has_index == 0)
	{
		LOGE("vkGetPhysicalDeviceQueueFamilyProperties failed");
		goto fail_select_qfp;
	}

	float queue_priority[2] =
	{
		1.0f, 0.5f
	};
	VkDeviceQueueCreateInfo dqc_info =
	{
		.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext            = NULL,
		.flags            = 0,
		.queueFamilyIndex = self->queue_family_index,
		.queueCount       = queue_count,
		.pQueuePriorities = queue_priority
	};

	VkPhysicalDeviceFeatures pdf =
	{
		.samplerAnisotropy = VK_TRUE,
	};

	VkDeviceCreateInfo dc_info =
	{
		.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext                   = NULL,
		.flags                   = 0,
		.queueCreateInfoCount    = 1,
		.pQueueCreateInfos       = &dqc_info,
		.enabledLayerCount       = 0,
		.ppEnabledLayerNames     = NULL,
		.enabledExtensionCount   = extension_count,
		.ppEnabledExtensionNames = extension_names,
		.pEnabledFeatures        = self->max_anisotropy ?
		                           &pdf : NULL
	};

	if(vkCreateDevice(self->physical_device, &dc_info,
	                  NULL, &self->device) != VK_SUCCESS)
	{
		LOGE("vkCreateDevice failed");
		goto fail_create_device;
	}

	vkGetDeviceQueue(self->device,
	                 self->queue_family_index,
	                 VKK_QUEUE_FOREGROUND,
	                 &(self->queue[VKK_QUEUE_FOREGROUND]));
	if(queue_count == 1)
	{
		// multiple queues are desired for priority based
		// scheduling of rendering tasks
		LOGW("device only supports a single queue");
		self->queue[VKK_QUEUE_BACKGROUND] = self->queue[VKK_QUEUE_FOREGROUND];
	}
	else
	{
		vkGetDeviceQueue(self->device,
		                 self->queue_family_index,
		                 VKK_QUEUE_BACKGROUND,
		                 &(self->queue[VKK_QUEUE_BACKGROUND]));
	}

	FREE(qfp);

	// success
	return 1;

	// failure
	fail_create_device:
	fail_select_qfp:
		FREE(qfp);
	return 0;
}

static void
vkk_engine_importPipelineCache(vkk_engine_t* self,
                               int* _size,
                               void** _data)
{
	ASSERT(self);

	*_size = 0;
	*_data = NULL;

	char cache[256];
	snprintf(cache, 256, "%s/pipeline.cache",
	         self->internal_path);

	// ignore if cache doesn't exist
	if(access(cache, F_OK) != 0)
	{
		return;
	}

	FILE* f = fopen(cache, "r");
	if(f == NULL)
	{
		LOGW("invalid");
		return;
	}

	fseek(f, 0, SEEK_END);
	int size = (int) ftell(f);
	if(size <= 0)
	{
		LOGW("invalid");
		goto fail_size;
	}

	void* data = CALLOC(size, sizeof(char));
	if(data == NULL)
	{
		LOGW("invalid");
		goto fail_calloc;
	}

	fseek(f, 0, SEEK_SET);
	if(fread(data, size, 1, f) != 1)
	{
		LOGW("invalid");
		goto fail_read;
	}
	fclose(f);

	// success
	*_size = size;
	*_data = data;
	return;

	// failure
	fail_read:
		FREE(data);
	fail_calloc:
	fail_size:
		fclose(f);
}

static void
vkk_engine_exportPipelineCache(vkk_engine_t* self)
{
	ASSERT(self);

	size_t size = 0;
	if(vkGetPipelineCacheData(self->device,
	                          self->pipeline_cache,
	                          &size, NULL) != VK_SUCCESS)
	{
		LOGE("invalid");
		return;
	}

	if(size == 0)
	{
		LOGE("invalid");
		return;
	}

	void* data = CALLOC(size, sizeof(char));
	if(data == NULL)
	{
		LOGE("invalid");
		return;
	}

	if(vkGetPipelineCacheData(self->device,
	                          self->pipeline_cache,
	                          &size, data) != VK_SUCCESS)
	{
		LOGE("invalid");
		goto fail_data;
	}

	char cache[256];
	snprintf(cache, 256, "%s/pipeline.cache",
	         self->internal_path);

	FILE* f = fopen(cache, "w");
	if(f == NULL)
	{
		LOGE("invalid");
		goto fail_open;
	}

	if(fwrite(data, size, 1, f) != 1)
	{
		LOGE("invalid");
		goto fail_write;
	}

	FREE(data);
	fclose(f);

	// success
	return;

	// failure
	fail_write:
		fclose(f);
	fail_open:
	fail_data:
		FREE(data);
}

static int vkk_engine_newPipelineCache(vkk_engine_t* self)
{
	ASSERT(self);

	// import the pipeline cache
	int   size = 0;
	void* data = NULL;
	vkk_engine_importPipelineCache(self, &size, &data);

	VkPipelineCacheCreateInfo pc_info =
	{
		.sType           = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
		.pNext           = NULL,
		.flags           = 0,
		.initialDataSize = (uint32_t) size,
		.pInitialData    = data
	};

	if(vkCreatePipelineCache(self->device, &pc_info, NULL,
	                         &self->pipeline_cache) != VK_SUCCESS)
	{
		LOGE("vkCreatePipelineCache failed");
		goto fail_pipeline_cache;
	}

	FREE(data);

	// success
	return 1;

	// failure
	fail_pipeline_cache:
		FREE(data);
	return 0;
}

static uint32_t*
vkk_engine_importShaderModule(vkk_engine_t* self,
                              const char* fname,
                              size_t* _size)
{
	ASSERT(self);
	ASSERT(fname);
	ASSERT(_size);

	char resource[256];
	snprintf(resource, 256, "%s/resource.bfs",
	         self->internal_path);

	bfs_file_t* bfs;
	bfs = bfs_file_open(resource, 1, BFS_MODE_RDONLY);
	if(bfs == NULL)
	{
		return NULL;
	}

	size_t size = 0;
	void*  code = NULL;
	if(bfs_file_blobGet(bfs, 0, fname,
	                    &size, &code) == 0)
	{
		goto fail_get;
	}

	if((size == 0) || ((size % 4) != 0))
	{
		LOGE("invalid fname=%s, size=%u", fname, (unsigned int) size);
		goto fail_size;
	}

	bfs_file_close(&bfs);

	*_size = size;

	// success
	return (uint32_t*) code;

	// failure
	fail_size:
		FREE(code);
	fail_get:
		bfs_file_close(&bfs);
	return NULL;
}

static void vkk_engine_initImageUsage(vkk_engine_t* self)
{
	ASSERT(self);

	int i;
	VkFormatProperties fp;
	for(i = 0; i < VKK_IMAGE_FORMAT_COUNT; ++i)
	{
		VkFormat format = vkk_util_imageFormat(i);
		vkGetPhysicalDeviceFormatProperties(self->physical_device,
		                                    format, &fp);

		// check for texture caps
		VkFormatFeatureFlags flags = fp.optimalTilingFeatures;
		if((flags & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) &&
		   (flags & VK_FORMAT_FEATURE_TRANSFER_DST_BIT))
		{
			self->image_caps_array[i].texture = 1;
		}

		// check for mipmap caps
		if((flags & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) &&
		   (flags & VK_FORMAT_FEATURE_BLIT_SRC_BIT)      &&
		   (flags & VK_FORMAT_FEATURE_BLIT_DST_BIT)      &&
		   (flags & VK_FORMAT_FEATURE_TRANSFER_SRC_BIT)  &&
		   (flags & VK_FORMAT_FEATURE_TRANSFER_DST_BIT))
		{
			self->image_caps_array[i].mipmap = 1;
		}

		// check for linear filtering
		if(flags & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)
		{
			self->image_caps_array[i].filter_linear = 1;
		}

		// check for target caps
		if(flags & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)
		{
			self->image_caps_array[i].target = 1;
			if(flags & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT)
			{
				self->image_caps_array[i].target_blend = 1;
			}
		}

		// debug image caps
		#if 0
			LOGI("caps(%i) texture=%i, mipmap=%i, filter_linear=%i, target=%i, target_blend=%i",
			     i,
			     (int) self->image_caps_array[i].texture,
			     (int) self->image_caps_array[i].mipmap,
			     (int) self->image_caps_array[i].filter_linear,
			     (int) self->image_caps_array[i].target,
			     (int) self->image_caps_array[i].target_blend);
		#endif
	}
}

static vkk_object_t*
vkk_object_new(vkk_objectType_e type, void* obj)
{
	ASSERT(obj);

	vkk_object_t* self;
	self = (vkk_object_t*) CALLOC(1, sizeof(vkk_object_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->type = type;
	self->obj  = obj;
	return self;
}

static void
vkk_object_delete(vkk_object_t** _self)
{
	ASSERT(_self);

	vkk_object_t* self = *_self;
	if(self)
	{
		FREE(self);
		*_self = NULL;
	}
}

static void
vkk_engine_destructRenderer(vkk_engine_t* self, int wait,
                            vkk_renderer_t** _renderer)
{
	ASSERT(self);
	ASSERT(_renderer);

	vkk_renderer_t* renderer = *_renderer;
	if(renderer)
	{
		// default renderer deleted in vkk_engine_delete
		if(renderer->type == VKK_RENDERER_TYPE_IMAGE)
		{
			vkk_imageRenderer_delete(_renderer);
		}
		else if(renderer->type == VKK_RENDERER_TYPE_IMAGESTREAM)
		{
			vkk_imageStreamRenderer_t* isr;
			isr = (vkk_imageStreamRenderer_t*) renderer;
			if(wait)
			{
				vkk_engine_rendererWaitForTimestamp(self, isr->ts);
			}
			else if(isr->ts != 0.0)
			{
				vkk_engine_queueWaitIdle(self, VKK_QUEUE_FOREGROUND);
			}

			vkk_imageStreamRenderer_delete(_renderer);
		}
		else if(renderer->type == VKK_RENDERER_TYPE_SECONDARY)
		{
			vkk_secondaryRenderer_t* sec;
			sec = (vkk_secondaryRenderer_t*) renderer;
			if(wait)
			{
				vkk_engine_rendererWaitForTimestamp(self, sec->ts);
			}
			else if(sec->ts != 0.0)
			{
				vkk_engine_queueWaitIdle(self, VKK_QUEUE_FOREGROUND);
			}

			vkk_secondaryRenderer_delete(_renderer);
		}
	}
}

static void
vkk_engine_destructCompute(vkk_engine_t* self, int wait,
                           vkk_compute_t** _compute)
{
	ASSERT(self);
	ASSERT(_compute);

	vkk_compute_destruct(_compute);
}

static void
vkk_engine_destructBuffer(vkk_engine_t* self, int wait,
                          vkk_buffer_t** _buffer)
{
	ASSERT(self);
	ASSERT(_buffer);

	vkk_buffer_t* buffer = *_buffer;
	if(buffer)
	{
		if(wait)
		{
			vkk_engine_rendererWaitForTimestamp(self, buffer->ts);
		}
		else if(buffer->ts != 0.0)
		{
			vkk_engine_queueWaitIdle(self, VKK_QUEUE_FOREGROUND);
		}

		uint32_t count;
		count = (buffer->update == VKK_UPDATE_MODE_ASYNCHRONOUS) ?
		        vkk_engine_imageCount(self) : 1;
		int i;
		for(i = 0; i < count; ++i)
		{
			vkk_memoryManager_free(self->mm, &buffer->memory[i]);
			vkDestroyBuffer(self->device,
			                buffer->buffer[i],
			                NULL);
		}
		FREE(buffer->memory);
		FREE(buffer->buffer);
		FREE(buffer);
		*_buffer = NULL;
	}
}

static void
vkk_engine_destructImage(vkk_engine_t* self, int wait,
                         vkk_image_t** _image)
{
	ASSERT(self);
	ASSERT(_image);

	vkk_image_t* image = *_image;
	if(image)
	{
		if(wait)
		{
			vkk_engine_rendererWaitForTimestamp(self, image->ts);
		}
		else if(image->ts != 0.0)
		{
			vkk_engine_queueWaitIdle(self, VKK_QUEUE_FOREGROUND);
		}

		vkDestroyImageView(self->device, image->image_view,
		                   NULL);
		vkk_memoryManager_free(self->mm, &image->memory);
		vkDestroyImage(self->device, image->image, NULL);
		FREE(image->layout_array);
		FREE(image);
		*_image = NULL;
	}
}

static void
vkk_engine_destructUniformSetFactory(vkk_engine_t* self,
                                     vkk_uniformSetFactory_t** _usf)
{
	ASSERT(self);
	ASSERT(_usf);

	vkk_uniformSetFactory_t* usf = *_usf;
	if(usf)
	{
		cc_listIter_t* iter;
		iter = cc_list_head(usf->us_list);
		while(iter)
		{
			vkk_uniformSet_t* us;
			us = (vkk_uniformSet_t*)
			     cc_list_remove(usf->us_list, &iter);
			FREE(us->ds_array);
			FREE(us->ua_array);
			FREE(us);
		}

		iter = cc_list_head(usf->dp_list);
		while(iter)
		{
			VkDescriptorPool dp;
			dp = (VkDescriptorPool)
			     cc_list_remove(usf->dp_list, &iter);
			vkDestroyDescriptorPool(self->device, dp, NULL);
		}

		cc_list_delete(&usf->dp_list);
		cc_list_delete(&usf->us_list);
		vkDestroyDescriptorSetLayout(self->device,
		                             usf->ds_layout, NULL);
		FREE(usf->ub_array);
		FREE(usf);
		*_usf = NULL;
	}
}

static void
vkk_engine_destructUniformSet(vkk_engine_t* self, int wait,
                              vkk_uniformSet_t** _us)
{
	ASSERT(self);
	ASSERT(_us);

	vkk_uniformSet_t* us = *_us;
	if(us)
	{
		if(wait)
		{
			vkk_engine_rendererWaitForTimestamp(self, us->ts);
		}
		else if(us->ts != 0.0)
		{
			vkk_engine_queueWaitIdle(self, VKK_QUEUE_FOREGROUND);
		}

		vkk_engine_usfLock(self);
		if(cc_list_append(us->usf->us_list, NULL,
		                  (const void*) us) == NULL)
		{
			// when an error occurs the uniform set will
			// be unreachable from the factory but the
			// uniform set will still be freed when the
			// corresponding uniform set factory is freed
			FREE(us->ds_array);
			FREE(us->ua_array);
			FREE(us);
		}
		vkk_engine_usfUnlock(self);
		vkk_uniformSetFactory_decRef(us->usf);
		*_us = NULL;
	}
}

static void
vkk_engine_destructPipelineLayout(vkk_engine_t* self,
                                  vkk_pipelineLayout_t** _pl)
{
	ASSERT(self);
	ASSERT(_pl);

	vkk_pipelineLayout_t* pl = *_pl;
	if(pl)
	{
		vkDestroyPipelineLayout(self->device,
		                        pl->pl, NULL);
		FREE(pl);
		*_pl = NULL;
	}
}

static void
vkk_engine_destructGraphicsPipeline(vkk_engine_t* self, int wait,
                                    vkk_graphicsPipeline_t** _gp)
{
	ASSERT(self);
	ASSERT(_gp);

	vkk_graphicsPipeline_t* gp = *_gp;
	if(gp)
	{
		if(wait)
		{
			vkk_engine_rendererWaitForTimestamp(self, gp->ts);
		}
		else if(gp->ts != 0.0)
		{
			vkk_engine_queueWaitIdle(self, VKK_QUEUE_FOREGROUND);
		}

		vkDestroyPipeline(self->device,
		                  gp->pipeline, NULL);
		FREE(gp);
		*_gp = NULL;
	}
}

static void
vkk_engine_destructComputePipeline(vkk_engine_t* self, int wait,
                                   vkk_computePipeline_t** _cp)
{
	ASSERT(self);
	ASSERT(_cp);

	vkk_computePipeline_t* cp = *_cp;
	if(cp)
	{
		if(wait)
		{
			vkk_engine_rendererWaitForTimestamp(self, cp->ts);
		}
		else if(cp->ts != 0.0)
		{
			vkk_engine_queueWaitIdle(self, VKK_QUEUE_FOREGROUND);
		}

		vkDestroyPipeline(self->device,
		                  cp->pipeline, NULL);
		FREE(cp);
		*_cp = NULL;
	}
}

static void
vkk_engine_runDestructFn(int tid, void* owner, void* task)
{
	ASSERT(owner);
	ASSERT(task);

	vkk_engine_t* engine;
	vkk_object_t* object;
	engine = (vkk_engine_t*) owner;
	object = (vkk_object_t*) task;

	if(object->type == VKK_OBJECT_TYPE_RENDERER)
	{
		vkk_engine_destructRenderer(engine, 1,
		                            &object->renderer);
	}
	else if(object->type == VKK_OBJECT_TYPE_COMPUTE)
	{
		vkk_engine_destructCompute(engine, 1,
		                            &object->compute);
	}
	else if(object->type == VKK_OBJECT_TYPE_BUFFER)
	{
		vkk_engine_destructBuffer(engine, 1,
		                          &object->buffer);
	}
	else if(object->type == VKK_OBJECT_TYPE_IMAGE)
	{
		vkk_engine_destructImage(engine, 1,
		                         &object->image);
	}
	else if(object->type == VKK_OBJECT_TYPE_UNIFORMSETFACTORY)
	{
		vkk_engine_destructUniformSetFactory(engine,
		                                     &object->usf);
	}
	else if(object->type == VKK_OBJECT_TYPE_UNIFORMSET)
	{
		vkk_engine_destructUniformSet(engine, 1,
		                              &object->us);
	}
	else if(object->type == VKK_OBJECT_TYPE_PIPELINELAYOUT)
	{
		vkk_engine_destructPipelineLayout(engine,
		                                  &object->pl);
	}
	else if(object->type == VKK_OBJECT_TYPE_GRAPHICSPIPELINE)
	{
		vkk_engine_destructGraphicsPipeline(engine, 1,
		                                    &object->gp);
	}
	else if(object->type == VKK_OBJECT_TYPE_COMPUTEPIPELINE)
	{
		vkk_engine_destructComputePipeline(engine, 1,
		                                   &object->cp);
	}
	else
	{
		LOGE("invalid type=%i", object->type);
	}

	vkk_object_delete(&object);
}

/***********************************************************
* public                                                   *
***********************************************************/

void vkk_engine_version(vkk_engine_t* self,
                        const vkk_version_t** version)
{
	ASSERT(self);
	ASSERT(version);

	*version = &self->version;
}

void vkk_engine_appInfo(vkk_engine_t* self,
                        const char** app_name,
                        const vkk_version_t** app_version)
{
	ASSERT(self);
	ASSERT(app_name);
	ASSERT(app_version);

	*app_name    = self->app_name;
	*app_version = &self->app_version;
}

const char* vkk_engine_internalPath(vkk_engine_t* self)
{
	ASSERT(self);

	return self->internal_path;
}

const char* vkk_engine_externalPath(vkk_engine_t* self)
{
	ASSERT(self);

	return self->external_path;
}

void vkk_engine_meminfo(vkk_engine_t* self,
                        size_t* _count_chunks,
                        size_t* _count_slots,
                        size_t* _size_chunks,
                        size_t* _size_slots)
{
	ASSERT(self);
	ASSERT(_count_chunks);
	ASSERT(_count_slots);
	ASSERT(_size_chunks);
	ASSERT(_size_slots);

	vkk_memoryManager_meminfo(self->mm,
	                          _count_chunks, _count_slots,
	                          _size_chunks, _size_slots);
}

void vkk_engine_imageCaps(vkk_engine_t* self,
                          vkk_imageFormat_e format,
                          vkk_imageCaps_t* caps)
{
	ASSERT(self);
	ASSERT(caps);

	*caps = self->image_caps_array[format];
}

float vkk_engine_maxAnisotropy(vkk_engine_t* self)
{
	ASSERT(self);

	return self->max_anisotropy;
}

vkk_renderer_t*
vkk_engine_defaultRenderer(vkk_engine_t* self)
{
	ASSERT(self);

	// default renderer will be NULL for no display mode
	return self->renderer;
}

void
vkk_engine_platformCmd(vkk_engine_t* self, int cmd)
{
	ASSERT(self);

	if((cmd == VKK_PLATFORM_CMD_LOADURL)         ||
	   (cmd == VKK_PLATFORM_CMD_DOCUMENT_CREATE) ||
	   (cmd == VKK_PLATFORM_CMD_DOCUMENT_OPEN)   ||
	   (cmd == VKK_PLATFORM_CMD_DOCUMENT_NAME))
	{
		LOGE("invalid cmd=%i", cmd);
		return;
	}

	vkk_platformCmdInfo_t info =
	{
		.cmd = cmd
	};
	vkk_platform_cmd(self->platform, &info);
}

void
vkk_engine_platformCmdLoadUrl(vkk_engine_t* self,
                              const char* url)
{
	ASSERT(self);
	ASSERT(url);

	vkk_platformCmdInfo_t info =
	{
		.cmd = VKK_PLATFORM_CMD_LOADURL
	};
	snprintf(info.msg, 256, "{\"url\":\"%s\"}", url);
	vkk_platform_cmd(self->platform, &info);
}

#ifdef ANDROID

void
vkk_engine_platformCmdDocumentCreate(vkk_engine_t* self,
                                     void* priv,
                                     vkk_platformCmd_documentFn document_fn,
                                     const char* type,
                                     const char* mode,
                                     const char* name,
                                     const char* ext)
{
	// priv may be NULL
	ASSERT(self);
	ASSERT(document_fn);
	ASSERT(type);
	ASSERT(mode);
	ASSERT(name);
	ASSERT(ext);

	vkk_platformCmdInfo_t info =
	{
		.cmd         = VKK_PLATFORM_CMD_DOCUMENT_CREATE,
		.priv        = priv,
		.document_fn = document_fn
	};
	snprintf(info.msg, 256,
	         "{\"title\":\"%s%s\",\"type\":\"%s\",\"mode\":\"%s\"}",
	         name, ext, type, mode);

	vkk_platform_cmd(self->platform, &info);
}

void
vkk_engine_platformCmdDocumentOpen(vkk_engine_t* self,
                                   void* priv,
                                   vkk_platformCmd_documentFn document_fn,
                                   const char* type,
                                   const char* mode)
{
	// priv may be NULL
	ASSERT(self);
	ASSERT(document_fn);
	ASSERT(type);
	ASSERT(mode);

	vkk_platformCmdInfo_t info =
	{
		.cmd         = VKK_PLATFORM_CMD_DOCUMENT_OPEN,
		.priv        = priv,
		.document_fn = document_fn
	};
	snprintf(info.msg, 256,
	         "{\"type\":\"%s\",\"mode\":\"%s\"}",
	         type, mode);

	vkk_platform_cmd(self->platform, &info);
}

#else // LINUX

void
vkk_engine_platformCmdDocumentCreate(vkk_engine_t* self,
                                     void* priv,
                                     vkk_platformCmd_documentFn document_fn,
                                     const char* fname)
{
	// priv may be NULL
	ASSERT(self);
	ASSERT(document_fn);
	ASSERT(fname);

	vkk_platformCmdInfo_t info =
	{
		.cmd         = VKK_PLATFORM_CMD_DOCUMENT_CREATE,
		.priv        = priv,
		.document_fn = document_fn
	};
	snprintf(info.msg, 256, "%s", fname);
	vkk_platform_cmd(self->platform, &info);
}

void
vkk_engine_platformCmdDocumentOpen(vkk_engine_t* self,
                                   void* priv,
                                   vkk_platformCmd_documentFn document_fn,
                                   const char* fname)
{
	// priv may be NULL
	ASSERT(self);
	ASSERT(document_fn);
	ASSERT(fname);

	vkk_platformCmdInfo_t info =
	{
		.cmd         = VKK_PLATFORM_CMD_DOCUMENT_OPEN,
		.priv        = priv,
		.document_fn = document_fn
	};
	snprintf(info.msg, 256, "%s", fname);
	vkk_platform_cmd(self->platform, &info);
}

void
vkk_engine_platformCmdDocumentName(vkk_engine_t* self,
                                   void* priv,
                                   vkk_platformCmd_documentFn document_fn,
                                   const char* fname)
{
	// priv may be NULL
	ASSERT(self);
	ASSERT(document_fn);
	ASSERT(fname);

	vkk_platformCmdInfo_t info =
	{
		.cmd         = VKK_PLATFORM_CMD_DOCUMENT_NAME,
		.priv        = priv,
		.document_fn = document_fn
	};
	snprintf(info.msg, 256, "%s", fname);
	vkk_platform_cmd(self->platform, &info);
}

#endif

/***********************************************************
* protected                                                *
***********************************************************/

vkk_engine_t* vkk_engine_new(vkk_platform_t* platform,
                             const char* app_name,
                             const char* app_dir,
                             vkk_version_t* app_version,
                             const char* internal_path,
                             const char* external_path)
{
	ASSERT(platform);
	ASSERT(app_name);
	ASSERT(app_dir);
	ASSERT(app_version);
	ASSERT(internal_path);
	ASSERT(external_path);

	vkk_engine_t* self;
	self = (vkk_engine_t*)
	       CALLOC(1, sizeof(vkk_engine_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	self->platform = platform;

	#ifndef ANDROID
		if(vkk_engine_initSDL(self, app_name) == 0)
		{
			FREE(self);
			return NULL;
		}
	#endif

	self->version.major = 1;
	self->version.minor = 1;
	self->version.patch = 59;

	// app info
	snprintf(self->app_name, 256, "%s", app_name);
	memcpy((void*) &self->app_version,
	       (const void*) app_version,
	       sizeof(vkk_version_t));

	// initialize paths
	// trim tailing '/' character of internal/external path
	size_t len = strlen(internal_path);
	if(internal_path[len - 1] == '/')
	{
		#ifdef ANDROID
			snprintf(self->internal_path, 256, "%s",
			         internal_path);
			len = strlen(self->internal_path);
			if(self->internal_path[len - 1] == '/')
			{
				self->internal_path[len - 1] = '\0';
			}
		#else
			snprintf(self->internal_path, 256, "%s%s",
			         internal_path, app_dir);
		#endif
	}
	else
	{
		#ifdef ANDROID
			snprintf(self->internal_path, 256, "%s",
			         internal_path);
		#else
			snprintf(self->internal_path, 256, "%s/%s",
			         internal_path, app_dir);
		#endif
	}
	len = strlen(external_path);
	if(external_path[len - 1] == '/')
	{
		#ifdef ANDROID
			snprintf(self->external_path, 256, "%s",
			         external_path);
			len = strlen(self->external_path);
			if(self->external_path[len - 1] == '/')
			{
				self->external_path[len - 1] = '\0';
			}
		#else
			snprintf(self->external_path, 256, "%s%s",
			         external_path, app_dir);
		#endif
	}
	else
	{
		#ifdef ANDROID
			snprintf(self->external_path, 256, "%s",
			         external_path);
		#else
			snprintf(self->external_path, 256, "%s/%s",
			         external_path, app_dir);
		#endif
	}

	if(pthread_mutex_init(&self->cmd_mutex, NULL) != 0)
	{
		LOGE("pthread_mutex_init failed");
		goto fail_cmd_mutex;
	}

	if(pthread_mutex_init(&self->usf_mutex, NULL) != 0)
	{
		LOGE("pthread_mutex_init failed");
		goto fail_usf_mutex;
	}

	if(pthread_mutex_init(&self->utility_mutex, NULL) != 0)
	{
		LOGE("pthread_mutex_init failed");
		goto fail_utility_mutex;
	}

	if(pthread_mutex_init(&self->renderer_mutex, NULL) != 0)
	{
		LOGE("pthread_mutex_init failed");
		goto fail_renderer_mutex;
	}

	if(pthread_cond_init(&self->renderer_cond, NULL) != 0)
	{
		LOGE("pthread_cond_init failed");
		goto fail_renderer_cond;
	}

	if(vkk_engine_newInstance(self, app_name,
	                          app_version) == 0)
	{
		goto fail_instance;
	}

	if(vkk_engine_newSurface(self) == 0)
	{
		goto fail_surface;
	}

	if(vkk_engine_getPhysicalDevice(self) == 0)
	{
		goto fail_physical_device;
	}

	if(vkk_engine_newDevice(self) == 0)
	{
		goto fail_device;
	}

	self->mm = vkk_memoryManager_new(self);
	if(self->mm == NULL)
	{
		goto fail_mm;
	}

	self->xfer = vkk_xferManager_new(self);
	if(self->xfer == NULL)
	{
		goto fail_xfer;
	}

	if(vkk_engine_newPipelineCache(self) == 0)
	{
		goto fail_pipeline_cache;
	}

	self->shader_modules = cc_map_new();
	if(self->shader_modules == NULL)
	{
		goto fail_shader_modules;
	}

	self->samplers = cc_map_new();
	if(self->samplers == NULL)
	{
		goto fail_samplers;
	}

	vkk_engine_initImageUsage(self);

	if(vkk_engine_noDisplay())
	{
		// skip default renderer
	}
	else
	{
		self->renderer = vkk_defaultRenderer_new(self);
		if(self->renderer == NULL)
		{
			goto fail_renderer;
		}
	}

	self->jobq_destruct = cc_jobq_new((void*) self, 1,
	                                  CC_JOBQ_THREAD_PRIORITY_DEFAULT,
	                                  vkk_engine_runDestructFn);
	if(self->jobq_destruct == NULL)
	{
		goto fail_jobq_destruct;
	}

	// success
	return self;

	// failure
	fail_jobq_destruct:
		vkk_defaultRenderer_delete(&self->renderer);
	fail_renderer:
		cc_map_delete(&self->samplers);
	fail_samplers:
		cc_map_delete(&self->shader_modules);
	fail_shader_modules:
		vkDestroyPipelineCache(self->device,
		                       self->pipeline_cache, NULL);
	fail_pipeline_cache:
		vkk_xferManager_delete(&self->xfer);
	fail_xfer:
		vkk_memoryManager_delete(&self->mm);
	fail_mm:
		vkDestroyDevice(self->device, NULL);
	fail_device:
	fail_physical_device:
		vkDestroySurfaceKHR(self->instance,
		                    self->surface, NULL);
	fail_surface:
		vkDestroyInstance(self->instance, NULL);
	fail_instance:
		pthread_cond_destroy(&self->renderer_cond);
	fail_renderer_cond:
		pthread_mutex_destroy(&self->renderer_mutex);
	fail_renderer_mutex:
		pthread_mutex_destroy(&self->utility_mutex);
	fail_utility_mutex:
		pthread_mutex_destroy(&self->usf_mutex);
	fail_usf_mutex:
		pthread_mutex_destroy(&self->cmd_mutex);
	fail_cmd_mutex:
		#ifndef ANDROID
			vkk_engine_shutdownSDL(self);
		#endif
		FREE(self);
	return NULL;
}

void vkk_engine_delete(vkk_engine_t** _self)
{
	// *_self can be null
	ASSERT(_self);

	vkk_engine_t* self = *_self;
	if(self)
	{
		ASSERT(self->shutdown);

		// finish destruction jobq
		// objects in jobq may depend on default renderer
		cc_jobq_finish(self->jobq_destruct);
		vkk_defaultRenderer_delete(&self->renderer);
		cc_jobq_delete(&self->jobq_destruct);

		cc_mapIter_t* miter = cc_map_head(self->samplers);
		while(miter)
		{
			VkSampler* samplerp;
			samplerp = (VkSampler*)
			           cc_map_remove(self->samplers, &miter);
			vkDestroySampler(self->device, *samplerp, NULL);
			FREE(samplerp);
		}
		cc_map_delete(&self->samplers);

		miter = cc_map_head(self->shader_modules);
		while(miter)
		{
			VkShaderModule sm;
			sm = (VkShaderModule)
			     cc_map_remove(self->shader_modules, &miter);
			vkDestroyShaderModule(self->device, sm, NULL);
		}
		cc_map_delete(&self->shader_modules);
		vkk_engine_exportPipelineCache(self);
		vkDestroyPipelineCache(self->device,
		                       self->pipeline_cache, NULL);
		vkk_xferManager_delete(&self->xfer);
		vkk_memoryManager_delete(&self->mm);
		vkDestroyDevice(self->device, NULL);
		vkDestroySurfaceKHR(self->instance,
		                    self->surface, NULL);
		vkDestroyInstance(self->instance, NULL);
		pthread_mutex_destroy(&self->utility_mutex);
		pthread_mutex_destroy(&self->usf_mutex);
		pthread_mutex_destroy(&self->cmd_mutex);
		#ifndef ANDROID
			vkk_engine_shutdownSDL(self);
		#endif
		FREE(self);
		*_self = NULL;
	}
}

void vkk_engine_shutdown(vkk_engine_t* self)
{
	ASSERT(self);

	vkk_engine_rendererLock(self);
	if(self->shutdown == 0)
	{
		vkDeviceWaitIdle(self->device);
		self->shutdown = 1;
		vkk_engine_rendererSignal(self);
		vkk_memoryManager_shutdown(self->mm);
		vkk_xferManager_shutdown(self->xfer);
	}
	vkk_engine_rendererUnlock(self);
}

void vkk_engine_deviceWaitIdle(vkk_engine_t* self)
{
	ASSERT(self);

	if(self->renderer)
	{
		vkk_defaultRenderer_deviceWaitIdle(self->renderer);
	}
}

int vkk_engine_recreate(vkk_engine_t* self)
{
	ASSERT(self);

	if(self->renderer)
	{
		return vkk_defaultRenderer_recreate(self->renderer);
	}
	return 1;
}

void vkk_engine_mipmapImage(vkk_engine_t* self,
                            vkk_image_t* image,
                            VkCommandBuffer cb)
{
	ASSERT(self);
	ASSERT(image);
	ASSERT(cb != VK_NULL_HANDLE);

	// transition the base mip level to a src for blitting
	vkk_util_imageMemoryBarrier(image, cb,
	                            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	                            0, 1);

	// transition the sub mip levels to dst for blitting
	vkk_util_imageMemoryBarrier(image, cb,
	                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	                            1, image->mip_levels - 1);

	int i;
	uint32_t w = image->width;
	uint32_t h = image->height;
	uint32_t d = image->depth;
	for(i = 1; i < image->mip_levels; ++i)
	{
		VkImageBlit ib =
		{
			.srcSubresource =
			{
				.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel       = i - 1,
				.baseArrayLayer = 0,
				.layerCount     = 1
			},
			.srcOffsets =
			{
				{
					.x = 0,
					.y = 0,
					.z = 0,
				},
				{
					.x = (uint32_t) (w >> (i - 1)),
					.y = (uint32_t) (h >> (i - 1)),
					.z = (uint32_t) (d >> (i - 1)),
				}
			},
			.dstSubresource =
			{
				.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel       = i,
				.baseArrayLayer = 0,
				.layerCount     = 1
			},
			.dstOffsets =
			{
				{
					.x = 0,
					.y = 0,
					.z = 0,
				},
				{
					.x = (uint32_t) (w >> i),
					.y = (uint32_t) (h >> i),
					.z = (uint32_t) (d >> i),
				}
			}
		};

		// enforce the minimum size
		if(ib.srcOffsets[1].x == 0)
		{
			ib.srcOffsets[1].x = 1;
		}
		if(ib.srcOffsets[1].y == 0)
		{
			ib.srcOffsets[1].y = 1;
		}
		if(ib.srcOffsets[1].z == 0)
		{
			ib.srcOffsets[1].z = 1;
		}
		if(ib.dstOffsets[1].x == 0)
		{
			ib.dstOffsets[1].x = 1;
		}
		if(ib.dstOffsets[1].y == 0)
		{
			ib.dstOffsets[1].y = 1;
		}
		if(ib.dstOffsets[1].z == 0)
		{
			ib.dstOffsets[1].z = 1;
		}

		VkFormat format = vkk_util_imageFormat(image->format);

		VkFormatProperties fp;
		vkGetPhysicalDeviceFormatProperties(self->physical_device,
		                                    format, &fp);

		VkFilter filter = VK_FILTER_NEAREST;
		if(fp.optimalTilingFeatures &
		   VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)
		{
			filter = VK_FILTER_LINEAR;
		}

		vkCmdBlitImage(cb,
		               image->image,
		               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		               image->image,
		               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		               1, &ib, filter);

		// transition the mip level i to a src for blitting
		vkk_util_imageMemoryBarrier(image, cb,
		                            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		                            i, 1);
	}
}

uint32_t vkk_engine_imageCount(vkk_engine_t* self)
{
	ASSERT(self);

	if(self->renderer)
	{
		return vkk_defaultRenderer_imageCount(self->renderer);
	}
	return 1;
}

int vkk_engine_queueSubmit(vkk_engine_t* self,
                           uint32_t queue,
                           VkCommandBuffer* cb,
                           uint32_t wait_count,
                           VkSemaphore* semaphore_wait,
                           VkSemaphore* semaphore_submit,
                           VkPipelineStageFlags* wait_dst_stage_mask,
                           VkFence fence)
{
	// semaphore_wait, semaphore_submit and
	// wait_dst_stage_mask may be NULL
	ASSERT(self);
	ASSERT(queue < VKK_QUEUE_COUNT);
	ASSERT(cb);

	VkSubmitInfo s_info =
	{
		.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext                = NULL,
		.waitSemaphoreCount   = wait_count,
		.pWaitSemaphores      = wait_count ?
		                        semaphore_wait : NULL,
		.pWaitDstStageMask    = wait_dst_stage_mask,
		.commandBufferCount   = 1,
		.pCommandBuffers      = cb,
		.signalSemaphoreCount = semaphore_submit ? 1 : 0,
		.pSignalSemaphores    = semaphore_submit
	};

	vkk_engine_rendererLock(self);
	if(self->shutdown)
	{
		vkk_engine_rendererUnlock(self);
		return 0;
	}

	if(vkQueueSubmit(self->queue[queue], 1, &s_info,
	                 fence) != VK_SUCCESS)
	{
		LOGE("vkQueueSubmit failed");
		vkk_engine_rendererUnlock(self);
		return 0;
	}
	vkk_engine_rendererUnlock(self);

	return 1;
}

void vkk_engine_queueWaitIdle(vkk_engine_t* self,
                              uint32_t queue)
{
	ASSERT(self);
	ASSERT(queue < VKK_QUEUE_COUNT);

	vkk_engine_rendererLock(self);
	if(self->shutdown == 0)
	{
		vkQueueWaitIdle(self->queue[queue]);
	}
	vkk_engine_rendererUnlock(self);
}

int
vkk_engine_allocateDescriptorSetsLocked(vkk_engine_t* self,
                                        VkDescriptorPool dp,
                                        const VkDescriptorSetLayout* dsl_array,
                                        uint32_t ds_count,
                                        VkDescriptorSet* ds_array)
{
	ASSERT(self);
	ASSERT(dp != VK_NULL_HANDLE);
	ASSERT(dsl_array);
	ASSERT(ds_count > 0);
	ASSERT(ds_array);

	VkDescriptorSetAllocateInfo ds_info =
	{
		.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext              = NULL,
		.descriptorPool     = dp,
		.descriptorSetCount = ds_count,
		.pSetLayouts        = dsl_array
	};

	if(vkAllocateDescriptorSets(self->device, &ds_info,
	                            ds_array) != VK_SUCCESS)
	{
		LOGE("vkAllocateDescriptorSets failed");
		return 0;
	}

	return 1;
}

VkDescriptorPool
vkk_engine_newDescriptorPoolLocked(vkk_engine_t* self,
                                   vkk_uniformSetFactory_t* usf)
{
	ASSERT(self);
	ASSERT(usf);

	VkDescriptorType dt_map[VKK_UNIFORM_TYPE_COUNT] =
	{
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	};

	// fill the descriptor pool size array and
	// count the number of types in the factory
	int      i;
	uint32_t ps_count = 0;
	uint32_t maxSets  = VKK_DESCRIPTOR_POOL_SIZE;
	VkDescriptorPoolSize ps_array[VKK_UNIFORM_TYPE_COUNT];
	for(i = 0; i < VKK_UNIFORM_TYPE_COUNT; ++i)
	{
		// ensure the factory can allocate maxSets of each type
		char type_count = usf->type_count[i];
		if(type_count)
		{
			VkDescriptorPoolSize* ps;
			ps                  = &(ps_array[ps_count]);
			ps->type            = dt_map[i];
			ps->descriptorCount = type_count*VKK_DESCRIPTOR_POOL_SIZE;
			if(ps->descriptorCount > maxSets)
			{
				maxSets = ps->descriptorCount;
			}
			++ps_count;
		}
	}

	if(ps_count == 0)
	{
		LOGE("invalid");
		return VK_NULL_HANDLE;
	}

	// create a new descriptor pool
	VkDescriptorPool dp;
	VkDescriptorPoolCreateInfo dp_info =
	{
		.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext         = NULL,
		.flags         = 0,
		.maxSets       = maxSets,
		.poolSizeCount = ps_count,
		.pPoolSizes    = ps_array
	};
	if(vkCreateDescriptorPool(self->device, &dp_info, NULL,
	                          &dp) != VK_SUCCESS)
	{
		LOGE("vkCreateDescriptorPool failed");
		return VK_NULL_HANDLE;
	}

	// append the descriptor pool
	if(cc_list_append(usf->dp_list, NULL,
	                  (const void*) dp) == NULL)
	{
		goto fail_append_dp;
	}

	usf->ds_available = VKK_DESCRIPTOR_POOL_SIZE;

	// success
	return dp;

	// failure
	fail_append_dp:
		vkDestroyDescriptorPool(self->device, dp, NULL);
	return VK_NULL_HANDLE;
}

void
vkk_engine_attachUniformBuffer(vkk_engine_t* self,
                               vkk_uniformSet_t* us,
                               vkk_uniformAttachment_t* ua)
{
	ASSERT(self);
	ASSERT(us);
	ASSERT(ua);

	VkDescriptorType dt_map[VKK_UNIFORM_TYPE_COUNT] =
	{
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	};

	uint32_t count;
	count = (us->usf->update == VKK_UPDATE_MODE_ASYNCHRONOUS) ?
	        vkk_engine_imageCount(self) : 1;

	int i;
	for(i = 0; i < count; ++i)
	{
		uint32_t idx;
		idx = (ua->buffer->update == VKK_UPDATE_MODE_ASYNCHRONOUS) ? i : 0;
		VkDescriptorBufferInfo db_info =
		{
			.buffer  = ua->buffer->buffer[idx],
			.offset  = 0,
			.range   = ua->buffer->size
		};

		VkWriteDescriptorSet writes =
		{
			.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext            = NULL,
			.dstSet           = us->ds_array[i],
			.dstBinding       = ua->binding,
			.dstArrayElement  = 0,
			.descriptorCount  = 1,
			.descriptorType   = dt_map[ua->type],
			.pImageInfo       = NULL,
			.pBufferInfo      = &db_info,
			.pTexelBufferView = NULL,
		};

		vkUpdateDescriptorSets(self->device, 1, &writes,
		                       0, NULL);
	}
}

void
vkk_engine_attachUniformSampler(vkk_engine_t* self,
                                vkk_uniformSet_t* us,
                                vkk_samplerInfo_t* si,
                                vkk_image_t* image,
                                uint32_t binding)
{
	ASSERT(self);
	ASSERT(us);
	ASSERT(si);
	ASSERT(image);

	uint32_t count;
	count = (us->usf->update == VKK_UPDATE_MODE_ASYNCHRONOUS) ?
	        vkk_engine_imageCount(self) : 1;

	VkSampler* samplerp;
	samplerp = vkk_engine_getSamplerp(self, si);
	if(samplerp == NULL)
	{
		// ignore;
		return;
	}

	int i;
	for(i = 0; i < count; ++i)
	{
		VkDescriptorImageInfo di_info =
		{
			.sampler     = *samplerp,
			.imageView   = image->image_view,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		VkWriteDescriptorSet writes =
		{
			.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext            = NULL,
			.dstSet           = us->ds_array[i],
			.dstBinding       = binding,
			.dstArrayElement  = 0,
			.descriptorCount  = 1,
			.descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo       = &di_info,
			.pBufferInfo      = NULL,
			.pTexelBufferView = NULL,
		};

		vkUpdateDescriptorSets(self->device, 1, &writes,
		                       0, NULL);
	}
}

int
vkk_engine_getMemoryTypeIndex(vkk_engine_t* self,
                              uint32_t mt_bits,
                              VkFlags mp_flags,
                              uint32_t* mt_index)
{
	ASSERT(self);
	ASSERT(mt_index);

	VkPhysicalDeviceMemoryProperties mp;
	vkGetPhysicalDeviceMemoryProperties(self->physical_device,
	                                    &mp);

	// find a memory type that meets the memory requirements
	int i;
	for(i = 0; i < mp.memoryTypeCount; ++i)
	{
		if(mt_bits & 1)
		{
			VkFlags mp_flagsi;
			mp_flagsi = mp.memoryTypes[i].propertyFlags;
			if((mp_flagsi & mp_flags) == mp_flags)
			{
				*mt_index = i;
				return 1;
			}
		}
		mt_bits >>= 1;
	}

	return 0;
}

VkShaderModule
vkk_engine_getShaderModule(vkk_engine_t* self,
                           const char* fname)
{
	ASSERT(self);
	ASSERT(fname);

	vkk_engine_utilityLock(self);

	cc_mapIter_t* miter;
	VkShaderModule sm;
	miter = cc_map_find(self->shader_modules, fname);
	if(miter)
	{
		sm = (VkShaderModule) cc_map_val(miter);
		vkk_engine_utilityUnlock(self);
		return sm;
	}

	size_t    size = 0;
	uint32_t* code;
	code = vkk_engine_importShaderModule(self, fname, &size);
	if(code == NULL)
	{
		vkk_engine_utilityUnlock(self);
		return VK_NULL_HANDLE;
	}

	VkShaderModuleCreateInfo sm_info =
	{
		.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext    = NULL,
		.flags    = 0,
		.codeSize = size,
		.pCode    = code
	};

	if(vkCreateShaderModule(self->device, &sm_info, NULL,
	                        &sm) != VK_SUCCESS)
	{
		vkk_engine_utilityUnlock(self);
		LOGE("vkCreateShaderModule failed");
		goto fail_create;
	}

	if(cc_map_add(self->shader_modules, (const void*) sm,
	              fname) == NULL)
	{
		vkk_engine_utilityUnlock(self);
		goto fail_add;
	}

	FREE(code);

	vkk_engine_utilityUnlock(self);

	// success
	return sm;

	// failure
	fail_add:
		vkDestroyShaderModule(self->device, sm, NULL);
	fail_create:
		FREE(code);
	return VK_NULL_HANDLE;
}

VkSampler*
vkk_engine_getSamplerp(vkk_engine_t* self,
                       vkk_samplerInfo_t* si)
{
	ASSERT(self);
	ASSERT(si);

	vkk_engine_utilityLock(self);

	// reuse existing sampler
	cc_mapIter_t* miter;
	VkSampler*    samplerp;
	miter = cc_map_findp(self->samplers,
	                     sizeof(vkk_samplerInfo_t), si);
	if(miter)
	{
		samplerp = (VkSampler*) cc_map_val(miter);
		vkk_engine_utilityUnlock(self);
		return samplerp;
	}

	VkFilter filter_map[VKK_SAMPLER_FILTER_COUNT] =
	{
		VK_FILTER_NEAREST,
		VK_FILTER_LINEAR
	};

	VkSamplerMipmapMode mipmap_map[VKK_SAMPLER_MIPMAP_MODE_COUNT] =
	{
		VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_MIPMAP_MODE_LINEAR
	};

	VkBool32 anisotropyEnable = VK_FALSE;
	if(si->anisotropy)
	{
		anisotropyEnable = VK_TRUE;
	}

	// Note: the maxLod represents the maximum number of mip
	// levels that can be supported and is just used to clamp
	// the computed maxLod for a particular texture.
	// A large value for maxLod effectively allows all mip
	// levels to be used for mipmapped textures.
	VkSamplerCreateInfo sci =
	{
		.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.pNext                   = NULL,
		.flags                   = 0,
		.magFilter               = filter_map[si->mag_filter],
		.minFilter               = filter_map[si->min_filter],
		.mipmapMode              = mipmap_map[si->mipmap_mode],
		.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.mipLodBias              = 0.0f,
		.anisotropyEnable        = anisotropyEnable,
		.maxAnisotropy           = si->max_anisotropy,
		.compareEnable           = VK_FALSE,
		.compareOp               = VK_COMPARE_OP_NEVER,
		.minLod                  = 0.0f,
		.maxLod                  = VK_LOD_CLAMP_NONE,
		.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
		.unnormalizedCoordinates = VK_FALSE
	};

	samplerp = (VkSampler*) CALLOC(1, sizeof(VkSampler));
	if(samplerp == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_alloc;
	}

	if(vkCreateSampler(self->device, &sci, NULL,
	                   samplerp) != VK_SUCCESS)
	{
		LOGE("vkCreateSampler failed");
		goto fail_create;
	}

	if(cc_map_addp(self->samplers, (const void*) samplerp,
	               sizeof(vkk_samplerInfo_t), si) == NULL)
	{
		goto fail_add;
	}

	vkk_engine_utilityUnlock(self);

	// succcess
	return samplerp;

	// failure
	fail_add:
		vkDestroySampler(self->device, *samplerp, NULL);
	fail_create:
		FREE(samplerp);
	fail_alloc:
		vkk_engine_utilityUnlock(self);
	return NULL;
}

void vkk_engine_usfLock(vkk_engine_t* self)
{
	ASSERT(self);

	pthread_mutex_lock(&self->usf_mutex);
	TRACE_BEGIN();
}

void vkk_engine_usfUnlock(vkk_engine_t* self)
{
	ASSERT(self);

	TRACE_END();
	pthread_mutex_unlock(&self->usf_mutex);
}

void vkk_engine_utilityLock(vkk_engine_t* self)
{
	ASSERT(self);

	pthread_mutex_lock(&self->utility_mutex);
	TRACE_BEGIN();
}

void vkk_engine_utilityUnlock(vkk_engine_t* self)
{
	ASSERT(self);

	TRACE_END();
	pthread_mutex_unlock(&self->utility_mutex);
}

void vkk_engine_rendererLock(vkk_engine_t* self)
{
	ASSERT(self);

	pthread_mutex_lock(&self->renderer_mutex);
	TRACE_BEGIN();
}

void vkk_engine_rendererUnlock(vkk_engine_t* self)
{
	ASSERT(self);

	TRACE_END();
	pthread_mutex_unlock(&self->renderer_mutex);
}

void vkk_engine_rendererSignal(vkk_engine_t* self)
{
	ASSERT(self);

	pthread_cond_broadcast(&self->renderer_cond);
}

void vkk_engine_rendererWait(vkk_engine_t* self)
{
	ASSERT(self);

	TRACE_END();
	pthread_cond_wait(&self->renderer_cond,
	                  &self->renderer_mutex);
	TRACE_BEGIN();
}

int vkk_engine_rendererCheckTimestamp(vkk_engine_t* self,
                                      double ts)
{
	ASSERT(self);

	vkk_renderer_t* renderer = self->renderer;

	// ignore zero
	if((renderer == NULL) || (ts == 0.0))
	{
		return 1;
	}

	// check timestamp status
	int status = 1;
	vkk_engine_rendererLock(self);
	if(vkk_defaultRenderer_tsExpiredLocked(renderer) < ts)
	{
		status = 0;
	}
	vkk_engine_rendererUnlock(self);

	return status;
}

void vkk_engine_rendererWaitForTimestamp(vkk_engine_t* self,
                                         double ts)
{
	ASSERT(self);

	vkk_renderer_t* renderer = self->renderer;

	// ignore zero
	if((renderer == NULL) || (ts == 0.0))
	{
		return;
	}

	// block until the renderer expires the timestamp
	vkk_engine_rendererLock(self);
	while(vkk_defaultRenderer_tsExpiredLocked(renderer) < ts)
	{
		if(self->shutdown)
		{
			break;
		}

		vkk_engine_rendererWait(self);
	}
	vkk_engine_rendererUnlock(self);
}

int vkk_engine_newSurface(vkk_engine_t* self)
{
	ASSERT(self);

	#ifdef ANDROID
		ANativeWindow* window = self->platform->app->window;
		if(window == NULL)
		{
			// ignore since window may be NULL when terminated
			return 0;
		}

		VkAndroidSurfaceCreateInfoKHR as_info =
		{
			.sType  = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
			.pNext  = NULL,
			.flags  = 0,
			.window = window
		};

		if(vkCreateAndroidSurfaceKHR(self->instance,
		                             &as_info, NULL,
		                             &self->surface) != VK_SUCCESS)
		{
			LOGE("vkCreateAndroidSurfaceKHR failed");
			return 0;
		}

		// check if surface is still supported by the device
		// when recreated
		if(self->device != VK_NULL_HANDLE)
		{
			VkBool32 supported = VK_FALSE;
			if(vkGetPhysicalDeviceSurfaceSupportKHR(self->physical_device,
			                                        self->queue_family_index,
			                                        self->surface,
			                                        &supported) != VK_SUCCESS)
			{
				LOGE("vkGetPhysicalDeviceSurfaceSupportKHR failed");
				vkk_engine_deleteSurface(self);
				return 0;
			}

			if(supported == VK_FALSE)
			{
				LOGE("surface is unsupported");
				vkk_engine_deleteSurface(self);
				return 0;
			}
		}
	#else
		if(vkk_engine_noDisplay())
		{
			// skip create surface
		}
		else
		{
			if(SDL_Vulkan_CreateSurface(self->window, self->instance,
			                            &self->surface) == SDL_FALSE)
			{
				LOGE("SDL_Vulkan_CreateSurface failed: %s",
				     SDL_GetError());
				return 0;
			}
		}
	#endif

	return 1;
}

void vkk_engine_deleteSurface(vkk_engine_t* self)
{
	ASSERT(self);

	vkDestroySurfaceKHR(self->instance,
	                    self->surface, NULL);
	self->surface = VK_NULL_HANDLE;
}

void
vkk_engine_deleteDefaultDepthImage(vkk_engine_t* self,
                                   vkk_image_t** _image)
{
	ASSERT(self);
	ASSERT(_image);

	vkk_engine_destructImage(self, 0, _image);
}

void
vkk_engine_deleteObject(vkk_engine_t* self,
                        vkk_objectType_e type,
                        void* obj)
{
	ASSERT(self);
	ASSERT(obj);

	vkk_object_t* object;
	object = vkk_object_new(type, obj);
	if(object == NULL)
	{
		goto fail_object;
	}

	if(cc_jobq_run(self->jobq_destruct, (void*) object) == 0)
	{
		goto fail_run;
	}

	// success
	return;

	// failure
	// destruct immediately but wait for idle if necessary
	fail_run:
		vkk_object_delete(&object);
	fail_object:
	{
		if(type == VKK_OBJECT_TYPE_RENDERER)
		{
			vkk_engine_destructRenderer(self, 0,
			                            (vkk_renderer_t**) &obj);
		}
		else if(type == VKK_OBJECT_TYPE_COMPUTE)
		{
			vkk_engine_destructCompute(self, 0,
			                           (vkk_compute_t**) &obj);
		}
		else if(type == VKK_OBJECT_TYPE_BUFFER)
		{
			vkk_engine_destructBuffer(self, 0,
			                          (vkk_buffer_t**) &obj);
		}
		else if(type == VKK_OBJECT_TYPE_IMAGE)
		{
			vkk_engine_destructImage(self, 0,
			                         (vkk_image_t**) &obj);
		}
		else if(type == VKK_OBJECT_TYPE_UNIFORMSETFACTORY)
		{
			vkk_engine_destructUniformSetFactory(self,
			                                     (vkk_uniformSetFactory_t**) &obj);
		}
		else if(type == VKK_OBJECT_TYPE_UNIFORMSET)
		{
			vkk_engine_destructUniformSet(self, 0,
			                              (vkk_uniformSet_t**) &obj);
		}
		else if(type == VKK_OBJECT_TYPE_PIPELINELAYOUT)
		{
			vkk_engine_destructPipelineLayout(self,
			                                  (vkk_pipelineLayout_t**) &obj);
		}
		else if(type == VKK_OBJECT_TYPE_GRAPHICSPIPELINE)
		{
			vkk_engine_destructGraphicsPipeline(self, 0,
			                                    (vkk_graphicsPipeline_t**) &obj);
		}
		else if(type == VKK_OBJECT_TYPE_COMPUTEPIPELINE)
		{
			vkk_engine_destructComputePipeline(self, 0,
			                                   (vkk_computePipeline_t**) &obj);
		}
		else
		{
			LOGE("invalid type=%i", type);
		}
	}
}
