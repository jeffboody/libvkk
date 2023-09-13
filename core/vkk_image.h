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

#ifndef vkk_image_H
#define vkk_image_H

#include <vulkan/vulkan.h>

#include "../vkk.h"

typedef struct vkk_image_s
{
	vkk_engine_t*     engine;
	double            ts;
	uint32_t          width;
	uint32_t          height;
	uint32_t          depth;
	vkk_imageFormat_e format;
	int               mipmap;
	vkk_stage_e       stage;
	uint32_t          mip_levels;
	VkImageLayout*    layout_array;
	VkImage           image;
	vkk_memory_t*     memory;
	VkImageView       image_view;
	VkSemaphore       semaphore;
} vkk_image_t;

// protected
int vkk_image_createSemaphore(vkk_image_t* self);

#endif
