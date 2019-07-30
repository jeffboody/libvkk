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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LOG_TAG "vkk"
#include "../libcc/cc_log.h"
#include "../libcc/cc_memory.h"
#include "../libpak/pak_file.h"
#include "vkk_defaultRenderer.h"
#include "vkk_engine.h"
#include "vkk_offscreenRenderer.h"
#include "vkk_util.h"

#define VKK_DESCRIPTOR_POOL_SIZE 64

/***********************************************************
* private - vkk_image_t                                    *
***********************************************************/

static size_t
vkk_image_size(vkk_image_t* self)
{
	assert(self);

	size_t bpp[VKK_IMAGE_FORMAT_COUNT] =
	{
		2, // VKK_IMAGE_FORMAT_RGBA4444
		2, // VKK_IMAGE_FORMAT_RGB565
		2, // VKK_IMAGE_FORMAT_RGBA5551
		1, // VKK_IMAGE_FORMAT_R8
		2, // VKK_IMAGE_FORMAT_RG88
		3, // VKK_IMAGE_FORMAT_RGB888
		4, // VKK_IMAGE_FORMAT_RGBA8888
		4, // VKK_IMAGE_FORMAT_DEPTH
	};

	return self->width*self->height*bpp[self->format];
}

/***********************************************************
* private                                                  *
***********************************************************/

static uint32_t
vkk_engine_swapchainImageCount(vkk_engine_t* self)
{
	assert(self);

	return vkk_defaultRenderer_swapchainImageCount(self->renderer);
}

static int
vkk_engine_hasDeviceExtensions(vkk_engine_t* self,
                               uint32_t count,
                               const char** names)
{
	assert(self);
	assert(count > 0);
	assert(names);

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
vkk_engine_getMemoryTypeIndex(vkk_engine_t* self,
                              uint32_t mt_bits,
                              VkFlags mp_flags,
                              uint32_t* mt_index)
{
	assert(self);
	assert(mt_index);

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

	LOGE("invalid memory type");
	return 0;
}

#ifndef ANDROID
static int
vkk_engine_initSDL(vkk_engine_t* self, const char* app_name)
{
	assert(self);
	assert(app_name);

	SDL_version version;
	SDL_VERSION(&version);
	LOGI("SDL %i.%i.%i",
	     version.major, version.minor, version.patch);

	if(SDL_Init(SDL_INIT_VIDEO) < 0)
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
		if(fscanf(f, "%i %i %i",
		          &width, &height, &fullscreen) != 3)
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
#endif

static int
vkk_engine_newInstance(vkk_engine_t* self,
                       const char* app_name,
                       uint32_t app_version)
{
	assert(self);
	assert(app_name);

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

	VkApplicationInfo a_info =
	{
		.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext              = NULL,
		.pApplicationName   = app_name,
		.applicationVersion = app_version,
		.pEngineName        = app_name,
		.engineVersion      = app_version,
		.apiVersion         = VK_MAKE_VERSION(1,0,0),
	};

	#ifndef ANDROID
		#define DEBUG_LAYERS
	#endif
	#if defined(DEBUG_LAYERS)
		uint32_t layer_count = 1;
		const char* layer_names[] =
		{
			"VK_LAYER_LUNARG_standard_validation"
		};
	#elif defined(DEBUG_DUMP)
		uint32_t layer_count = 2;
		const char* layer_names[] =
		{
			"VK_LAYER_LUNARG_standard_validation",
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

static int vkk_engine_newSurface(vkk_engine_t* self)
{
	assert(self);

	#ifdef ANDROID
		VkAndroidSurfaceCreateInfoKHR as_info =
		{
			.sType  = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
			.pNext  = NULL,
			.flags  = 0,
			.window = self->app->window
		};

		if(vkCreateAndroidSurfaceKHR(self->instance,
		                             &as_info, NULL,
		                             &self->surface) != VK_SUCCESS)
		{
			LOGE("vkCreateAndroidSurfaceKHR failed");
			return 0;
		}
	#else
		if(SDL_Vulkan_CreateSurface(self->window, self->instance,
		                            &self->surface) == SDL_FALSE)
		{
			LOGE("SDL_Vulkan_CreateSurface failed: %s", SDL_GetError());
			return 0;
		}
	#endif

	return 1;
}

static int vkk_engine_getPhysicalDevice(vkk_engine_t* self)
{
	assert(self);

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

	return 1;
}

static int vkk_engine_newDevice(vkk_engine_t* self)
{
	assert(self);

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

	int i;
	int has_index = 0;
	self->queue_family_index = 0;
	for(i = 0; i < qfp_count; ++i)
	{
		if(qfp[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			VkBool32 supported;
			if(vkGetPhysicalDeviceSurfaceSupportKHR(self->physical_device,
			                                        i, self->surface,
			                                        &supported) != VK_SUCCESS)
			{
				LOGW("vkGetPhysicalDeviceSurfaceSupportKHR failed");
				continue;
			}

			if(supported && (has_index == 0))
			{
				// select the first supported device queue
				self->queue_family_index = i;
				has_index = 1;
			}
		}
	}

	if(has_index == 0)
	{
		LOGE("vkGetPhysicalDeviceQueueFamilyProperties failed");
		goto fail_select_qfp;
	}

	float queue_priority = 0.0f;
	VkDeviceQueueCreateInfo dqc_info =
	{
		.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext            = NULL,
		.flags            = 0,
		.queueFamilyIndex = self->queue_family_index,
		.queueCount       = 1,
		.pQueuePriorities = &queue_priority
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
		.pEnabledFeatures        = NULL
	};

	if(vkCreateDevice(self->physical_device, &dc_info,
	                  NULL, &self->device) != VK_SUCCESS)
	{
		LOGE("vkCreateDevice failed");
		goto fail_create_device;
	}

	vkGetDeviceQueue(self->device,
	                 self->queue_family_index,
                     0, &self->queue);

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
	assert(self);

	*_size = 0;
	*_data = NULL;

	// ignore if cache doesn't exist
	if(access(self->cache, F_OK) != 0)
	{
		return;
	}

	FILE* f = fopen(self->cache, "r");
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
	assert(self);

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

	FILE* f = fopen(self->cache, "w");
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

static int vkk_engine_newCacheAndPools(vkk_engine_t* self)
{
	assert(self);

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

	VkCommandPoolCreateInfo cpc_info =
	{
		.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext            = NULL,
		.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = self->queue_family_index
	};

	if(vkCreateCommandPool(self->device, &cpc_info, NULL,
	                       &self->command_pool) != VK_SUCCESS)
	{
		LOGE("vkCreateCommandPool failed");
		goto fail_command_pool;
	}

	// success
	return 1;

	// failure
	fail_command_pool:
		vkDestroyPipelineCache(self->device,
		                       self->pipeline_cache, NULL);
	fail_pipeline_cache:
		FREE(data);
	return 0;
}

static uint32_t*
vkk_engine_importShaderModule(vkk_engine_t* self,
                              const char* fname,
                              size_t* _size)
{
	assert(self);
	assert(fname);
	assert(_size);

	pak_file_t* pak;
	pak = pak_file_open(self->resource, PAK_FLAG_READ);
	if(pak == NULL)
	{
		return NULL;
	}

	size_t size = (size_t) pak_file_seek(pak, fname);
	if((size == 0) || ((size % 4) != 0))
	{
		LOGE("invalid fname=%s, size=%u", fname, (unsigned int) size);
		goto fail_size;
	}

	uint32_t* code;
	code = (uint32_t*)
	       CALLOC(size/4, sizeof(uint32_t));
	if(code == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_alloc;
	}

	if(fread((void*) code, size, 1, pak->f) != 1)
	{
		LOGE("fread failed");
		goto fail_code;
	}

	pak_file_close(&pak);

	*_size = size;

	// success
	return code;

	// failure
	fail_code:
		FREE(code);
	fail_alloc:
	fail_size:
		pak_file_close(&pak);
	return NULL;
}

static VkShaderModule
vkk_engine_getShaderModule(vkk_engine_t* self,
                           const char* fname)
{
	assert(self);
	assert(fname);

	vkk_engine_smLock(self);

	cc_mapIter_t miter;
	VkShaderModule sm;
	sm = (VkShaderModule)
	     cc_map_find(self->shader_modules, &miter, fname);
	if(sm != VK_NULL_HANDLE)
	{
		vkk_engine_smUnlock(self);
		return sm;
	}

	size_t    size = 0;
	uint32_t* code;
	code = vkk_engine_importShaderModule(self, fname, &size);
	if(code == NULL)
	{
		vkk_engine_smUnlock(self);
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
		vkk_engine_smUnlock(self);
		LOGE("vkCreateShaderModule failed");
		goto fail_create;
	}

	if(cc_map_add(self->shader_modules, (const void*) sm,
	              fname) == 0)
	{
		vkk_engine_smUnlock(self);
		goto fail_add;
	}

	FREE(code);

	vkk_engine_smUnlock(self);

	// success
	return sm;

	// failure
	fail_add:
		vkDestroyShaderModule(self->device, sm, NULL);
	fail_create:
		FREE(code);
	return VK_NULL_HANDLE;
}

static VkDescriptorPool
vkk_engine_newDescriptorPoolLocked(vkk_engine_t* self,
                                   vkk_uniformSetFactory_t* usf)
{
	assert(self);
	assert(usf);

	VkDescriptorType dt_map[VKK_UNIFORM_TYPE_COUNT] =
	{
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
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

static int
vkk_engine_uploadImage(vkk_engine_t* self,
                       vkk_image_t* image,
                       const void* pixels)
{
	assert(self);
	assert(image);
	assert(pixels);

	VkFenceCreateInfo f_info =
	{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0
	};

	VkFence fence;
	if(vkCreateFence(self->device, &f_info, NULL,
	                 &fence) != VK_SUCCESS)
	{
		LOGE("vkCreateFence failed");
		return 0;
	}

	// create a transfer buffer
	size_t size = vkk_image_size(image);
	VkBufferCreateInfo b_info =
	{
		.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext                 = NULL,
		.flags                 = 0,
		.size                  = size,
		.usage                 = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		.sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices   = &self->queue_family_index
	};

	VkBuffer buffer;
	if(vkCreateBuffer(self->device, &b_info, NULL,
	                  &buffer) != VK_SUCCESS)
	{
		LOGE("vkCreateBuffer failed");
		goto fail_buffer;
	}

	// allocate memory for the transfer buffer
	VkMemoryRequirements mr;
	vkGetBufferMemoryRequirements(self->device, buffer, &mr);

	VkFlags mp_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
	                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	uint32_t mt_index;
	if(vkk_engine_getMemoryTypeIndex(self,
	                                 mr.memoryTypeBits,
	                                 mp_flags,
	                                 &mt_index) == 0)
	{
		goto fail_memory_type;
	}

	VkMemoryAllocateInfo ma_info =
	{
		.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext           = NULL,
		.allocationSize  = mr.size,
		.memoryTypeIndex = mt_index
	};

	VkDeviceMemory memory;
	if(vkAllocateMemory(self->device, &ma_info, NULL,
	                    &memory) != VK_SUCCESS)
	{
		LOGE("vkAllocateMemory failed");
		goto fail_allocate_memory;
	}

	// copy pixels into transfer buffer memory
	void* data;
	if(vkMapMemory(self->device, memory, 0, mr.size, 0,
	               &data) != VK_SUCCESS)
	{
		LOGE("vkMapMemory failed");
		goto fail_map;
	}
	memcpy(data, pixels, size);
	vkUnmapMemory(self->device, memory);

	if(vkBindBufferMemory(self->device, buffer, memory,
	                      0) != VK_SUCCESS)
	{
		LOGE("vkBindBufferMemory failed");
		goto fail_bind;
	}

	// allocate a transfer command buffer
	VkCommandBuffer cb;
	if(vkk_engine_allocateCommandBuffers(self, 1, &cb) == 0)
	{
		goto fail_allocate_cb;
	}

	// begin the transfer commands
	VkCommandBufferInheritanceInfo cbi_info =
	{
		.sType                = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		.pNext                = NULL,
		.renderPass           = VK_NULL_HANDLE,
		.subpass              = 0,
		.framebuffer          = VK_NULL_HANDLE,
		.occlusionQueryEnable = VK_FALSE,
		.queryFlags           = 0,
		.pipelineStatistics   = 0
	};

	VkCommandBufferBeginInfo cb_info =
	{
		.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext            = NULL,
		.flags            = 0,
		.pInheritanceInfo = &cbi_info
	};

	vkk_engine_cmdLock(self);
	if(vkBeginCommandBuffer(cb, &cb_info) != VK_SUCCESS)
	{
		LOGE("vkBeginCommandBuffer failed");
		vkk_engine_cmdUnlock(self);
		goto fail_begin_cb;
	}

	// transition the image to copy the transfer buffer to
	// the image and generate mip levels if needed
	vkk_util_imageMemoryBarrier(image, cb,
	                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	                            0, image->mip_levels);

	// copy the transfer buffer to the image
	VkBufferImageCopy bic =
	{
		.bufferOffset      = 0,
		.bufferRowLength   = 0,
		.bufferImageHeight = 0,
		.imageSubresource  =
		{
			.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel       = 0,
			.baseArrayLayer = 0,
			.layerCount     = 1
		},
		.imageOffset =
		{
			.x = 0,
			.y = 0,
			.z = 0,
		},
		.imageExtent =
		{
			.width  = image->width,
			.height = image->height,
			.depth  = 1
		}
	};

	vkCmdCopyBufferToImage(cb, buffer, image->image,
	                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	                       1, &bic);

	// at this point we may need to generate mip_levels if
	// mipmapping was enabled
	if(image->mip_levels > 1)
	{
		vkk_engine_mipmapImage(self, image, cb);
	}

	// transition the image from transfer mode to shading mode
	vkk_util_imageMemoryBarrier(image, cb,
	                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	                            0, image->mip_levels);

	// end the transfer commands
	vkEndCommandBuffer(cb);
	vkk_engine_cmdUnlock(self);

	// submit the commands
	if(vkk_engine_queueSubmit(self, &cb,
	                          NULL, NULL, NULL,
	                          fence) == 0)
	{
		goto fail_submit;
	}

	uint64_t timeout = UINT64_MAX;
	if(vkWaitForFences(self->device, 1, &fence, VK_TRUE,
	                   timeout) != VK_SUCCESS)
	{
		LOGW("vkWaitForFences failed");
		vkk_engine_queueWaitIdle(self);
	}

	// release temporary objects
	vkk_engine_freeCommandBuffers(self, 1, &cb);
	vkFreeMemory(self->device, memory, NULL);
	vkDestroyBuffer(self->device, buffer, NULL);
	vkDestroyFence(self->device, fence, NULL);

	// success
	return 1;

	// failure
	fail_submit:
	fail_begin_cb:
		vkk_engine_freeCommandBuffers(self, 1, &cb);
	fail_allocate_cb:
	fail_bind:
	fail_map:
		vkFreeMemory(self->device, memory, NULL);
	fail_allocate_memory:
	fail_memory_type:
		vkDestroyBuffer(self->device, buffer, NULL);
	fail_buffer:
		vkDestroyFence(self->device, fence, NULL);
	return 0;
}

static void
vkk_engine_attachUniformBuffer(vkk_engine_t* self,
                               vkk_uniformSet_t* us,
                               vkk_buffer_t* buffer,
                               uint32_t binding)
{
	assert(self);
	assert(us);
	assert(buffer);

	uint32_t count;
	count = us->usf->dynamic ?
	        vkk_engine_swapchainImageCount(self) : 1;

	int i;
	for(i = 0; i < count; ++i)
	{
		uint32_t idx = buffer->dynamic ? i : 0;
		VkDescriptorBufferInfo db_info =
		{
			.buffer  = buffer->buffer[idx],
			.offset  = 0,
			.range   = buffer->size
		};

		VkWriteDescriptorSet writes =
		{
			.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext            = NULL,
			.dstSet           = us->ds_array[i],
			.dstBinding       = binding,
			.dstArrayElement  = 0,
			.descriptorCount  = 1,
			.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pImageInfo       = NULL,
			.pBufferInfo      = &db_info,
			.pTexelBufferView = NULL,
		};

		vkUpdateDescriptorSets(self->device, 1, &writes,
		                       0, NULL);
	}
}

static void
vkk_engine_attachUniformSampler(vkk_engine_t* self,
                                vkk_uniformSet_t* us,
                                vkk_sampler_t* sampler,
                                vkk_image_t* image,
                                uint32_t binding)
{
	assert(self);
	assert(us);
	assert(sampler);
	assert(image);

	uint32_t count;
	count = us->usf->dynamic ?
	        vkk_engine_swapchainImageCount(self) : 1;

	int i;
	for(i = 0; i < count; ++i)
	{
		VkDescriptorImageInfo di_info =
		{
			.sampler     = sampler->sampler,
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

/***********************************************************
* engine new/delete API                                    *
***********************************************************/

vkk_engine_t* vkk_engine_new(void* app,
                             const char* app_name,
                             uint32_t app_version,
                             const char* resource,
                             const char* cache)
{
	#ifdef ANDROID
		assert(app);
	#else
		assert(app == NULL);
	#endif
	assert(app_name);
	assert(resource);
	assert(cache);

	vkk_engine_t* self;
	self = (vkk_engine_t*)
	       CALLOC(1, sizeof(vkk_engine_t));
	if(self == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	#ifdef ANDROID
		self->app = (struct android_app*) app;
	#else
		if(vkk_engine_initSDL(self, app_name) == 0)
		{
			FREE(self);
			return NULL;
		}
	#endif

	snprintf(self->resource, 256, "%s", resource);
	snprintf(self->cache, 256, "%s", cache);

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

	if(pthread_mutex_init(&self->sm_mutex, NULL) != 0)
	{
		LOGE("pthread_mutex_init failed");
		goto fail_sm_mutex;
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

	if(vkk_engine_newCacheAndPools(self) == 0)
	{
		goto fail_cacheAndPools;
	}

	self->shader_modules = cc_map_new();
	if(self->shader_modules == NULL)
	{
		goto fail_shader_modules;
	}

	self->renderer = vkk_defaultRenderer_new(self);
	if(self->renderer == NULL)
	{
		goto fail_renderer;
	}

	// success
	return self;

	// failure
	fail_renderer:
		cc_map_delete(&self->shader_modules);
	fail_shader_modules:
		vkDestroyCommandPool(self->device,
		                     self->command_pool, NULL);
		vkDestroyPipelineCache(self->device,
		                       self->pipeline_cache, NULL);
	fail_cacheAndPools:
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
		pthread_mutex_destroy(&self->sm_mutex);
	fail_sm_mutex:
		pthread_mutex_destroy(&self->usf_mutex);
	fail_usf_mutex:
		pthread_mutex_destroy(&self->cmd_mutex);
	fail_cmd_mutex:
		#ifndef ANDROID
			SDL_DestroyWindow(self->window);
			SDL_Quit();
		#endif
		FREE(self);
	return NULL;
}

void vkk_engine_delete(vkk_engine_t** _self)
{
	// *_self can be null
	assert(_self);

	vkk_engine_t* self = *_self;
	if(self)
	{
		assert(self->shutdown);

		vkk_defaultRenderer_delete(&self->renderer);

		cc_mapIter_t  miterator;
		cc_mapIter_t* miter;
		miter = cc_map_head(self->shader_modules, &miterator);
		while(miter)
		{
			VkShaderModule sm;
			sm = (VkShaderModule)
			     cc_map_remove(self->shader_modules, &miter);
			vkDestroyShaderModule(self->device, sm, NULL);
		}
		cc_map_delete(&self->shader_modules);
		vkDestroyCommandPool(self->device,
		                     self->command_pool, NULL);
		vkk_engine_exportPipelineCache(self);
		vkDestroyPipelineCache(self->device,
		                       self->pipeline_cache, NULL);
		vkDestroyDevice(self->device, NULL);
		vkDestroySurfaceKHR(self->instance,
		                    self->surface, NULL);
		vkDestroyInstance(self->instance, NULL);
		pthread_mutex_destroy(&self->sm_mutex);
		pthread_mutex_destroy(&self->usf_mutex);
		pthread_mutex_destroy(&self->cmd_mutex);
		#ifndef ANDROID
			SDL_DestroyWindow(self->window);
			SDL_Quit();
		#endif
		FREE(self);
		*_self = NULL;
	}
}

void vkk_engine_shutdown(vkk_engine_t* self)
{
	assert(self);

	vkk_engine_rendererLock(self);
	vkDeviceWaitIdle(self->device);
	self->shutdown = 1;
	vkk_engine_rendererSignal(self);
	vkk_engine_rendererUnlock(self);
}

vkk_renderer_t*
vkk_engine_newRenderer(vkk_engine_t* self,
                       uint32_t width, uint32_t height,
                       int format)
{
	assert(self);

	return vkk_offscreenRenderer_new(self, width, height,
	                                 format);
}

void vkk_engine_deleteRenderer(vkk_engine_t* self,
                               vkk_renderer_t** _renderer)
{
	assert(self);
	assert(_renderer);

	// do not delete default renderer
	vkk_renderer_t* renderer = *_renderer;
	if(self->renderer == renderer)
	{
		return;
	}

	vkk_offscreenRenderer_delete(_renderer);
}

vkk_buffer_t*
vkk_engine_newBuffer(vkk_engine_t* self, int dynamic,
                     int usage, size_t size,
                     const void* buf)
{
	// buf may be NULL
	assert(self);

	uint32_t count;
	count = dynamic ?
	        vkk_engine_swapchainImageCount(self) : 1;

	VkBufferUsageFlags usage_flags;
	if(usage == VKK_BUFFER_USAGE_UNIFORM)
	{
		usage_flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	}
	else if(usage == VKK_BUFFER_USAGE_VERTEX)
	{
		usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	}
	else
	{
		LOGE("invalid usage=%i", usage);
		return NULL;
	}

	vkk_buffer_t* buffer;
	buffer = (vkk_buffer_t*) CALLOC(1, sizeof(vkk_buffer_t));
	if(buffer == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	buffer->dynamic = dynamic;
	buffer->size    = size;

	buffer->buffer = (VkBuffer*)
	                 CALLOC(count, sizeof(VkBuffer));
	if(buffer->buffer == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_buffer;
	}

	buffer->memory = (VkDeviceMemory*)
	                 CALLOC(count, sizeof(VkDeviceMemory));
	if(buffer->memory == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_memory;
	}

	VkBufferCreateInfo b_info =
	{
		.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext                 = NULL,
		.flags                 = 0,
		.size                  = size,
		.usage                 = usage_flags,
		.sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices   = &self->queue_family_index
	};

	int i;
	for(i = 0; i < count; ++i)
	{
		if(vkCreateBuffer(self->device, &b_info, NULL,
		                  &buffer->buffer[i]) != VK_SUCCESS)
		{
			LOGE("vkCreateBuffer failed");
			goto fail_create_buffer;
		}

		VkMemoryRequirements mr;
		vkGetBufferMemoryRequirements(self->device,
		                              buffer->buffer[i],
		                              &mr);

		VkFlags mp_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		uint32_t mt_index;
		if(vkk_engine_getMemoryTypeIndex(self,
		                                 mr.memoryTypeBits,
		                                 mp_flags,
		                                 &mt_index) == 0)
		{
			goto fail_memory_type;
		}

		VkMemoryAllocateInfo ma_info =
		{
			.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext           = NULL,
			.allocationSize  = mr.size,
			.memoryTypeIndex = mt_index
		};

		if(vkAllocateMemory(self->device, &ma_info, NULL,
		                    &buffer->memory[i]) != VK_SUCCESS)
		{
			LOGE("vkAllocateMemory failed");
			goto fail_allocate;
		}

		if(buf)
		{
			void* data;
			if(vkMapMemory(self->device, buffer->memory[i],
			               0, mr.size, 0, &data) != VK_SUCCESS)
			{
				LOGE("vkMapMemory failed");
				goto fail_map;
			}
			memcpy(data, buf, size);
			vkUnmapMemory(self->device, buffer->memory[i]);
		}

		if(vkBindBufferMemory(self->device,
		                      buffer->buffer[i],
		                      buffer->memory[i], 0) != VK_SUCCESS)
		{
			LOGE("vkBindBufferMemory failed");
			goto fail_bind;
		}
	}

	// success
	return buffer;

	// failure
	fail_bind:
	fail_map:
	fail_allocate:
	fail_memory_type:
	fail_create_buffer:
	{
		int j;
		for(j = 0; j <= i; ++j)
		{
			vkFreeMemory(self->device,
			             buffer->memory[j],
			             NULL);
			vkDestroyBuffer(self->device,
			                buffer->buffer[j],
			                NULL);
		}
		FREE(buffer->memory);
	}
	fail_memory:
		FREE(buffer->buffer);
	fail_buffer:
		FREE(buffer);
	return NULL;
}

void vkk_engine_deleteBuffer(vkk_engine_t* self,
                             vkk_buffer_t** _buffer)
{
	assert(self);
	assert(_buffer);

	vkk_buffer_t* buffer = *_buffer;
	if(buffer)
	{
		vkk_engine_rendererWaitForTimestamp(self, buffer->ts);

		uint32_t count;
		count = buffer->dynamic ?
		        vkk_engine_swapchainImageCount(self) : 1;
		int i;
		for(i = 0; i < count; ++i)
		{
			vkFreeMemory(self->device,
			             buffer->memory[i],
			             NULL);
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

vkk_image_t* vkk_engine_newImage(vkk_engine_t* self,
                                 uint32_t width,
                                 uint32_t height,
                                 int format,
                                 int mipmap,
                                 int stage,
                                 const void* pixels)
{
	// pixels may be NULL for depth image or
	// offscreen rendering
	assert(self);

	// check if mipmapped images are a power-of-two
	// and compute the mip_levels
	uint32_t mip_levels = 1;
	if(mipmap)
	{
		// mipmap does not apply to the depth image
		assert(format != VKK_IMAGE_FORMAT_DEPTH);

		uint32_t w = 1;
		uint32_t h = 1;
		uint32_t n = 1;
		uint32_t m = 1;

		while(w < width)
		{
			w *= 2;
			n += 1;
		}

		while(h < height)
		{
			h *= 2;
			m += 1;
		}

		if((w != width) || (h != height))
		{
			LOGE("invalid width=%u, height=%u", width, height);
			return NULL;
		}

		mip_levels = (m > n) ? m : n;
	}

	vkk_image_t* image;
	image = (vkk_image_t*) CALLOC(1, sizeof(vkk_image_t));
	if(image == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	image->layout_array = (VkImageLayout*)
	                      CALLOC(mip_levels, sizeof(VkImageLayout));
	if(image->layout_array == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_layout_array;
	}

	image->width      = width;
	image->height     = height;
	image->format     = format;
	image->stage      = stage;
	image->mip_levels = mip_levels;

	// initialize the image layout
	int i;
	for(i = 0; i < mip_levels; ++i)
	{
		image->layout_array[i] = VK_IMAGE_LAYOUT_UNDEFINED;
	}

	VkFormat format_map[VKK_IMAGE_FORMAT_COUNT] =
	{
		VK_FORMAT_R4G4B4A4_UNORM_PACK16,
		VK_FORMAT_R5G6B5_UNORM_PACK16,
		VK_FORMAT_R5G5B5A1_UNORM_PACK16,
		VK_FORMAT_R8_UNORM,
		VK_FORMAT_R8G8_UNORM,
		VK_FORMAT_R8G8B8_UNORM,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_D24_UNORM_S8_UINT,
	};

	VkImageUsageFlags  usage      = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	                                VK_IMAGE_USAGE_SAMPLED_BIT;
	VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	if(format == VKK_IMAGE_FORMAT_DEPTH)
	{
		usage      = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT |
		             VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else
	{
		if(mip_levels > 1)
		{
			// mip levels are generated iteratively by blitting
			usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}

		if(pixels == NULL)
		{
			// enable render-to-texture
			usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}
	}

	VkImageCreateInfo i_info =
	{
		.sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext       = NULL,
		.flags       = 0,
		.imageType   = VK_IMAGE_TYPE_2D,
		.format      = format_map[format],
		.extent      =
		{
			width,
			height,
			1
		},
		.mipLevels   = mip_levels,
		.arrayLayers = 1,
		.samples     = VK_SAMPLE_COUNT_1_BIT,
		.tiling      = VK_IMAGE_TILING_OPTIMAL,
		.usage       = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices   = &self->queue_family_index,
		.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED
	};

	if(vkCreateImage(self->device, &i_info, NULL,
	                 &image->image) != VK_SUCCESS)
	{
		LOGE("vkCreateImage failed");
		goto fail_create_image;
	}

	VkMemoryRequirements mr;
	vkGetImageMemoryRequirements(self->device,
	                             image->image,
	                             &mr);

	VkFlags  mp_flags = 0;
	uint32_t mt_index;
	if(vkk_engine_getMemoryTypeIndex(self,
	                                 mr.memoryTypeBits,
	                                 mp_flags,
	                                 &mt_index) == 0)
	{
		goto fail_memory_type;
	}

	VkMemoryAllocateInfo ma_info =
	{
		.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext           = NULL,
		.allocationSize  = mr.size,
		.memoryTypeIndex = mt_index
	};

	if(vkAllocateMemory(self->device, &ma_info, NULL,
	                    &image->memory) != VK_SUCCESS)
	{
		LOGE("vkAllocateMemory failed");
		goto fail_allocate;
	}

	if(vkBindImageMemory(self->device, image->image,
	                     image->memory, 0) != VK_SUCCESS)
	{
		LOGE("vkBindBufferMemory failed");
		goto fail_bind;
	}

	VkImageViewCreateInfo iv_info =
	{
		.sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext      = NULL,
		.flags      = 0,
		.image      = image->image,
		.viewType   = VK_IMAGE_VIEW_TYPE_2D,
		.format     = format_map[format],
		.components =
		{
			.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.a = VK_COMPONENT_SWIZZLE_IDENTITY
		},
		.subresourceRange =
		{
			.aspectMask     = aspectMask,
			.baseMipLevel   = 0,
			.levelCount     = mip_levels,
			.baseArrayLayer = 0,
			.layerCount     = 1
		}
	};

	if(vkCreateImageView(self->device, &iv_info, NULL,
	                     &image->image_view) != VK_SUCCESS)
	{
		LOGE("vkCreateImageView failed");
		goto fail_image_view;
	}

	// upload pixel data
	if(pixels && (format != VKK_IMAGE_FORMAT_DEPTH))
	{
		if(vkk_engine_uploadImage(self, image, pixels) == 0)
		{
			goto fail_upload;
		}
	}

	// success
	return image;

	// failure
	fail_upload:
		vkDestroyImageView(self->device, image->image_view,
		                   NULL);
	fail_image_view:
	fail_bind:
		vkFreeMemory(self->device,
		             image->memory, NULL);
	fail_allocate:
	fail_memory_type:
		vkDestroyImage(self->device,
		               image->image, NULL);
	fail_create_image:
		FREE(image->layout_array);
	fail_layout_array:
		FREE(image);
	return NULL;
}

void vkk_engine_deleteImage(vkk_engine_t* self,
                            vkk_image_t** _image)
{
	assert(self);
	assert(_image);

	vkk_image_t* image = *_image;
	if(image)
	{
		vkk_engine_rendererWaitForTimestamp(self, image->ts);

		vkDestroyImageView(self->device, image->image_view,
		                   NULL);
		vkFreeMemory(self->device, image->memory, NULL);
		vkDestroyImage(self->device, image->image, NULL);
		FREE(image->layout_array);
		FREE(image);
		*_image = NULL;
	}
}

vkk_sampler_t*
vkk_engine_newSampler(vkk_engine_t* self, int min_filter,
                      int mag_filter, int mipmap_mode)
{
	assert(self);

	vkk_sampler_t* sampler;
	sampler = (vkk_sampler_t*)
	          CALLOC(1, sizeof(vkk_sampler_t));
	if(sampler == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
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

	// Note: the maxLod represents the maximum number of mip
	// levels that can be supported and is just used to clamp
	// the computed maxLod for a particular texture.
	// A large value for maxLod effectively allows all mip
	// levels to be used for mipmapped textures.
	VkSamplerCreateInfo si =
	{
		.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.pNext                   = NULL,
		.flags                   = 0,
		.magFilter               = filter_map[mag_filter],
		.minFilter               = filter_map[min_filter],
		.mipmapMode              = mipmap_map[mipmap_mode],
		.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.mipLodBias              = 0.0f,
		.anisotropyEnable        = VK_FALSE,
		.maxAnisotropy           = 0.0f,
		.compareEnable           = VK_FALSE,
		.compareOp               = VK_COMPARE_OP_NEVER,
		.minLod                  = 0.0f,
		.maxLod                  = 1024.0f,
		.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
		.unnormalizedCoordinates = VK_FALSE
	};

	if(vkCreateSampler(self->device, &si, NULL,
	                   &sampler->sampler) != VK_SUCCESS)
	{
		LOGE("vkCreateSampler failed");
		goto fail_create;
	}

	// success
	return sampler;

	// failure
	fail_create:
		FREE(sampler);
	return NULL;
}

void vkk_engine_deleteSampler(vkk_engine_t* self,
                              vkk_sampler_t** _sampler)
{
	assert(self);
	assert(_sampler);

	vkk_sampler_t* sampler = *_sampler;
	if(sampler)
	{
		vkk_engine_rendererWaitForTimestamp(self, sampler->ts);

		vkDestroySampler(self->device, sampler->sampler, NULL);
		FREE(sampler);
		*_sampler = NULL;
	}
}

vkk_uniformSetFactory_t*
vkk_engine_newUniformSetFactory(vkk_engine_t* self,
                                int dynamic,
                                uint32_t ub_count,
                                vkk_uniformBinding_t* ub_array)
{
	assert(self);
	assert(ub_array);

	VkDescriptorType dt_map[VKK_UNIFORM_TYPE_COUNT] =
	{
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	};

	VkShaderStageFlags ss_map[VKK_STAGE_COUNT] =
	{
		0,
		VK_SHADER_STAGE_VERTEX_BIT,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
	};

	vkk_uniformSetFactory_t* usf;
	usf = (vkk_uniformSetFactory_t*)
	      CALLOC(1, sizeof(vkk_uniformSetFactory_t));
	if(usf == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	usf->dynamic  = dynamic;
	usf->ub_count = ub_count;

	// copy the ub_array
	usf->ub_array = (vkk_uniformBinding_t*)
	                CALLOC(ub_count,
	                       sizeof(vkk_uniformBinding_t));
	if(usf->ub_array == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_ub_array;
	}
	memcpy(usf->ub_array, ub_array,
	       ub_count*sizeof(vkk_uniformBinding_t));

	// create temportary descriptor set layout bindings
	VkDescriptorSetLayoutBinding* bindings;
	bindings = (VkDescriptorSetLayoutBinding*)
	           CALLOC(ub_count,
	                  sizeof(VkDescriptorSetLayoutBinding));
	if(bindings == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_bindings;
	}

	// fill in bindings
	int i;
	for(i = 0; i < ub_count; ++i)
	{
		vkk_uniformBinding_t*         usb = &(ub_array[i]);
		VkDescriptorSetLayoutBinding* b   = &(bindings[i]);
		b->binding            = usb->binding;
		b->descriptorType     = dt_map[usb->type];
		b->descriptorCount    = 1;
		b->stageFlags         = ss_map[usb->stage];
		b->pImmutableSamplers = usb->sampler ? &usb->sampler->sampler : NULL;
	}

	VkDescriptorSetLayoutCreateInfo dsl_info =
	{
		.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext        = NULL,
		.flags        = 0,
		.bindingCount = ub_count,
		.pBindings    = bindings,
	};

	if(vkCreateDescriptorSetLayout(self->device,
	                               &dsl_info, NULL,
	                               &usf->ds_layout) != VK_SUCCESS)
	{
		LOGE("vkCreateDescriptorSetLayout failed");
		goto fail_create_dsl;
	}

	usf->dp_list = cc_list_new();
	if(usf->dp_list == NULL)
	{
		goto fail_dp_list;
	}

	usf->us_list = cc_list_new();
	if(usf->us_list == NULL)
	{
		goto fail_us_list;
	}

	// increment type counter
	for(i = 0; i < ub_count; ++i)
	{
		++usf->type_count[ub_array[i].type];
	}

	FREE(bindings);

	// success
	return usf;

	// failure
	fail_us_list:
		cc_list_delete(&usf->dp_list);
	fail_dp_list:
		vkDestroyDescriptorSetLayout(self->device,
		                             usf->ds_layout, NULL);
	fail_create_dsl:
		FREE(bindings);
	fail_bindings:
		FREE(usf->ub_array);
	fail_ub_array:
		FREE(usf);
	return NULL;
}

void
vkk_engine_deleteUniformSetFactory(vkk_engine_t* self,
                                   vkk_uniformSetFactory_t** _usf)
{
	assert(self);
	assert(_usf);

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

vkk_uniformSet_t*
vkk_engine_newUniformSet(vkk_engine_t* self,
                         uint32_t set,
                         uint32_t ua_count,
                         vkk_uniformAttachment_t* ua_array,
                         vkk_uniformSetFactory_t* usf)
{
	assert(self);
	assert(ua_array);
	assert(usf);
	assert(ua_count == usf->ub_count);

	vkk_renderer_t* renderer = self->renderer;

	// get the last expired timestamp
	vkk_engine_rendererLock(self);
	double ets = vkk_defaultRenderer_tsExpiredLocked(renderer);
	vkk_engine_rendererUnlock(self);

	// check if a uniform set can be reused
	vkk_engine_usfLock(self);
	vkk_uniformSet_t* us   = NULL;
	cc_listIter_t*    iter = cc_list_head(usf->us_list);
	while(iter)
	{
		vkk_uniformSet_t* tmp;
		tmp = (vkk_uniformSet_t*)
		      cc_list_peekIter(iter);

		if(ets >= tmp->ts)
		{
			us = tmp;
			cc_list_remove(usf->us_list, &iter);
			break;
		}

		iter = cc_list_next(iter);
	}
	vkk_engine_usfUnlock(self);

	int i;
	if(us == NULL)
	{
		// create a new uniform set
		us = (vkk_uniformSet_t*)
		     CALLOC(1, sizeof(vkk_uniformSet_t));
		if(us == NULL)
		{
			LOGE("CALLOC failed");
			return NULL;
		}

		us->set      = set;
		us->ua_count = ua_count;
		us->usf      = usf;

		// copy the ua_array
		us->ua_array = (vkk_uniformAttachment_t*)
		               CALLOC(ua_count,
		                      sizeof(vkk_uniformAttachment_t));
		if(us->ua_array == NULL)
		{
			LOGE("CALLOC failed");
			goto fail_ua_array;
		}
		memcpy(us->ua_array, ua_array,
		       ua_count*sizeof(vkk_uniformAttachment_t));

		uint32_t ds_count;
		ds_count = usf->dynamic ?
		           vkk_engine_swapchainImageCount(self) : 1;
		us->ds_array = (VkDescriptorSet*)
		               CALLOC(ds_count, sizeof(VkDescriptorSet));
		if(us->ds_array == NULL)
		{
			LOGE("CALLOC failed");
			goto fail_ds_array;
		}

		// initialize the descriptor set layouts
		VkDescriptorSetLayout dsl_array[VKK_DESCRIPTOR_POOL_SIZE];
		for(i = 0; i < ds_count; ++i)
		{
			dsl_array[i] = usf->ds_layout;
		}

		// allocate the descriptor set from the pool
		vkk_engine_usfLock(self);
		VkDescriptorPool dp;
		dp = (VkDescriptorPool)
		     cc_list_peekTail(usf->dp_list);

		// create a new pool on demand
		if((ds_count > usf->ds_available) || (dp == VK_NULL_HANDLE))
		{
			// create a new pool
			dp = vkk_engine_newDescriptorPoolLocked(self, usf);
			if(dp == VK_NULL_HANDLE)
			{
				vkk_engine_usfUnlock(self);
				goto fail_dp;
			}
		}

		if(vkk_engine_allocateDescriptorSetsLocked(self, dp,
		                                           dsl_array,
		                                           ds_count,
		                                           us->ds_array) == 0)
		{
			vkk_engine_usfUnlock(self);
			goto fail_allocate_ds;
		}

		usf->ds_available -= ds_count;
		vkk_engine_usfUnlock(self);
	}
	else
	{
		// reuse the uniform set
		us->ts  = 0.0;
		us->set = set;
		us->usf = usf;

		// copy the ua_array
		memcpy(us->ua_array, ua_array,
		       ua_count*sizeof(vkk_uniformAttachment_t));
	}

	// attach buffers and images
	for(i = 0; i < ua_count; ++i)
	{
		assert(ua_array[i].binding == usf->ub_array[i].binding);
		assert(ua_array[i].type == usf->ub_array[i].type);
		assert(ua_array[i].type < VKK_UNIFORM_TYPE_COUNT);

		if(ua_array[i].type == VKK_UNIFORM_TYPE_BUFFER)
		{
			vkk_engine_attachUniformBuffer(self, us,
			                               ua_array[i].buffer,
			                               ua_array[i].binding);
		}
		else if(ua_array[i].type == VKK_UNIFORM_TYPE_SAMPLER)
		{
			vkk_engine_attachUniformSampler(self, us,
			                                usf->ub_array[i].sampler,
			                                ua_array[i].image,
			                                ua_array[i].binding);
		}
	}

	// success
	return us;

	// failure
	fail_allocate_ds:
	fail_dp:
		FREE(us->ds_array);
	fail_ds_array:
		FREE(us->ua_array);
	fail_ua_array:
		FREE(us);
	return NULL;
}

void vkk_engine_deleteUniformSet(vkk_engine_t* self,
                                 vkk_uniformSet_t** _us)
{
	assert(self);
	assert(_us);

	vkk_uniformSet_t* us = *_us;
	if(us)
	{
		vkk_engine_rendererWaitForTimestamp(self, us->ts);

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
		*_us = NULL;
	}
}

vkk_pipelineLayout_t*
vkk_engine_newPipelineLayout(vkk_engine_t* self,
                             uint32_t usf_count,
                             vkk_uniformSetFactory_t** usf_array)
{
	assert(self);
	assert(usf_array);

	// allow for a constant and dynamic uniform set
	if(usf_count > 2)
	{
		LOGE("invalid usf_count=%i", usf_count);
		return NULL;
	}

	vkk_pipelineLayout_t* pl;
	pl = (vkk_pipelineLayout_t*)
	     CALLOC(1, sizeof(vkk_pipelineLayout_t));
	if(pl == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	pl->usf_count = usf_count;

	VkDescriptorSetLayout* dsl_array;
	dsl_array = (VkDescriptorSetLayout*)
	            CALLOC(usf_count, sizeof(VkDescriptorSetLayout));
	if(dsl_array == NULL)
	{
		goto fail_dsl_array;
	}

	int i;
	for(i = 0; i < usf_count; ++i)
	{
		dsl_array[i] = usf_array[i]->ds_layout;
	}

	VkPipelineLayoutCreateInfo pl_info =
	{
		.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext                  = NULL,
		.flags                  = 0,
		.setLayoutCount         = usf_count,
		.pSetLayouts            = dsl_array,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges    = NULL
	};

	if(vkCreatePipelineLayout(self->device, &pl_info, NULL,
	                          &pl->pl) != VK_SUCCESS)
	{
		LOGE("vkCreatePipelineLayout failed");
		goto fail_create_pl;
	}

	FREE(dsl_array);

	// success
	return pl;

	// failure
	fail_create_pl:
		FREE(dsl_array);
	fail_dsl_array:
		FREE(pl);
	return NULL;
}

void vkk_engine_deletePipelineLayout(vkk_engine_t* self,
                                     vkk_pipelineLayout_t** _pl)
{
	assert(self);
	assert(_pl);

	vkk_pipelineLayout_t* pl = *_pl;
	if(pl)
	{
		vkDestroyPipelineLayout(self->device,
		                        pl->pl, NULL);
		FREE(pl);
		*_pl = NULL;
	}
}

vkk_graphicsPipeline_t*
vkk_engine_newGraphicsPipeline(vkk_engine_t* self,
                               vkk_graphicsPipelineInfo_t* gpi)
{
	assert(self);
	assert(gpi);

	VkShaderModule vs;
	VkShaderModule fs;
	vs = vkk_engine_getShaderModule(self, gpi->vs);
	fs = vkk_engine_getShaderModule(self, gpi->fs);
	if((vs == VK_NULL_HANDLE) || (fs == VK_NULL_HANDLE))
	{
		return NULL;
	}

	vkk_graphicsPipeline_t* gp;
	gp = (vkk_graphicsPipeline_t*)
	     CALLOC(1, sizeof(vkk_graphicsPipeline_t));
	if(gp == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	VkPipelineShaderStageCreateInfo pss_info[2] =
	{
		// vertex stage
		{
			.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext               = NULL,
			.flags               = 0,
			.stage               = VK_SHADER_STAGE_VERTEX_BIT,
			.module              = vs,
			.pName               = "main",
			.pSpecializationInfo = NULL
		},

		// fragment stage
		{
			.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext               = NULL,
			.flags               = 0,
			.stage               = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module              = fs,
			.pName               = "main",
			.pSpecializationInfo = NULL
		}
	};

	VkVertexInputBindingDescription* vib;
	vib = (VkVertexInputBindingDescription*)
	      CALLOC(gpi->vb_count, sizeof(VkVertexInputBindingDescription));
	if(vib == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_vib;
	}

	VkVertexInputAttributeDescription* via;
	via = (VkVertexInputAttributeDescription*)
	      CALLOC(gpi->vb_count, sizeof(VkVertexInputAttributeDescription));
	if(via == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_via;
	}

	uint32_t stride[VKK_VERTEX_FORMAT_COUNT] =
	{
		sizeof(float),
		sizeof(int),
		sizeof(short)
	};

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
	};

	int i;
	for(i = 0; i < gpi->vb_count; ++i)
	{
		vkk_vertexBufferInfo_t* vbi= &(gpi->vbi[i]);
		vib[i].binding   = vbi->location;
		vib[i].stride    = stride[vbi->format]*vbi->components;
		vib[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		via[i].location  = vbi->location;
		via[i].binding   = vbi->location;
		via[i].format    = format[4*vbi->format +
		                          vbi->components - 1];
		via[i].offset    = 0;
	}

	VkPipelineVertexInputStateCreateInfo pvis_info =
	{
		.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext                           = NULL,
		.flags                           = 0,
		.vertexBindingDescriptionCount   = gpi->vb_count,
		.pVertexBindingDescriptions      = vib,
		.vertexAttributeDescriptionCount = gpi->vb_count,
		.pVertexAttributeDescriptions    = via
	};

	VkPrimitiveTopology topology[VKK_PRIMITIVE_TRIANGLE_COUNT] =
	{
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN
	};
	VkPipelineInputAssemblyStateCreateInfo pias_info =
	{
		.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext                  = NULL,
		.flags                  = 0,
		.topology               = topology[gpi->primitive],
		.primitiveRestartEnable = gpi->primitive_restart
	};

	VkViewport viewport =
	{
		.x        = 0.0f,
		.y        = 0.0f,
		.width    = (float) 0.0,
		.height   = (float) 0.0,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	VkRect2D scissor =
	{
		.offset =
		{
			.x = 0,
			.y = 0
		},
		.extent =
		{
			.width  = (uint32_t) 0,
			.height = (uint32_t) 0,
		}
	};

	VkPipelineViewportStateCreateInfo pvs_info =
	{
		.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.pNext         = NULL,
		.flags         = 0,
		.viewportCount = 1,
		.pViewports    = &viewport,
		.scissorCount  = 1,
		.pScissors     = &scissor
	};

	VkCullModeFlags cullMode;
	cullMode = gpi->cull_back ? VK_CULL_MODE_BACK_BIT :
	                            VK_CULL_MODE_NONE;
	VkPipelineRasterizationStateCreateInfo prs_info =
	{
		.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.pNext                   = NULL,
		.flags                   = 0,
		.depthClampEnable        = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode             = VK_POLYGON_MODE_FILL,
		.cullMode                = cullMode,
		.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable         = VK_FALSE,
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp          = 0.0f,
		.depthBiasSlopeFactor    = 0.0f,
		.lineWidth               = 1.0f
	};

	VkPipelineMultisampleStateCreateInfo pms_info =
	{
		.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.pNext                 = NULL,
		.flags                 = 0,
		.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable   = VK_FALSE,
		.minSampleShading      = 0.0f,
		.pSampleMask           = NULL,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable      = VK_FALSE
	};

	VkPipelineDepthStencilStateCreateInfo pdss_info =
	{
		.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.pNext                 = NULL,
		.flags                 = 0,
		.depthTestEnable       = gpi->depth_test,
		.depthWriteEnable      = gpi->depth_write,
		.depthCompareOp        = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable     = VK_FALSE,
		.front =
		{
			.failOp      = VK_STENCIL_OP_KEEP,
			.passOp      = VK_STENCIL_OP_KEEP,
			.depthFailOp = VK_STENCIL_OP_KEEP,
			.compareOp   = VK_COMPARE_OP_NEVER,
			.compareMask = 0,
			.writeMask   = 0,
			.reference   = 0
		},
		.back =
		{
			.failOp      = VK_STENCIL_OP_KEEP,
			.passOp      = VK_STENCIL_OP_KEEP,
			.depthFailOp = VK_STENCIL_OP_KEEP,
			.compareOp   = VK_COMPARE_OP_NEVER,
			.compareMask = 0,
			.writeMask   = 0,
			.reference   = 0
		},
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f
	};

	VkPipelineColorBlendAttachmentState pcbs =
	{
		.blendEnable         = VK_FALSE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp        = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.alphaBlendOp        = VK_BLEND_OP_ADD,
		.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT |
		                       VK_COLOR_COMPONENT_G_BIT |
		                       VK_COLOR_COMPONENT_B_BIT |
		                       VK_COLOR_COMPONENT_A_BIT,
	};

	if(gpi->blend_mode == VKK_BLEND_MODE_TRANSPARENCY)
	{
		pcbs.blendEnable = VK_TRUE;
	}

	VkPipelineColorBlendStateCreateInfo pcbs_info =
	{
		.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.pNext           = NULL,
		.flags           = 0,
		.logicOpEnable   = VK_FALSE,
		.logicOp         = VK_LOGIC_OP_CLEAR,
		.attachmentCount = 1,
		.pAttachments    = &pcbs,
		.blendConstants  = { 0.0f, 0.0f, 0.0f, 0.0f }
	};

	VkDynamicState dynamic_state[2] =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo pds_info =
	{
		.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.pNext             = NULL,
		.flags             = 0,
		.dynamicStateCount = 2,
		.pDynamicStates    = dynamic_state,
	};

	VkGraphicsPipelineCreateInfo gp_info =
	{
		.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext               = NULL,
		.flags               = 0,
		.stageCount          = 2,
		.pStages             = pss_info,
		.pVertexInputState   = &pvis_info,
		.pInputAssemblyState = &pias_info,
		.pTessellationState  = NULL,
		.pViewportState      = &pvs_info,
		.pRasterizationState = &prs_info,
		.pMultisampleState   = &pms_info,
		.pDepthStencilState  = &pdss_info,
		.pColorBlendState    = &pcbs_info,
		.pDynamicState       = &pds_info,
		.layout              = gpi->pl->pl,
		.renderPass          = vkk_renderer_renderPass(gpi->renderer),
		.subpass             = 0,
		.basePipelineHandle  = VK_NULL_HANDLE,
		.basePipelineIndex   = -1
	};

	if(vkCreateGraphicsPipelines(self->device,
	                             self->pipeline_cache,
	                             1, &gp_info, NULL,
	                             &gp->pipeline) != VK_SUCCESS)
	{
		LOGE("vkCreateGraphicsPipelines failed");
		goto fail_create;
	}

	// success
	return gp;

	// failure
	fail_create:
		FREE(via);
	fail_via:
		FREE(vib);
	fail_vib:
		FREE(gp);
	return NULL;
}

void vkk_engine_deleteGraphicsPipeline(vkk_engine_t* self,
                                       vkk_graphicsPipeline_t** _gp)
{
	assert(self);
	assert(_gp);

	vkk_graphicsPipeline_t* gp = *_gp;
	if(gp)
	{
		vkk_engine_rendererWaitForTimestamp(self, gp->ts);

		vkDestroyPipeline(self->device,
		                  gp->pipeline, NULL);
		FREE(gp);
		*_gp = NULL;
	}
}

/***********************************************************
* default renderer API                                     *
***********************************************************/

int vkk_engine_resize(vkk_engine_t* self)
{
	assert(self);

	return vkk_defaultRenderer_resize(self->renderer);
}

vkk_renderer_t* vkk_engine_renderer(vkk_engine_t* self)
{
	assert(self);

	return self->renderer;
}

/***********************************************************
* public                                                   *
***********************************************************/

void vkk_engine_mipmapImage(vkk_engine_t* self,
                            vkk_image_t* image,
                            VkCommandBuffer cb)
{
	assert(self);
	assert(image);
	assert(cb != VK_NULL_HANDLE);

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
					.z = 1,
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
					.z = 1,
				}
			}
		};

		vkCmdBlitImage(cb,
		               image->image,
		               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		               image->image,
		               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		               1, &ib, VK_FILTER_LINEAR);

		// transition the mip level i to a src for blitting
		vkk_util_imageMemoryBarrier(image, cb,
		                            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		                            i, 1);
	}
}

int vkk_engine_queueSubmit(vkk_engine_t* self,
                           VkCommandBuffer* cb,
                           VkSemaphore* semaphore_acquire,
                           VkSemaphore* semaphore_submit,
                           VkPipelineStageFlags* wait_dst_stage_mask,
                           VkFence fence)
{
	// semaphore_acquire, semaphore_submit and
	// wait_dst_stage_mask may be NULL
	assert(self);
	assert(cb);

	VkSubmitInfo s_info =
	{
		.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext                = NULL,
		.waitSemaphoreCount   = semaphore_acquire ? 1 : 0,
		.pWaitSemaphores      = semaphore_acquire,
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

	if(vkQueueSubmit(self->queue, 1, &s_info,
	                 fence) != VK_SUCCESS)
	{
		LOGE("vkQueueSubmit failed");
		vkk_engine_rendererUnlock(self);
		return 0;
	}
	vkk_engine_rendererUnlock(self);

	return 1;
}

void vkk_engine_queueWaitIdle(vkk_engine_t* self)
{
	assert(self);

	vkk_engine_rendererLock(self);
	if(self->shutdown == 0)
	{
		vkQueueWaitIdle(self->queue);
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
	assert(self);
	assert(dp != VK_NULL_HANDLE);
	assert(dsl_array);
	assert(ds_count > 0);
	assert(ds_array);

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

int vkk_engine_allocateCommandBuffers(vkk_engine_t* self,
                                      int cb_count,
                                      VkCommandBuffer* cb_array)
{
	assert(self);
	assert(cb_count > 0);
	assert(cb_array);

	VkCommandBufferAllocateInfo cba_info =
	{
		.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext              = NULL,
		.commandPool        = self->command_pool,
		.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = cb_count
	};

	vkk_engine_cmdLock(self);
	if(vkAllocateCommandBuffers(self->device, &cba_info,
	                            cb_array) != VK_SUCCESS)
	{
		LOGE("vkAllocateCommandBuffers failed");
		vkk_engine_cmdUnlock(self);
		return 0;
	}
	vkk_engine_cmdUnlock(self);

	return 1;
}

void vkk_engine_freeCommandBuffers(vkk_engine_t* self,
                                   uint32_t cb_count,
                                   const VkCommandBuffer* cb_array)
{
	assert(self);
	assert(cb_array);

	vkk_engine_cmdLock(self);
	vkFreeCommandBuffers(self->device,
	                     self->command_pool,
	                     cb_count,
	                     cb_array);
	vkk_engine_cmdUnlock(self);
}

void vkk_engine_cmdLock(vkk_engine_t* self)
{
	assert(self);

	pthread_mutex_lock(&self->cmd_mutex);
	TRACE_BEGIN();
}

void vkk_engine_cmdUnlock(vkk_engine_t* self)
{
	assert(self);

	TRACE_END();
	pthread_mutex_unlock(&self->cmd_mutex);
}

void vkk_engine_usfLock(vkk_engine_t* self)
{
	assert(self);

	pthread_mutex_lock(&self->usf_mutex);
	TRACE_BEGIN();
}

void vkk_engine_usfUnlock(vkk_engine_t* self)
{
	assert(self);

	TRACE_END();
	pthread_mutex_unlock(&self->usf_mutex);
}

void vkk_engine_smLock(vkk_engine_t* self)
{
	assert(self);

	pthread_mutex_lock(&self->sm_mutex);
	TRACE_BEGIN();
}

void vkk_engine_smUnlock(vkk_engine_t* self)
{
	assert(self);

	TRACE_END();
	pthread_mutex_unlock(&self->sm_mutex);
}

void vkk_engine_rendererLock(vkk_engine_t* self)
{
	assert(self);

	pthread_mutex_lock(&self->renderer_mutex);
	TRACE_BEGIN();
}

void vkk_engine_rendererUnlock(vkk_engine_t* self)
{
	assert(self);

	TRACE_END();
	pthread_mutex_unlock(&self->renderer_mutex);
}

void vkk_engine_rendererSignal(vkk_engine_t* self)
{
	assert(self);

	pthread_cond_broadcast(&self->renderer_cond);
}

void vkk_engine_rendererWait(vkk_engine_t* self)
{
	assert(self);

	TRACE_END();
	pthread_cond_wait(&self->renderer_cond,
	                  &self->renderer_mutex);
	TRACE_BEGIN();
}

void vkk_engine_rendererWaitForTimestamp(vkk_engine_t* self,
                                         double ts)
{
	assert(self);

	vkk_renderer_t* renderer = self->renderer;

	// ignore zero
	if(ts == 0.0)
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
