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

#ifndef vkk_xferManager_H
#define vkk_xferManager_H

#include <pthread.h>
#include <vulkan/vulkan.h>

#include "../../libcc/cc_list.h"
#include "../../libcc/cc_multimap.h"
#include "../vkk.h"

typedef enum
{
	VKK_XFER_MODE_READ  = 0,
	VKK_XFER_MODE_WRITE = 1,
} vkk_xferMode_e;

typedef struct vkk_xferManager_s
{
	vkk_engine_t* engine;

	int shutdown;

	// xfer instance
	cc_list_t* instance_list;

	// multimap from size to xfer buffer
	cc_multimap_t* buffer_map;

	pthread_mutex_t mutex;
} vkk_xferManager_t;

vkk_xferManager_t* vkk_xferManager_new(vkk_engine_t* engine);
void               vkk_xferManager_delete(vkk_xferManager_t** _self);
void               vkk_xferManager_shutdown(vkk_xferManager_t* self);
int                vkk_xferManager_fillStorage(vkk_xferManager_t* self,
                                               vkk_buffer_t* buffer,
                                               size_t offset,
                                               size_t size,
                                               uint32_t data);
int                vkk_xferManager_blitStorage(vkk_xferManager_t* self,
                                               vkk_xferMode_e mode,
                                               vkk_buffer_t* buffer,
                                               size_t offset,
                                               size_t size,
                                               void* data);
int                vkk_xferManager_blitStorage2(vkk_xferManager_t* self,
                                                vkk_buffer_t* src_buffer,
                                                vkk_buffer_t* dst_buffer,
                                                size_t src_offset,
                                                size_t dst_offset,
                                                size_t size);
int                vkk_xferManager_readImage(vkk_xferManager_t* self,
                                             vkk_image_t* image,
                                             void* pixels);
int                vkk_xferManager_writeImage(vkk_xferManager_t* self,
                                              vkk_image_t* image,
                                              const void* pixels);

#endif
