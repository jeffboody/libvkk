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

#ifndef vkk_uniformSetFactory_H
#define vkk_uniformSetFactory_H

#include <vulkan/vulkan.h>

#include "../vkk.h"

// this value corresponds to the Vulkan required supported
// limit for maxBoundDescriptorSets
#define VKK_ENGINE_MAX_USF_COUNT 4

typedef struct vkk_uniformSetFactory_s
{
	vkk_engine_t*         engine;
	uint32_t              ref_count;
	vkk_updateMode_e      update;
	uint32_t              ub_count;
	vkk_uniformBinding_t* ub_array;
	uint32_t              ds_available;
	VkDescriptorSetLayout ds_layout;
	cc_list_t*            dp_list;
	cc_list_t*            us_list;
	char                  type_count[VKK_UNIFORM_TYPE_COUNT];
} vkk_uniformSetFactory_t;

// protected
void vkk_uniformSetFactory_incRef(vkk_uniformSetFactory_t* self);
void vkk_uniformSetFactory_decRef(vkk_uniformSetFactory_t* self);

#endif
