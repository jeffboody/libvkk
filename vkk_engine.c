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

#define LOG_TAG "vkk"
#include "../libcc/cc_log.h"
#include "../libcc/cc_memory.h"
#include "vkk_engine.h"

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

	int flags = (fullscreen ? SDL_WINDOW_FULLSCREEN : 0) |
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

static int vkk_engine_newCacheAndPools(vkk_engine_t* self)
{
	assert(self);

	VkPipelineCacheCreateInfo pc_info =
	{
		.sType           = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
		.pNext           = NULL,
		.flags           = 0,
		.initialDataSize = 0,
		.pInitialData    = NULL
	};

	if(vkCreatePipelineCache(self->device, &pc_info, NULL,
	                         &self->pipeline_cache) != VK_SUCCESS)
	{
		LOGE("vkCreatePipelineCache failed");
		return 0;
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
	return 0;
}

static int vkk_engine_newSwapchain(vkk_engine_t* self)
{
	assert(self);

	VkSurfaceCapabilitiesKHR caps;
	if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(self->physical_device,
	                                             self->surface,
	                                             &caps) != VK_SUCCESS)
	{
		LOGE("vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed");
		return 0;
	}

	// check the minImageCount
	uint32_t minImageCount = 3;
	if((caps.maxImageCount > 0) &&
	   (minImageCount > caps.maxImageCount))
	{
		minImageCount = caps.maxImageCount;
	}
	else if(minImageCount < caps.minImageCount)
	{
		minImageCount = caps.minImageCount;
	}

	uint32_t sf_count;
	if(vkGetPhysicalDeviceSurfaceFormatsKHR(self->physical_device,
	                                        self->surface,
	                                        &sf_count,
	                                        NULL) != VK_SUCCESS)
	{
		LOGE("vkGetPhysicalDeviceSurfaceFormatsKHR failed");
		return 0;
	}

	VkSurfaceFormatKHR* sf;
	sf = (VkSurfaceFormatKHR*)
	     CALLOC(sf_count, sizeof(VkSurfaceFormatKHR));
	if(sf == NULL)
	{
		LOGE("CALLOC failed");
		return 0;
	}

	if(vkGetPhysicalDeviceSurfaceFormatsKHR(self->physical_device,
	                                        self->surface,
	                                        &sf_count,
	                                        sf) != VK_SUCCESS)
	{
		LOGE("vkGetPhysicalDeviceSurfaceFormatsKHR failed");
		goto fail_surface_formats3;
	}

	self->swapchain_frame       = 0;
	self->swapchain_format      = sf[0].format;
	self->swapchain_color_space = sf[0].colorSpace;
	int i;
	for(i = 0; i < sf_count; ++i)
	{
		if((sf[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) &&
		   ((sf[i].format == VK_FORMAT_R8G8B8A8_UNORM) ||
		    (sf[i].format == VK_FORMAT_B8G8R8A8_UNORM)))
		{
			self->swapchain_format      = sf[i].format;
			self->swapchain_color_space = sf[i].colorSpace;
			break;
		}
	}

	uint32_t pm_count;
	if(vkGetPhysicalDeviceSurfacePresentModesKHR(self->physical_device,
	                                             self->surface,
	                                             &pm_count,
	                                             NULL) != VK_SUCCESS)
	{
		goto fail_present_modes1;
	}

	VkPresentModeKHR* pm;
	pm = (VkPresentModeKHR*)
	     CALLOC(pm_count, sizeof(VkPresentModeKHR));
	if(pm == NULL)
	{
		goto fail_present_modes2;
	}

	if(vkGetPhysicalDeviceSurfacePresentModesKHR(self->physical_device,
	                                             self->surface,
	                                             &pm_count,
	                                             pm) != VK_SUCCESS)
	{
		goto fail_present_modes3;
	}

	VkPresentModeKHR present_mode = pm[0];
	for(i = 0; i < pm_count; ++i)
	{
		if(pm[i] == VK_PRESENT_MODE_FIFO_KHR)
		{
			present_mode = pm[i];
			break;
		}
	}

	VkSurfaceTransformFlagBitsKHR preTransform;
	if(caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
	{
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else
	{
		preTransform = caps.currentTransform;
	}

	VkSwapchainCreateInfoKHR sc_info =
	{
		.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext                 = NULL,
		.flags                 = 0,
		.surface               = self->surface,
		.minImageCount         = minImageCount,
		.imageFormat           = self->swapchain_format,
		.imageColorSpace       = self->swapchain_color_space,
		.imageExtent           = caps.currentExtent,
		.imageArrayLayers      = 1,
		.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices   = &self->queue_family_index,
		.preTransform          = preTransform,
		.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode           = present_mode,
		.clipped               = VK_TRUE,
		.oldSwapchain          = VK_NULL_HANDLE
	};

	self->swapchain_extent = caps.currentExtent;

	if(vkCreateSwapchainKHR(self->device, &sc_info, NULL,
	                        &self->swapchain) != VK_SUCCESS)
	{
		LOGE("vkCreateSwapchainKHR failed");
		goto fail_swapchain;
	}

	uint32_t count = 0;
	if(vkGetSwapchainImagesKHR(self->device,
	                           self->swapchain,
	                           &count,
	                           NULL) != VK_SUCCESS)
	{
		LOGE("vkGetSwapchainImagesKHR failed");
		goto fail_get1;
	}

	// validate swapchain_image_count across resizes
	if(self->swapchain_image_count &&
	   (self->swapchain_image_count != count))
	{
		LOGE("invalid %u, %u",
		     self->swapchain_image_count, count);
		goto fail_count;
	}
	self->swapchain_image_count = count;

	self->swapchain_images = (VkImage*)
	                         CALLOC(self->swapchain_image_count,
	                                sizeof(VkImage));
	if(self->swapchain_images == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_images;
	}

	self->swapchain_fences = (VkFence*)
	                         CALLOC(self->swapchain_image_count,
	                                sizeof(VkFence));
	if(self->swapchain_fences == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_fences;
	}

	uint32_t image_count = self->swapchain_image_count;
	if(vkGetSwapchainImagesKHR(self->device,
	                           self->swapchain,
	                           &image_count,
	                           self->swapchain_images) != VK_SUCCESS)
	{
		LOGE("vkGetSwapchainImagesKHR failed");
		goto fail_get2;
	}

	for(i = 0; i < image_count; ++i)
	{
		VkFenceCreateInfo f_info =
		{
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = NULL,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT
		};

		if(vkCreateFence(self->device, &f_info, NULL,
		                 &self->swapchain_fences[i]) != VK_SUCCESS)
		{
			goto fail_create_fence;
		}
	}

	FREE(pm);
	FREE(sf);

	// success
	return 1;

	// failure
	fail_create_fence:
	{
		int j;
		for(j = 0; j < i; ++j)
		{
			vkDestroyFence(self->device,
			               self->swapchain_fences[j], NULL);
		}
	}
	fail_get2:
		FREE(self->swapchain_fences);
	fail_fences:
		FREE(self->swapchain_images);
	fail_images:
	fail_count:
	fail_get1:
		vkDestroySwapchainKHR(self->device,
		                      self->swapchain, NULL);
	fail_swapchain:
	fail_present_modes3:
		FREE(pm);
	fail_present_modes2:
	fail_present_modes1:
	fail_surface_formats3:
		FREE(sf);
	return 0;
}

static void vkk_engine_deleteSwapchain(vkk_engine_t* self)
{
	assert(self);

	if(self->swapchain == VK_NULL_HANDLE)
	{
		return;
	}

	int i;
	for(i = 0; i < self->swapchain_image_count; ++i)
	{
		vkDestroyFence(self->device,
		               self->swapchain_fences[i], NULL);
	}
	FREE(self->swapchain_fences);
	FREE(self->swapchain_images);
	vkDestroySwapchainKHR(self->device,
	                      self->swapchain, NULL);
	self->swapchain_fences = NULL;
	self->swapchain_images = NULL;
	self->swapchain        = VK_NULL_HANDLE;
}

static int vkk_engine_newDepth(vkk_engine_t* self)
{
	assert(self);

	self->depth_image = vkk_engine_newImage(self,
	                                        self->swapchain_extent.width,
	                                        self->swapchain_extent.height,
	                                        VKK_IMAGE_FORMAT_DEPTH,
	                                        0, NULL);
	if(self->depth_image == NULL)
	{
		return 0;
	}

	return 1;
}

static void
vkk_engine_deleteDepth(vkk_engine_t* self)
{
	assert(self);

	vkk_engine_deleteImage(self, &self->depth_image);
}

static int vkk_engine_newFramebuffer(vkk_engine_t* self)
{
	assert(self);

	self->framebuffer_image_views = (VkImageView*)
	                                CALLOC(self->swapchain_image_count,
	                                       sizeof(VkImageView));
	if(self->framebuffer_image_views == NULL)
	{
		LOGE("CALLOC failed");
		return 0;
	}

	self->framebuffers = (VkFramebuffer*)
	                     CALLOC(self->swapchain_image_count,
	                            sizeof(VkFramebuffer));
	if(self->framebuffers == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_alloc_framebuffers;
	}

	int i;
	for(i = 0; i < self->swapchain_image_count; ++i)
	{
		VkImageViewCreateInfo iv_info =
		{
			.sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext      = NULL,
			.flags      = 0,
			.image      = self->swapchain_images[i],
			.viewType   = VK_IMAGE_VIEW_TYPE_2D,
			.format     = self->swapchain_format,
			.components =
			{
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY
			},
			.subresourceRange =
			{
				.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel   = 0,
				.levelCount     = 1,
				.baseArrayLayer = 0,
				.layerCount     = 1
			}
		};

		if(vkCreateImageView(self->device, &iv_info, NULL,
		                     &self->framebuffer_image_views[i]) != VK_SUCCESS)
		{
			LOGE("vkCreateImageView failed");
			goto fail_image_view;
		}

		VkImageView attachments[2] =
		{
			self->framebuffer_image_views[i],
			self->depth_image->image_view,
		};

		VkFramebufferCreateInfo f_info =
		{
			.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext           = NULL,
			.flags           = 0,
			.renderPass      = self->render_pass,
			.attachmentCount = 2,
			.pAttachments    = attachments,
			.width           = self->swapchain_extent.width,
			.height          = self->swapchain_extent.height,
			.layers          = 1,
		};

		if(vkCreateFramebuffer(self->device, &f_info, NULL,
		                       &self->framebuffers[i]) != VK_SUCCESS)
		{
			vkDestroyImageView(self->device,
			                   self->framebuffer_image_views[i],
			                   NULL);
			self->framebuffer_image_views[i] = VK_NULL_HANDLE;

			LOGE("vkCreateFramebuffer failed");
			goto fail_framebuffer;
		}
	}

	// success
	return 1;

	// failure
	fail_framebuffer:
	fail_image_view:
	{
		int j;
		for(j = 0; j < i; ++j)
		{
			vkDestroyFramebuffer(self->device,
			                     self->framebuffers[j],
			                     NULL);
			vkDestroyImageView(self->device,
			                   self->framebuffer_image_views[j],
			                   NULL);
			self->framebuffers[j]            = VK_NULL_HANDLE;
			self->framebuffer_image_views[j] = VK_NULL_HANDLE;
		}
		FREE(self->framebuffers);
		self->framebuffers = NULL;
	}
	fail_alloc_framebuffers:
		FREE(self->framebuffer_image_views);
		self->framebuffer_image_views = NULL;
	return 0;
}

static void
vkk_engine_deleteFramebuffer(vkk_engine_t* self)
{
	assert(self);

	if(self->framebuffers == NULL)
	{
		return;
	}

	int i;
	for(i = 0; i < self->swapchain_image_count; ++i)
	{
		vkDestroyFramebuffer(self->device,
		                     self->framebuffers[i],
		                     NULL);
		vkDestroyImageView(self->device,
		                   self->framebuffer_image_views[i],
		                   NULL);
		self->framebuffers[i]            = VK_NULL_HANDLE;
		self->framebuffer_image_views[i] = VK_NULL_HANDLE;
	}
	FREE(self->framebuffers);
	FREE(self->framebuffer_image_views);
	self->framebuffers            = NULL;
	self->framebuffer_image_views = NULL;
}

static int vkk_engine_newRenderpass(vkk_engine_t* self)
{
	assert(self);

	VkAttachmentDescription attachments[2] =
	{
		{
			.flags          = 0,
			.format         = self->swapchain_format,
			.samples        = VK_SAMPLE_COUNT_1_BIT,
			.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		},
		{
			.flags          = 0,
			.format         = VK_FORMAT_D24_UNORM_S8_UINT,
			.samples        = VK_SAMPLE_COUNT_1_BIT,
			.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		}
	};

	VkAttachmentReference color_attachment =
	{
		.attachment = 0,
		.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkAttachmentReference depth_attachment =
	{
		.attachment = 1,
		.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass =
	{
		.flags = 0,
		.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount    = 0,
		.pInputAttachments       = NULL,
		.colorAttachmentCount    = 1,
		.pColorAttachments       = &color_attachment,
		.pResolveAttachments     = NULL,
		.pDepthStencilAttachment = &depth_attachment,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments    = NULL,
	};

	VkRenderPassCreateInfo rp_info =
	{
		.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext           = NULL,
		.flags           = 0,
		.attachmentCount = 2,
		.pAttachments    = attachments,
		.subpassCount    = 1,
		.pSubpasses      = &subpass,
		.dependencyCount = 0,
		.pDependencies   = NULL
	};

	if(vkCreateRenderPass(self->device, &rp_info, NULL,
	                      &self->render_pass) != VK_SUCCESS)
	{
		LOGE("vkCreateRenderPass failed");
		return 0;
	}

	return 1;
}

static int vkk_engine_newCommandBuffers(vkk_engine_t* self)
{
	assert(self);

	self->command_buffers = (VkCommandBuffer*)
	                        CALLOC(self->swapchain_image_count,
	                               sizeof(VkCommandBuffer));
	if(self->command_buffers == NULL)
	{
		LOGE("CALLOC failed");
		return 0;
	}

	VkCommandBufferAllocateInfo cba_info =
	{
		.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext              = NULL,
		.commandPool        = self->command_pool,
		.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = self->swapchain_image_count
	};

	if(vkAllocateCommandBuffers(self->device, &cba_info,
	                            self->command_buffers) != VK_SUCCESS)
	{
		LOGE("vkAllocateCommandBuffers failed");
		goto fail_allocate;
	}

	// success
	return 1;

	// failure
	fail_allocate:
		FREE(self->command_buffers);
	return 0;
}

static int vkk_engine_newSemaphores(vkk_engine_t* self)
{
	assert(self);

	self->semaphore_index   = 0;
	self->semaphore_acquire = (VkSemaphore*)
	                          CALLOC(self->swapchain_image_count,
	                                 sizeof(VkSemaphore));
	if(self->semaphore_acquire == NULL)
	{
		LOGE("CALLOC failed");
		return 0;
	}

	self->semaphore_submit = (VkSemaphore*)
	                         CALLOC(self->swapchain_image_count,
	                                sizeof(VkSemaphore));
	if(self->semaphore_submit == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_alloc_ss;
	}

	int i;
	for(i = 0; i < self->swapchain_image_count; ++i)
	{
		VkSemaphoreCreateInfo sa_info =
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0
		};

		if(vkCreateSemaphore(self->device, &sa_info, NULL,
		                     &self->semaphore_acquire[i]) != VK_SUCCESS)
		{
			LOGE("vkCreateSemaphore failed");
			goto fail_create;
		}

		VkSemaphoreCreateInfo ss_info =
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = NULL,
			.flags = 0
		};

		if(vkCreateSemaphore(self->device, &ss_info, NULL,
		                     &self->semaphore_submit[i]) != VK_SUCCESS)
		{
			vkDestroySemaphore(self->device,
			                   self->semaphore_acquire[i],
			                   NULL);

			LOGE("vkCreateSemaphore failed");
			goto fail_create;
		}
	}

	// success
	return 1;

	// failure
	fail_create:
	{
		int j;
		for(j = 0; j < i; ++j)
		{
			vkDestroySemaphore(self->device,
			                   self->semaphore_submit[j],
			                   NULL);
			vkDestroySemaphore(self->device,
			                   self->semaphore_acquire[j],
			                   NULL);
		}
		FREE(self->semaphore_submit);
	}
	fail_alloc_ss:
		FREE(self->semaphore_acquire);
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

	#ifdef ANDROID
		AAssetManager* am;
		am = self->app->activity->assetManager;
		AAsset* f = AAssetManager_open(am, fname,
		                               AASSET_MODE_BUFFER);
		if(f == NULL)
		{
			LOGE("AAssetManager_open %s failed", fname);
			return NULL;
		}

		size_t size = (size_t) AAsset_getLength(f);
		if((size == 0) || ((size % 4) != 0))
		{
			LOGE("invalid size=%u", (unsigned int) size);
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

		if(AAsset_read(f, code, size) != size)
		{
			LOGE("AAsset_read failed");
			goto fail_code;
		}

		AAsset_close(f);
	#else
		FILE* f = fopen(fname, "r");
		if(f == NULL)
		{
			LOGE("fopen %s failed", fname);
			return NULL;
		}

		fseek(f, 0, SEEK_END);
		size_t size = (size_t) ftell(f);

		if((size == 0) || ((size % 4) != 0))
		{
			LOGE("invalid size=%u", (unsigned int) size);
			goto fail_size;
		}

		if(fseek(f, 0, SEEK_SET) == -1)
		{
			LOGE("fseek failed");
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

		if(fread((void*) code, size, 1, f) != 1)
		{
			LOGE("fread failed");
			goto fail_code;
		}

		fclose(f);
	#endif

	*_size = size;

	// success
	return code;

	// failure
	fail_code:
		FREE(code);
	fail_alloc:
	fail_size:
	#ifdef ANDROID
		AAsset_close(f);
	#else
		fclose(f);
	#endif
	return NULL;
}

static VkShaderModule
vkk_engine_getShaderModule(vkk_engine_t* self,
                           const char* fname)
{
	assert(self);
	assert(fname);

	cc_mapIter_t miter;
	VkShaderModule sm;
	sm = (VkShaderModule)
	     cc_map_find(self->shader_modules, &miter, fname);
	if(sm != VK_NULL_HANDLE)
	{
		return sm;
	}

	size_t    size = 0;
	uint32_t* code;
	code = vkk_engine_importShaderModule(self, fname, &size);
	if(code == NULL)
	{
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
		LOGE("vkCreateShaderModule failed");
		goto fail_create;
	}

	if(cc_map_add(self->shader_modules, (const void*) sm,
	              fname) == 0)
	{
		goto fail_add;
	}

	FREE(code);

	// success
	return sm;

	// failure
	fail_add:
		vkDestroyShaderModule(self->device, sm, NULL);
	fail_create:
		FREE(code);
	return VK_NULL_HANDLE;
}

static void
vkk_engine_beginSemaphore(vkk_engine_t* self,
                          VkSemaphore* semaphore_acquire,
                          VkSemaphore* semaphore_submit)
{
	assert(self);
	assert(semaphore_acquire);
	assert(semaphore_submit);

	uint32_t idx       = self->semaphore_index;
	*semaphore_acquire = self->semaphore_acquire[idx];
	*semaphore_submit  = self->semaphore_submit[idx];
}

static void
vkk_engine_endSemaphore(vkk_engine_t* self,
                        VkSemaphore* semaphore_acquire,
                        VkSemaphore* semaphore_submit)
{
	assert(self);
	assert(semaphore_acquire);
	assert(semaphore_submit);

	uint32_t idx       = self->semaphore_index;
	*semaphore_acquire = self->semaphore_acquire[idx];
	*semaphore_submit  = self->semaphore_submit[idx];

	++idx;
	self->semaphore_index = idx%self->swapchain_image_count;
}

static VkDescriptorPool
vkk_engine_newDescriptorPool(vkk_engine_t* self,
                             vkk_uniformSetFactory_t* usf)
{
	assert(self);
	assert(usf);

	VkDescriptorType dt_map[VKK_UNIFORM_TYPE_COUNT] =
	{
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_DESCRIPTOR_TYPE_SAMPLER,
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
		return 0;
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
	VkCommandBufferAllocateInfo cba_info =
	{
		.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext              = NULL,
		.commandPool        = self->command_pool,
		.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};

	VkCommandBuffer cb;
	if(vkAllocateCommandBuffers(self->device, &cba_info,
	                            &cb) != VK_SUCCESS)
	{
		LOGE("vkAllocateCommandBuffers failed");
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

	if(vkBeginCommandBuffer(cb, &cb_info) != VK_SUCCESS)
	{
		LOGE("vkBeginCommandBuffer failed");
		goto fail_begin_cb;
	}

	// transition the image to copy the transfer buffer to
	// the image
	VkImageMemoryBarrier imb1 =
	{
		.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext               = NULL,
		.srcAccessMask       = 0,
		.dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
		.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image               = image->image,
		.subresourceRange    =
		{
			.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel   = 0,
			.levelCount     = 1,
			.baseArrayLayer = 0,
			.layerCount     = 1
		}
	};

	vkCmdPipelineBarrier(cb,
	                     VK_PIPELINE_STAGE_HOST_BIT,
	                     VK_PIPELINE_STAGE_TRANSFER_BIT,
	                     0, 0, NULL, 0 ,NULL, 1, &imb1);

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

	// transition the image from transfer mode to shading mode
	VkImageMemoryBarrier imb2 =
	{
		.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext               = NULL,
		.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
		.dstAccessMask       = VK_ACCESS_SHADER_READ_BIT,
		.oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image               = image->image,
		.subresourceRange    =
		{
			.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel   = 0,
			.levelCount     = 1,
			.baseArrayLayer = 0,
			.layerCount     = 1
		}
	};

	// NOTE: this only supports textures in fragment shaders
	vkCmdPipelineBarrier(cb,
	                     VK_PIPELINE_STAGE_TRANSFER_BIT,
	                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
	                     0, 0, NULL, 0 ,NULL, 1, &imb2);

	// end the transfer commands
	vkEndCommandBuffer(cb);

	// submit the commands
	VkSubmitInfo s_info =
	{
		.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext                = NULL,
		.waitSemaphoreCount   = 0,
		.pWaitSemaphores      = NULL,
		.pWaitDstStageMask    = NULL,
		.commandBufferCount   = 1,
		.pCommandBuffers      = &cb,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores    = NULL
	};

	if(vkQueueSubmit(self->queue, 1, &s_info,
	                 VK_NULL_HANDLE) != VK_SUCCESS)
	{
		LOGE("vkQueueSubmit failed");
		goto fail_submit;
	}
	vkQueueWaitIdle(self->queue);

	// release temporary objects
	vkFreeCommandBuffers(self->device,
	                     self->command_pool,
	                     1, &cb);
	vkFreeMemory(self->device, memory, NULL);
	vkDestroyBuffer(self->device, buffer, NULL);

	// success
	return 1;

	// failure
	fail_submit:
		vkEndCommandBuffer(cb);
	fail_begin_cb:
		vkFreeCommandBuffers(self->device,
		                     self->command_pool,
		                     1, &cb);
	fail_allocate_cb:
	fail_bind:
	fail_map:
		vkFreeMemory(self->device, memory, NULL);
	fail_allocate_memory:
	fail_memory_type:
		vkDestroyBuffer(self->device, buffer, NULL);
	return 0;
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_engine_t* vkk_engine_new(void* app,
                             const char* app_name,
                             uint32_t app_version)
{
	#ifdef ANDROID
		assert(app);
	#else
		assert(app == NULL);
	#endif
	assert(app_name);

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

	if(vkk_engine_newSwapchain(self) == 0)
	{
		goto fail_swapchain;
	}

	if(vkk_engine_newRenderpass(self) == 0)
	{
		goto fail_renderpass;
	}

	if(vkk_engine_newDepth(self) == 0)
	{
		goto fail_depth;
	}

	if(vkk_engine_newFramebuffer(self) == 0)
	{
		goto fail_framebuffer;
	}

	if(vkk_engine_newCommandBuffers(self) == 0)
	{
		goto fail_command_buffers;
	}

	if(vkk_engine_newSemaphores(self) == 0)
	{
		goto fail_semaphores;
	}

	self->shader_modules = cc_map_new();
	if(self->shader_modules == NULL)
	{
		goto fail_shader_modules;
	}

	// success
	return self;

	// failure
	fail_shader_modules:
	{
		int i;
		for(i = 0; i < self->swapchain_image_count; ++i)
		{
			vkDestroySemaphore(self->device,
			                   self->semaphore_submit[i],
			                   NULL);
			vkDestroySemaphore(self->device,
			                   self->semaphore_acquire[i],
			                   NULL);
		}
		FREE(self->semaphore_submit);
		FREE(self->semaphore_acquire);
	}
	fail_semaphores:
		vkFreeCommandBuffers(self->device,
		                     self->command_pool,
		                     self->swapchain_image_count,
		                     self->command_buffers);
		FREE(self->command_buffers);
	fail_command_buffers:
		vkk_engine_deleteFramebuffer(self);
	fail_framebuffer:
		vkk_engine_deleteDepth(self);
	fail_depth:
		vkDestroyRenderPass(self->device,
		                    self->render_pass, NULL);
	fail_renderpass:
		vkk_engine_deleteSwapchain(self);
	fail_swapchain:
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

		int i;
		for(i = 0; i < self->swapchain_image_count; ++i)
		{
			vkDestroySemaphore(self->device,
			                   self->semaphore_submit[i],
			                   NULL);
			vkDestroySemaphore(self->device,
			                   self->semaphore_acquire[i],
			                   NULL);
		}
		FREE(self->semaphore_submit);
		FREE(self->semaphore_acquire);

		vkFreeCommandBuffers(self->device,
		                     self->command_pool,
		                     self->swapchain_image_count,
		                     self->command_buffers);
		FREE(self->command_buffers);

		vkk_engine_deleteFramebuffer(self);

		vkk_engine_deleteDepth(self);

		vkDestroyRenderPass(self->device,
		                    self->render_pass, NULL);

		vkk_engine_deleteSwapchain(self);

		vkDestroyCommandPool(self->device,
		                     self->command_pool, NULL);
		vkDestroyPipelineCache(self->device,
		                       self->pipeline_cache, NULL);
		vkDestroyDevice(self->device, NULL);
		vkDestroySurfaceKHR(self->instance,
		                    self->surface, NULL);
		vkDestroyInstance(self->instance, NULL);
		#ifndef ANDROID
			SDL_DestroyWindow(self->window);
			SDL_Quit();
		#endif
		FREE(self);
		*_self = NULL;
	}
}

void vkk_engine_waitForIdle(vkk_engine_t* self)
{
	assert(self);

	vkDeviceWaitIdle(self->device);
}

int vkk_engine_resize(vkk_engine_t* self,
                      uint32_t* _width,
                      uint32_t* _height)
{
	assert(self);
	assert(_width);
	assert(_height);

	vkDeviceWaitIdle(self->device);

	vkk_engine_deleteDepth(self);
	vkk_engine_deleteFramebuffer(self);
	vkk_engine_deleteSwapchain(self);

	if(vkk_engine_newSwapchain(self) == 0)
	{
		return 0;
	}

	if(vkk_engine_newDepth(self) == 0)
	{
		goto fail_depth;
	}

	if(vkk_engine_newFramebuffer(self) == 0)
	{
		goto fail_framebuffer;
	}

	*_width  = self->swapchain_extent.width;
	*_height = self->swapchain_extent.height;

	// success
	return 1;

	// failure
	fail_framebuffer:
		vkk_engine_deleteDepth(self);
	fail_depth:
		vkk_engine_deleteSwapchain(self);
	return 0;
}

int vkk_engine_beginFrame(vkk_engine_t* self,
                          float* clear_color)
{
	VkSemaphore semaphore_acquire;
	VkSemaphore semaphore_submit;
	vkk_engine_beginSemaphore(self,
	                          &semaphore_acquire,
	                          &semaphore_submit);

	uint64_t timeout = 250000000;
	if(vkAcquireNextImageKHR(self->device,
	                         self->swapchain,
	                         timeout,
	                         semaphore_acquire,
	                         VK_NULL_HANDLE,
	                         &self->swapchain_frame) != VK_SUCCESS)
	{
		// failure typically caused by resizes
		return 0;
	}

	VkFence sc_fence;
	sc_fence = self->swapchain_fences[self->swapchain_frame];
	vkWaitForFences(self->device, 1,
	                &sc_fence, VK_TRUE, UINT64_MAX);
	vkResetFences(self->device, 1, &sc_fence);

	VkCommandBuffer cb;
	cb = self->command_buffers[self->swapchain_frame];
	if(vkResetCommandBuffer(cb, 0) != VK_SUCCESS)
	{
		LOGE("vkResetCommandBuffer failed");
		return 0;
	}

	VkFramebuffer framebuffer;
	framebuffer = self->framebuffers[self->swapchain_frame];
	VkCommandBufferInheritanceInfo cbi_info =
	{
		.sType                = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		.pNext                = NULL,
		.renderPass           = self->render_pass,
		.subpass              = 0,
		.framebuffer          = framebuffer,
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

	if(vkBeginCommandBuffer(cb, &cb_info) != VK_SUCCESS)
	{
		LOGE("vkBeginCommandBuffer failed");
		return 0;
	}

	VkImage image;
	image = self->swapchain_images[self->swapchain_frame];
	VkImageMemoryBarrier imb =
	{
		.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext               = NULL,
		.srcAccessMask       = 0,
		.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image               = image,
		.subresourceRange    =
		{
			.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel   = 0,
			.levelCount     = 1,
			.baseArrayLayer = 0,
			.layerCount     = 1
		}
	};

	vkCmdPipelineBarrier(cb,
	                     VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
	                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
	                     0, 0, NULL, 0 ,NULL, 1, &imb);

	if(self->depth_image->transition)
	{

		VkImageMemoryBarrier imb_depth =
		{
			.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.pNext               = NULL,
			.srcAccessMask       = 0,
			.dstAccessMask       = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout           = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image               = self->depth_image->image,
			.subresourceRange    =
			{
				.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT |
				                  VK_IMAGE_ASPECT_STENCIL_BIT,
				.baseMipLevel   = 0,
				.levelCount     = 1,
				.baseArrayLayer = 0,
				.layerCount     = 1
			}
		};

		vkCmdPipelineBarrier(cb,
		                     VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		                     VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		                     0, 0, NULL, 0 ,NULL, 1, &imb_depth);

		self->depth_image->transition = 0;
	}

	VkViewport viewport =
	{
		.x        = 0.0f,
		.y        = 0.0f,
		.width    = (float) self->swapchain_extent.width,
		.height   = (float) self->swapchain_extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};
	vkCmdSetViewport(cb, 0, 1, &viewport);

	VkRect2D scissor =
	{
		.offset =
		{
			.x = 0,
			.y = 0
		},
		.extent =
		{
			.width  = (uint32_t) self->swapchain_extent.width,
			.height = (uint32_t) self->swapchain_extent.height,
		}
	};
	vkCmdSetScissor(cb, 0, 1, &scissor);

	VkClearValue cv[2] =
	{
		{
			.color =
			{
				.float32={ 0.0f, 0.0f, 0.0f, 1.0f }
			},
		},
		{
			.depthStencil =
			{
				.depth   = 1.0f,
				.stencil = 0
			}
		}
	};

	uint32_t width  = self->swapchain_extent.width;
	uint32_t height = self->swapchain_extent.height;
	VkRenderPassBeginInfo rp_info =
	{
		.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext           = NULL,
		.renderPass      = self->render_pass,
		.framebuffer     = framebuffer,
		.renderArea      = { { .x=0, .y=0 },
		                     { .width=width,
		                       .height=height } },
		.clearValueCount = 2,
		.pClearValues    = cv
	};

	vkCmdBeginRenderPass(cb,
	                     &rp_info,
	                     VK_SUBPASS_CONTENTS_INLINE);

	return 1;
}

void vkk_engine_endFrame(vkk_engine_t* self)
{
	assert(self);

	VkSemaphore semaphore_acquire;
	VkSemaphore semaphore_submit;
	vkk_engine_endSemaphore(self,
	                        &semaphore_acquire,
	                        &semaphore_submit);

	VkCommandBuffer cb;
	cb = self->command_buffers[self->swapchain_frame];
	vkCmdEndRenderPass(cb);
	vkEndCommandBuffer(cb);

	VkPipelineStageFlags wait_dst_stage_mask;
	wait_dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo s_info =
	{
		.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext                = NULL,
		.waitSemaphoreCount   = 1,
		.pWaitSemaphores      = &semaphore_acquire,
		.pWaitDstStageMask    = &wait_dst_stage_mask,
		.commandBufferCount   = 1,
		.pCommandBuffers      = &cb,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores    = &semaphore_submit
	};

	VkFence sc_fence;
	sc_fence = self->swapchain_fences[self->swapchain_frame];
	if(vkQueueSubmit(self->queue, 1, &s_info,
	                 sc_fence) != VK_SUCCESS)
	{
		LOGE("vkQueueSubmit failed");
		return;
	}

	VkPresentInfoKHR p_info =
	{
		.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext              = NULL,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores    = &semaphore_submit,
		.swapchainCount     = 1,
		.pSwapchains        = &self->swapchain,
		.pImageIndices      = &self->swapchain_frame,
		.pResults           = NULL
	};

	if(vkQueuePresentKHR(self->queue,
	                     &p_info) != VK_SUCCESS)
	{
		// failure typically caused by resizes
		return;
	}
}

vkk_buffer_t*
vkk_engine_newBuffer(vkk_engine_t* self, int dynamic,
                     int usage, size_t size,
                     const void* buf)
{
	// buf may be NULL
	assert(self);

	uint32_t count;
	count = dynamic ? self->swapchain_image_count : 1;

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
		uint32_t count;
		count = buffer->dynamic ? self->swapchain_image_count : 1;
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

void vkk_engine_updateBuffer(vkk_engine_t* self,
                             vkk_buffer_t* buffer,
                             const void* buf)
{
	assert(self);
	assert(buffer);
	assert(buf);

	uint32_t idx;
	idx = buffer->dynamic ? self->swapchain_frame : 0;

	void* data;
	if(vkMapMemory(self->device,
	               buffer->memory[idx],
	               0, buffer->size, 0,
	               &data) == VK_SUCCESS)
	{
		memcpy(data, buf, buffer->size);
		vkUnmapMemory(self->device,
		              buffer->memory[idx]);
	}
	else
	{
		LOGW("vkMapMemory failed");
	}
}

vkk_image_t* vkk_engine_newImage(vkk_engine_t* self,
                                 uint32_t width,
                                 uint32_t height,
                                 int format,
                                 int mipmap,
                                 const void* pixels)
{
	// pixels may be NULL for depth buffer
	assert(self);

	// TODO - add mipmap support

	vkk_image_t* image;
	image = (vkk_image_t*) CALLOC(1, sizeof(vkk_image_t));
	if(image == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	image->width      = width;
	image->height     = height;
	image->format     = format;
	image->transition = 1;

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
		usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		aspectMask    = VK_IMAGE_ASPECT_DEPTH_BIT |
		                VK_IMAGE_ASPECT_STENCIL_BIT;
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
		.mipLevels   = 1,
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
			.levelCount     = 1,
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

	// textures must upload pixel data
	if(format != VKK_IMAGE_FORMAT_DEPTH)
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
		vkDestroyImageView(self->device, image->image_view,
		                   NULL);
		vkFreeMemory(self->device, image->memory, NULL);
		vkDestroyImage(self->device, image->image, NULL);
		FREE(image);
		*_image = NULL;
	}
}

vkk_uniformSetFactory_t*
vkk_engine_newUniformSetFactory(vkk_engine_t* self,
                                int dynamic,
                                uint32_t count,
                                vkk_uniformBinding_t* ub_array)
{
	assert(self);
	assert(ub_array);

	VkDescriptorType dt_map[VKK_UNIFORM_TYPE_COUNT] =
	{
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_DESCRIPTOR_TYPE_SAMPLER,
	};

	VkShaderStageFlags stage_map[4] =
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

	usf->dynamic = dynamic;

	// create temportary descriptor set layout bindings
	VkDescriptorSetLayoutBinding* bindings;
	bindings = (VkDescriptorSetLayoutBinding*)
	           CALLOC(count, sizeof(VkDescriptorSetLayoutBinding));
	if(bindings == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_bindings;
	}

	// fill in bindings
	int i;
	for(i = 0; i < count; ++i)
	{
		vkk_uniformBinding_t*         usb = &(ub_array[i]);
		VkDescriptorSetLayoutBinding* b   = &(bindings[i]);
		b->binding            = usb->binding;
		b->descriptorType     = dt_map[usb->type];
		b->descriptorCount    = 1;
		b->stageFlags         = stage_map[usb->stage];
		b->pImmutableSamplers = usb->sampler ? &usb->sampler->sampler : NULL;
	}

	VkDescriptorSetLayoutCreateInfo dsl_info =
	{
		.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext        = NULL,
		.flags        = 0,
		.bindingCount = count,
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
	for(i = 0; i < count; ++i)
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
		FREE(usf);
		*_usf = NULL;
	}
}

vkk_uniformSet_t*
vkk_engine_newUniformSet(vkk_engine_t* self,
                         vkk_uniformSetFactory_t* usf)
{
	assert(self);
	assert(usf);

	// check if a uniform set can be reused
	vkk_uniformSet_t* us;
	cc_listIter_t* iter = cc_list_head(usf->us_list);
	if(iter)
	{
		us = (vkk_uniformSet_t*)
		     cc_list_remove(usf->us_list, &iter);
		return us;
	}

	// create a new uniform set
	us = (vkk_uniformSet_t*)
	     CALLOC(1, sizeof(vkk_uniformSet_t));
	if(us == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

	uint32_t count;
	count = usf->dynamic ? self->swapchain_image_count : 1;

	us->usf      = usf;
	us->ds_array = (VkDescriptorSet*)
	               CALLOC(count, sizeof(VkDescriptorSet));
	if(us->ds_array == NULL)
	{
		LOGE("CALLOC failed");
		goto fail_alloc;
	}

	// initialize the descriptor set layouts
	VkDescriptorSetLayout dsl_array[VKK_DESCRIPTOR_POOL_SIZE];
	int i;
	for(i = 0; i < count; ++i)
	{
		dsl_array[i] = usf->ds_layout;
	}

	// allocate the descriptor set from the pool
	int retry = 1;
	VkDescriptorPool dp;
	dp = (VkDescriptorPool)
	     cc_list_peekTail(usf->dp_list);
	while(retry)
	{
		// create a new pool on demand
		if((count > usf->ds_available) || (dp == VK_NULL_HANDLE))
		{
			// create a new pool
			dp = vkk_engine_newDescriptorPool(self, usf);
			if(dp == VK_NULL_HANDLE)
			{
				goto fail_dsp;
			}

			retry = 0;
		}

		VkDescriptorSetAllocateInfo ds_info =
		{
			.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext              = NULL,
			.descriptorPool     = dp,
			.descriptorSetCount = count,
			.pSetLayouts        = dsl_array
		};

		if(vkAllocateDescriptorSets(self->device, &ds_info,
		                            us->ds_array) != VK_SUCCESS)
		{
			// retry with a new pool
			if(retry)
			{
				dp = VK_NULL_HANDLE;
				continue;
			}
			else
			{
				LOGE("vkAllocateDescriptorSets failed");
				goto fail_allocate_ds;
			}
		}

		usf->ds_available -= count;
		break;
	}

	// success
	return us;

	// failure
	fail_allocate_ds:
	fail_dsp:
		FREE(us->ds_array);
	fail_alloc:
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
		if(cc_list_append(us->usf->us_list, NULL,
		                  (const void*) us) == NULL)
		{
			// when an error occurs the uniform set will
			// be unreachable from the factory but the
			// uniform set will still be freed when the
			// corresponding uniform set factory is freed
			FREE(us->ds_array);
			FREE(us);
		}
		*_us = NULL;
	}
}

void vkk_engine_attachUniformBuffer(vkk_engine_t* self,
                                    vkk_uniformSet_t* us,
                                    vkk_buffer_t* buffer,
                                    uint32_t binding)
{
	assert(self);
	assert(us);
	assert(buffer);

	uint32_t count;
	count = us->usf->dynamic ? self->swapchain_image_count : 1;

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

void vkk_engine_bindUniformSet(vkk_engine_t* self,
                               vkk_pipelineLayout_t* pl,
                               vkk_uniformSet_t* us)
{
	assert(self);
	assert(pl);
	assert(us);

	uint32_t idx;
	idx = us->usf->dynamic ? self->swapchain_frame : 0;

	VkDescriptorSet ds = us->ds_array[idx];

	VkCommandBuffer cb;
	cb = self->command_buffers[self->swapchain_frame];
	vkCmdBindDescriptorSets(cb,
	                        VK_PIPELINE_BIND_POINT_GRAPHICS,
	                        pl->pl, 0, 1, &ds,
	                        0, NULL);
}

vkk_pipelineLayout_t*
vkk_engine_newPipelineLayout(vkk_engine_t* self,
                             uint32_t usf_count,
                             vkk_uniformSetFactory_t** usf_array)
{
	assert(self);
	assert(usf_array);

	vkk_pipelineLayout_t* pl;
	pl = (vkk_pipelineLayout_t*)
	     CALLOC(1, sizeof(vkk_pipelineLayout_t));
	if(pl == NULL)
	{
		LOGE("CALLOC failed");
		return NULL;
	}

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
		.vertexBindingDescriptionCount   = 2,
		.pVertexBindingDescriptions      = vib,
		.vertexAttributeDescriptionCount = 2,
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
		.width    = (float) self->swapchain_extent.width,
		.height   = (float) self->swapchain_extent.height,
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
			.width  = (uint32_t) self->swapchain_extent.width,
			.height = (uint32_t) self->swapchain_extent.height,
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

	// TODO - gpi->blend_mode
	VkPipelineColorBlendAttachmentState pcbs =
	{
		.blendEnable         = VK_FALSE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
		.colorBlendOp        = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp        = VK_BLEND_OP_ADD,
		.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT |
		                       VK_COLOR_COMPONENT_G_BIT |
		                       VK_COLOR_COMPONENT_B_BIT |
		                       VK_COLOR_COMPONENT_A_BIT,
	};

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
		.renderPass          = self->render_pass,
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
		vkDestroyPipeline(self->device,
		                  gp->pipeline, NULL);
		FREE(gp);
		*_gp = NULL;
	}
}

void vkk_engine_bindGraphicsPipeline(vkk_engine_t* self,
                                     vkk_graphicsPipeline_t* gp)
{
	assert(self);
	assert(gp);

	VkCommandBuffer cb;
	cb = self->command_buffers[self->swapchain_frame];
	vkCmdBindPipeline(cb,
	                  VK_PIPELINE_BIND_POINT_GRAPHICS,
	                  gp->pipeline);
}

void vkk_engine_draw(vkk_engine_t* self,
                     uint32_t vertex_count,
                     uint32_t vertex_buffer_count,
                     vkk_buffer_t** vertex_buffers)
{
	assert(self);
	assert(vertex_buffers);
	assert(vertex_buffer_count < 16);

	VkDeviceSize vb_offsets[16] =
	{
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0
	};
	VkBuffer vb_buffers[16];

	// fill in the vertex buffers
	int i = 0;
	int idx;
	for(i = 0; i < vertex_buffer_count; ++i)
	{
		idx = vertex_buffers[i]->dynamic ?
		      self->swapchain_frame : 0;
		vb_buffers[i] = vertex_buffers[i]->buffer[idx];
	}

	VkCommandBuffer cb;
	cb = self->command_buffers[self->swapchain_frame];

	vkCmdBindVertexBuffers(cb, 0, vertex_buffer_count,
	                       vb_buffers, vb_offsets);
	vkCmdDraw(cb, vertex_count, 1, 0, 0);
}

void vkk_engine_drawIndexed(vkk_engine_t* self,
                            uint32_t vertex_count,
                            uint32_t vertex_buffer_count,
                            int index_type,
                            vkk_buffer_t* index_buffer,
                            vkk_buffer_t** vertex_buffers)
{
	assert(self);
	assert(index_buffer);
	assert(vertex_buffers);
	assert(vertex_buffer_count < 16);

	VkDeviceSize vb_offsets[16] =
	{
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0
	};
	VkBuffer vb_buffers[16];

	// fill in the vertex buffers
	int i = 0;
	int idx;
	for(i = 0; i < vertex_buffer_count; ++i)
	{
		idx = vertex_buffers[i]->dynamic ?
		      self->swapchain_frame : 0;
		vb_buffers[i] = vertex_buffers[i]->buffer[idx];
	}

	VkCommandBuffer cb;
	cb = self->command_buffers[self->swapchain_frame];

	VkIndexType it_map[VKK_INDEX_TYPE_COUNT] =
	{
		VK_INDEX_TYPE_UINT16,
		VK_INDEX_TYPE_UINT32
	};

	idx = index_buffer->dynamic ? self->swapchain_frame : 0;
	VkBuffer ib_buffer = index_buffer->buffer[idx];

	vkCmdBindIndexBuffer(cb, ib_buffer, 0,
	                     it_map[index_type]);
	vkCmdBindVertexBuffers(cb, 0, vertex_buffer_count,
	                       vb_buffers, vb_offsets);
	vkCmdDrawIndexed(cb, vertex_count, 1, 0, 0, 0);
}
