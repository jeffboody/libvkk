/*
 * Copyright (c) 2020 Jeff Boody
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

#ifndef vkk_imageUploader_H
#define vkk_imageUploader_H

#include <pthread.h>
#ifdef ANDROID
	#include <vulkan_wrapper.h>
#else
	#include <vulkan/vulkan.h>
#endif

#include "../libcc/cc_list.h"
#include "../libcc/cc_multimap.h"
#include "vkk.h"

typedef struct vkk_imageUploader_s
{
	vkk_engine_t* engine;

	int shutdown;

	// uploader instance
	cc_list_t* instance_list;

	// multimap from size to uploaderBuffers
	cc_multimap_t* buffer_map;

	pthread_mutex_t mutex;
} vkk_imageUploader_t;

vkk_imageUploader_t* vkk_imageUploader_new(vkk_engine_t* engine);
void                 vkk_imageUploader_delete(vkk_imageUploader_t** _self);
void                 vkk_imageUploader_shutdown(vkk_imageUploader_t* self);
int                  vkk_imageUploader_upload(vkk_imageUploader_t* self,
                                              vkk_image_t* image,
                                              const void* pixels);

#endif
