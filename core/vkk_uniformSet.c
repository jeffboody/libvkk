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

#define LOG_TAG "vkk"
#include "../../libcc/cc_log.h"
#include "../../libcc/cc_memory.h"
#include "vkk_buffer.h"
#include "vkk_defaultRenderer.h"
#include "vkk_engine.h"
#include "vkk_uniformSet.h"
#include "vkk_uniformSetFactory.h"
#include "vkk_util.h"

/***********************************************************
* public                                                   *
***********************************************************/

vkk_uniformSet_t*
vkk_uniformSet_new(vkk_engine_t* engine,
                   uint32_t set,
                   uint32_t ua_count,
                   vkk_uniformAttachment_t* ua_array,
                   vkk_uniformSetFactory_t* usf)
{
	// ua_array may be NULL when attachments are refs
	ASSERT(engine);
	ASSERT((ua_array           && (ua_count > 0)) ||
	       ((ua_array == NULL) && (ua_count == 0)));
	ASSERT(usf);

	// validate the update method
	int i;
	#ifdef ASSERT_DEBUG
		for(i = 0; i < ua_count; ++i)
		{
			if((ua_array[i].type == VKK_UNIFORM_TYPE_BUFFER) ||
			   (ua_array[i].type == VKK_UNIFORM_TYPE_STORAGE))
			{
				vkk_buffer_t* buffer = ua_array[i].buffer;
				ASSERT((buffer->update == usf->update) ||
				       (buffer->update == VKK_UPDATE_MODE_STATIC));
			}
		}
	#endif

	// get the last expired timestamp
	double ets = 0.0;
	vkk_renderer_t* renderer = engine->renderer;
	if(renderer)
	{
		vkk_engine_rendererLock(engine);
		ets = vkk_defaultRenderer_tsExpiredLocked(renderer);
		vkk_engine_rendererUnlock(engine);
	}

	// check if a uniform set can be reused
	vkk_engine_usfLock(engine);
	vkk_uniformSet_t* self = NULL;
	cc_listIter_t*    iter = cc_list_head(usf->us_list);
	while(iter)
	{
		vkk_uniformSet_t* tmp;
		tmp = (vkk_uniformSet_t*)
		      cc_list_peekIter(iter);

		if(ets >= tmp->ts)
		{
			self = tmp;
			cc_list_remove(usf->us_list, &iter);
			break;
		}

		iter = cc_list_next(iter);
	}
	vkk_engine_usfUnlock(engine);

	if(self == NULL)
	{
		// create a new uniform set
		self = (vkk_uniformSet_t*)
		       CALLOC(1, sizeof(vkk_uniformSet_t));
		if(self == NULL)
		{
			LOGE("CALLOC failed");
			return NULL;
		}

		self->engine   = engine;
		self->set      = set;
		self->ua_count = usf->ub_count;
		self->usf      = usf;

		// copy the ua_array
		self->ua_array = (vkk_uniformAttachment_t*)
		                 CALLOC(usf->ub_count,
		                        sizeof(vkk_uniformAttachment_t));
		if(self->ua_array == NULL)
		{
			LOGE("CALLOC failed");
			goto fail_ua_array;
		}
		vkk_util_copyUniformAttachmentArray(self->ua_array,
		                                    ua_count,
		                                    ua_array, usf);

		uint32_t ds_count;
		ds_count = (usf->update == VKK_UPDATE_MODE_ASYNCHRONOUS) ?
		           vkk_engine_imageCount(engine) : 1;
		self->ds_array = (VkDescriptorSet*)
		                 CALLOC(ds_count, sizeof(VkDescriptorSet));
		if(self->ds_array == NULL)
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
		vkk_engine_usfLock(engine);
		VkDescriptorPool dp;
		dp = (VkDescriptorPool)
		     cc_list_peekTail(usf->dp_list);

		// create a new pool on demand
		if((ds_count > usf->ds_available) || (dp == VK_NULL_HANDLE))
		{
			// create a new pool
			dp = vkk_engine_newDescriptorPoolLocked(engine, usf);
			if(dp == VK_NULL_HANDLE)
			{
				vkk_engine_usfUnlock(engine);
				goto fail_dp;
			}
		}

		if(vkk_engine_allocateDescriptorSetsLocked(engine, dp,
		                                           dsl_array,
		                                           ds_count,
		                                           self->ds_array) == 0)
		{
			vkk_engine_usfUnlock(engine);
			goto fail_allocate_ds;
		}

		usf->ds_available -= ds_count;
		vkk_engine_usfUnlock(engine);
	}
	else
	{
		// reuse the uniform set
		self->ts  = 0.0;
		self->set = set;
		self->usf = usf;

		// copy the ua_array
		memset(self->ua_array, 0,
		       usf->ub_count*sizeof(vkk_uniformAttachment_t));
		vkk_util_copyUniformAttachmentArray(self->ua_array,
		                                    ua_count,
		                                    ua_array, usf);
	}

	// attach buffers and images
	for(i = 0; i < ua_count; ++i)
	{
		if((ua_array[i].type == VKK_UNIFORM_TYPE_BUFFER) ||
		   (ua_array[i].type == VKK_UNIFORM_TYPE_STORAGE))
		{
			vkk_engine_attachUniformBuffer(engine, self,
			                               &ua_array[i]);
		}
		else if(ua_array[i].type == VKK_UNIFORM_TYPE_IMAGE)
		{
			uint32_t b = ua_array[i].binding;
			vkk_engine_attachUniformSampler(engine, self,
			                                &usf->ub_array[b].si,
			                                ua_array[i].image,
			                                ua_array[i].binding);
		}
	}

	vkk_uniformSetFactory_incRef(usf);

	// success
	return self;

	// failure
	fail_allocate_ds:
	fail_dp:
		FREE(self->ds_array);
	fail_ds_array:
		FREE(self->ua_array);
	fail_ua_array:
		FREE(self);
	return NULL;
}

void vkk_uniformSet_delete(vkk_uniformSet_t** _self)
{
	ASSERT(_self);

	vkk_uniformSet_t* self = *_self;
	if(self)
	{
		vkk_engine_deleteObject(self->engine,
		                        VKK_OBJECT_TYPE_UNIFORMSET,
		                        (void*) self);
		*_self = NULL;
	}
}
