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
#include "../libcc/cc_log.h"
#include "vkk_buffer.h"
#include "vkk_defaultRenderer.h"
#include "vkk_engine.h"
#include "vkk_graphicsPipeline.h"
#include "vkk_image.h"
#include "vkk_imageRenderer.h"
#include "vkk_memoryManager.h"
#include "vkk_pipelineLayout.h"
#include "vkk_renderer.h"
#include "vkk_secondaryRenderer.h"
#include "vkk_uniformSet.h"
#include "vkk_uniformSetFactory.h"

/***********************************************************
* private                                                  *
***********************************************************/

static void
vkk_fillUniformAttachmentArray(vkk_uniformAttachment_t* dst,
                               uint32_t src_ua_count,
                               vkk_uniformAttachment_t* src,
                               vkk_uniformSetFactory_t* usf)
{
	ASSERT(dst);
	ASSERT(src);
	ASSERT(usf);

	// dst binding/type already filled in
	// validate and fill buffer/image union
	uint32_t i;
	for(i = 0; i < src_ua_count; ++i)
	{
		uint32_t b = src[i].binding;
		ASSERT(b < usf->ub_count);
		ASSERT(b == dst[b].binding);
		ASSERT((src[i].type == VKK_UNIFORM_TYPE_BUFFER_REF) ||
		       (src[i].type == VKK_UNIFORM_TYPE_IMAGE_REF));
		ASSERT(src[i].type == dst[b].type);

		dst[b].buffer = src[i].buffer;
	}
}

static void
vkk_renderer_updateUniformBufferRef(vkk_renderer_t* self,
                                    vkk_uniformSet_t* us,
                                    uint32_t swapchain_frame,
                                    vkk_buffer_t* buffer,
                                    uint32_t binding)
{
	ASSERT(self);
	ASSERT(us);
	ASSERT(buffer);

	vkk_engine_t* engine = self->engine;

	uint32_t idx;
	idx = (buffer->update == VKK_UPDATE_MODE_ASYNCHRONOUS) ?
	      swapchain_frame : 0;
	VkDescriptorBufferInfo db_info =
	{
		.buffer  = buffer->buffer[idx],
		.offset  = 0,
		.range   = buffer->size
	};

	idx = (us->usf->update == VKK_UPDATE_MODE_ASYNCHRONOUS) ?
	      swapchain_frame : 0;
	VkWriteDescriptorSet writes =
	{
		.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext            = NULL,
		.dstSet           = us->ds_array[idx],
		.dstBinding       = binding,
		.dstArrayElement  = 0,
		.descriptorCount  = 1,
		.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.pImageInfo       = NULL,
		.pBufferInfo      = &db_info,
		.pTexelBufferView = NULL,
	};

	vkUpdateDescriptorSets(engine->device, 1, &writes,
	                       0, NULL);
}

static void
vkk_renderer_updateUniformImageRef(vkk_renderer_t* self,
                                   vkk_uniformSet_t* us,
                                   uint32_t swapchain_frame,
                                   vkk_samplerInfo_t* si,
                                   vkk_image_t* image,
                                   uint32_t binding)
{
	ASSERT(self);
	ASSERT(us);
	ASSERT(si);
	ASSERT(image);

	vkk_engine_t* engine = self->engine;

	VkSampler* samplerp;
	samplerp = vkk_engine_getSamplerp(engine, si);
	if(samplerp == NULL)
	{
		// ignore;
		return;
	}

	VkDescriptorImageInfo di_info =
	{
		.sampler     = *samplerp,
		.imageView   = image->image_view,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	uint32_t idx;
	idx = (us->usf->update == VKK_UPDATE_MODE_ASYNCHRONOUS) ?
	      swapchain_frame : 0;
	VkWriteDescriptorSet writes =
	{
		.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext            = NULL,
		.dstSet           = us->ds_array[idx],
		.dstBinding       = binding,
		.dstArrayElement  = 0,
		.descriptorCount  = 1,
		.descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo       = &di_info,
		.pBufferInfo      = NULL,
		.pTexelBufferView = NULL,
	};

	vkUpdateDescriptorSets(engine->device, 1, &writes,
	                       0, NULL);
}

static vkk_renderer_t*
vkk_renderer_getUpdater(vkk_renderer_t* self)
{
	ASSERT(self);

	if(self->type == VKK_RENDERER_TYPE_SECONDARY)
	{
		vkk_secondaryRenderer_t* sec;
		sec = (vkk_secondaryRenderer_t*) self;
		return sec->executor;
	}

	return self;
}

/***********************************************************
* protected                                                *
***********************************************************/

void vkk_renderer_init(vkk_renderer_t* self,
                       vkk_rendererType_e type,
                       vkk_engine_t* engine)
{
	ASSERT(self);
	ASSERT(engine);

	self->engine = engine;
	self->type   = type;
	self->mode   = VKK_RENDERER_MODE_DRAW;
}

VkRenderPass
vkk_renderer_renderPass(vkk_renderer_t* self)
{
	ASSERT(self);

	if(self->type == VKK_RENDERER_TYPE_DEFAULT)
	{
		return vkk_defaultRenderer_renderPass(self);
	}
	else if(self->type == VKK_RENDERER_TYPE_IMAGE)
	{
		return vkk_imageRenderer_renderPass(self);
	}
	else
	{
		return vkk_secondaryRenderer_renderPass(self);
	}
}

VkFramebuffer
vkk_renderer_framebuffer(vkk_renderer_t* self)
{
	ASSERT(self);

	if(self->type == VKK_RENDERER_TYPE_DEFAULT)
	{
		return vkk_defaultRenderer_framebuffer(self);
	}
	else if(self->type == VKK_RENDERER_TYPE_IMAGE)
	{
		return vkk_imageRenderer_framebuffer(self);
	}
	else
	{
		return vkk_secondaryRenderer_framebuffer(self);
	}
}

uint32_t
vkk_renderer_swapchainFrame(vkk_renderer_t* self)
{
	ASSERT(self);

	if(self->type == VKK_RENDERER_TYPE_DEFAULT)
	{
		return vkk_defaultRenderer_swapchainFrame(self);
	}
	else if(self->type == VKK_RENDERER_TYPE_IMAGE)
	{
		return vkk_imageRenderer_swapchainFrame(self);
	}
	else
	{
		return vkk_secondaryRenderer_swapchainFrame(self);
	}
}

VkCommandBuffer
vkk_renderer_commandBuffer(vkk_renderer_t* self)
{
	ASSERT(self);

	if(self->type == VKK_RENDERER_TYPE_DEFAULT)
	{
		return vkk_defaultRenderer_commandBuffer(self);
	}
	else if(self->type == VKK_RENDERER_TYPE_IMAGE)
	{
		return vkk_imageRenderer_commandBuffer(self);
	}
	else
	{
		return vkk_secondaryRenderer_commandBuffer(self);
	}
}

double
vkk_renderer_tsCurrent(vkk_renderer_t* self)
{
	ASSERT(self);

	double ts = 0.0;
	if(self->type == VKK_RENDERER_TYPE_DEFAULT)
	{
		ts = vkk_defaultRenderer_tsCurrent(self);
	}
	else if(self->type == VKK_RENDERER_TYPE_SECONDARY)
	{
		vkk_secondaryRenderer_t* sec;
		sec = (vkk_secondaryRenderer_t*) self;

		if(sec->executor->type == VKK_RENDERER_TYPE_DEFAULT)
		{
			ts = vkk_defaultRenderer_tsCurrent(sec->executor);
		}
	}

	return ts;
}

/***********************************************************
* public                                                   *
***********************************************************/

vkk_renderer_t*
vkk_renderer_newImage(vkk_engine_t* engine,
                      uint32_t width, uint32_t height,
                      vkk_imageFormat_e format)
{
	ASSERT(engine);

	return vkk_imageRenderer_new(engine, width, height,
	                             format);
}

vkk_renderer_t*
vkk_renderer_newSecondary(vkk_renderer_t* executor)
{
	ASSERT(executor);

	return vkk_secondaryRenderer_new(executor);
}

void vkk_renderer_delete(vkk_renderer_t** _self)
{
	ASSERT(_self);

	vkk_renderer_t* self = *_self;
	if(self)
	{
		vkk_engine_t* engine = self->engine;

		// do not delete default renderer
		if(self->type == VKK_RENDERER_TYPE_DEFAULT)
		{
			return;
		}

		vkk_engine_deleteObject(engine, VKK_OBJECT_TYPE_RENDERER,
		                        (void*) self);
		*_self = NULL;
	}
}

int vkk_renderer_beginDefault(vkk_renderer_t* self,
                              vkk_rendererMode_e mode,
                              float* clear_color)
{
	ASSERT(self);
	ASSERT((mode == VKK_RENDERER_MODE_DRAW) ||
	       (mode == VKK_RENDERER_MODE_EXECUTE));
	ASSERT(clear_color);
	ASSERT(self->type == VKK_RENDERER_TYPE_DEFAULT);

	vkk_engine_t* engine = self->engine;

	vkk_engine_rendererLock(engine);
	if(engine->shutdown)
	{
		LOGE("invalid");
		vkk_engine_rendererUnlock(engine);
		return 0;
	}
	vkk_engine_rendererUnlock(engine);

	if(vkk_defaultRenderer_begin(self, mode,
	                             clear_color) == 0)
	{
		return 0;
	}

	self->mode = mode;
	return 1;
}

int vkk_renderer_beginImage(vkk_renderer_t* self,
                            vkk_rendererMode_e mode,
                            vkk_image_t* image,
                            float* clear_color)
{
	ASSERT(self);
	ASSERT((mode == VKK_RENDERER_MODE_DRAW) ||
	       (mode == VKK_RENDERER_MODE_EXECUTE));
	ASSERT(image);
	ASSERT(clear_color);
	ASSERT(self->type == VKK_RENDERER_TYPE_IMAGE);

	vkk_engine_t* engine = self->engine;

	vkk_engine_rendererLock(engine);
	if(engine->shutdown)
	{
		LOGE("invalid");
		vkk_engine_rendererUnlock(engine);
		return 0;
	}
	vkk_engine_rendererUnlock(engine);

	if(vkk_imageRenderer_begin(self, mode, image,
	                           clear_color) == 0)
	{
		return 0;
	}

	self->mode = mode;
	return 1;
}

int vkk_renderer_beginSecondary(vkk_renderer_t* self)
{
	ASSERT(self);
	ASSERT(self->type == VKK_RENDERER_TYPE_SECONDARY);

	vkk_engine_t* engine = self->engine;

	vkk_engine_rendererLock(engine);
	if(engine->shutdown)
	{
		LOGE("invalid");
		vkk_engine_rendererUnlock(engine);
		return 0;
	}
	vkk_engine_rendererUnlock(engine);

	return vkk_secondaryRenderer_begin(self);
}

void vkk_renderer_end(vkk_renderer_t* self)
{
	ASSERT(self);

	if(self->type == VKK_RENDERER_TYPE_DEFAULT)
	{
		vkk_defaultRenderer_end(self);
	}
	else if(self->type == VKK_RENDERER_TYPE_IMAGE)
	{
		vkk_imageRenderer_end(self);
	}
	else
	{
		vkk_secondaryRenderer_end(self);
	}

	self->mode = VKK_RENDERER_MODE_DRAW;
	self->gp   = NULL;
}

void vkk_renderer_surfaceSize(vkk_renderer_t* self,
                              uint32_t* _width,
                              uint32_t* _height)
{
	ASSERT(self);

	if(self->type == VKK_RENDERER_TYPE_DEFAULT)
	{
		vkk_defaultRenderer_surfaceSize(self, _width, _height);
	}
	else if(self->type == VKK_RENDERER_TYPE_IMAGE)
	{
		vkk_imageRenderer_surfaceSize(self, _width, _height);
	}
	else
	{
		vkk_secondaryRenderer_surfaceSize(self, _width, _height);
	}
}

void vkk_renderer_updateBuffer(vkk_renderer_t* self,
                               vkk_buffer_t* buffer,
                               size_t size,
                               const void* buf)
{
	ASSERT(self);
	ASSERT(buffer);
	ASSERT(size > 0);
	ASSERT(buf);
	ASSERT(self->mode == VKK_RENDERER_MODE_DRAW);

	vkk_engine_t* engine = self->engine;

	if(size > buffer->size)
	{
		LOGE("invalid size=%i buffer->size=%i",
		     (int) size, (int) buffer->size);
		return;
	}

	// 1) update may NOT be STATIC
	// 2) update may NOT be ASYNCHRONOUS if the draw renderer
	//    is not the default renderer
	// 3) update may NOT be SYNCHRONOUS if the draw renderer
	//    is not the image renderer
	vkk_renderer_t* updater = vkk_renderer_getUpdater(self);
	if(buffer->update == VKK_UPDATE_MODE_STATIC)
	{
		LOGW("invalid static update mode");
		return;
	}
	else if((buffer->update == VKK_UPDATE_MODE_ASYNCHRONOUS) &&
	        (updater->type != VKK_RENDERER_TYPE_DEFAULT))
	{
		LOGW("invalid type=%i", updater->type);
		return;
	}
	else if((buffer->update == VKK_UPDATE_MODE_SYNCHRONOUS) &&
	        (updater->type != VKK_RENDERER_TYPE_IMAGE))
	{
		LOGW("invalid type=%i", updater->type);
		return;
	}

	uint32_t idx = 0;
	if(buffer->update == VKK_UPDATE_MODE_ASYNCHRONOUS)
	{
		if((buffer->usage == VKK_BUFFER_USAGE_VERTEX) ||
		   (buffer->usage == VKK_BUFFER_USAGE_INDEX))
		{
			uint32_t count;
			count = (buffer->update == VKK_UPDATE_MODE_ASYNCHRONOUS) ?
			        vkk_engine_swapchainImageCount(engine) : 1;
			buffer->vbib_index = (buffer->vbib_index + 1)%count;
			idx = buffer->vbib_index;
		}
		else
		{
			idx = vkk_renderer_swapchainFrame(self);
		}
	}

	vkk_memoryManager_update(engine->mm, buffer->memory[idx],
	                         size, buf);
}

void
vkk_renderer_updateUniformSetRefs(vkk_renderer_t* self,
                                  vkk_uniformSet_t* us,
                                  uint32_t ua_count,
                                  vkk_uniformAttachment_t* ua_array)
{
	ASSERT(self);
	ASSERT(us);
	ASSERT(ua_array);
	ASSERT(self->mode == VKK_RENDERER_MODE_DRAW);

	vkk_uniformSetFactory_t* usf = us->usf;

	vkk_fillUniformAttachmentArray(us->ua_array, ua_count,
	                               ua_array, usf);

	uint32_t swapchain_frame;
	swapchain_frame = vkk_renderer_swapchainFrame(self);

	// attach buffers and images
	uint32_t i;
	for(i = 0; i < ua_count; ++i)
	{
		if(ua_array[i].type == VKK_UNIFORM_TYPE_BUFFER_REF)
		{
			vkk_renderer_updateUniformBufferRef(self, us,
			                                    swapchain_frame,
			                                    ua_array[i].buffer,
			                                    ua_array[i].binding);
		}
		else if(ua_array[i].type == VKK_UNIFORM_TYPE_IMAGE_REF)
		{
			uint32_t b = ua_array[i].binding;
			vkk_renderer_updateUniformImageRef(self, us,
			                                   swapchain_frame,
			                                   &usf->ub_array[b].si,
			                                   ua_array[i].image,
			                                   ua_array[i].binding);
		}
	}
}

void vkk_renderer_bindGraphicsPipeline(vkk_renderer_t* self,
                                       vkk_graphicsPipeline_t* gp)
{
	ASSERT(self);
	ASSERT(self->mode == VKK_RENDERER_MODE_DRAW);
	ASSERT(self == gp->renderer);

	VkCommandBuffer cb = vkk_renderer_commandBuffer(self);

	vkCmdBindPipeline(cb,
	                  VK_PIPELINE_BIND_POINT_GRAPHICS,
	                  gp->pipeline);
	self->gp = gp;

	// update timestamp
	gp->ts = vkk_renderer_tsCurrent(self);
}

void vkk_renderer_bindUniformSets(vkk_renderer_t* self,
                                  uint32_t us_count,
                                  vkk_uniformSet_t** us_array)
{
	ASSERT(self);
	ASSERT(self->gp);
	ASSERT(us_array);
	ASSERT(self->mode == VKK_RENDERER_MODE_DRAW);

	double ts = vkk_renderer_tsCurrent(self);

	if(us_count > VKK_ENGINE_MAX_USF_COUNT)
	{
		LOGE("invalid us_count=%u", us_count);
		return;
	}

	// fill descriptor set array
	int             i;
	uint32_t        idx;
	VkDescriptorSet ds[VKK_ENGINE_MAX_USF_COUNT];
	uint32_t        swapchain_frame;
	swapchain_frame = vkk_renderer_swapchainFrame(self);

	for(i = 0; i < us_count; ++i)
	{
		idx   = (us_array[i]->usf->update == VKK_UPDATE_MODE_ASYNCHRONOUS) ?
		        swapchain_frame : 0;
		ds[i] = us_array[i]->ds_array[idx];

		// update timestamps
		if(self->type == VKK_RENDERER_TYPE_DEFAULT)
		{
			int j;
			for(j = 0; j < us_array[i]->ua_count; ++j)
			{
				vkk_uniformAttachment_t* ua;
				ua = &(us_array[i]->ua_array[j]);
				if(ua->type == VKK_UNIFORM_TYPE_BUFFER)
				{
					ua->buffer->ts = ts;
				}
				else if(ua->type == VKK_UNIFORM_TYPE_IMAGE)
				{
					ua->image->ts = ts;
				}
			}
			us_array[i]->ts = ts;
		}
	}

	uint32_t first = us_array[0]->set;

	VkCommandBuffer cb = vkk_renderer_commandBuffer(self);
	vkCmdBindDescriptorSets(cb,
	                        VK_PIPELINE_BIND_POINT_GRAPHICS,
	                        self->gp->pl->pl, first, us_count, ds,
	                        0, NULL);
}

void vkk_renderer_clearDepth(vkk_renderer_t* self)
{
	ASSERT(self);
	ASSERT(self->mode == VKK_RENDERER_MODE_DRAW);

	uint32_t width;
	uint32_t height;
	vkk_renderer_surfaceSize(self, &width, &height);
	VkClearRect rect =
	{
		.rect =
		{
			.offset =
			{
				.x = 0,
				.y = 0
			},
			.extent =
			{
				.width  = width,
				.height = height
			}
		},
		.baseArrayLayer = 0,
		.layerCount     = 1
	};

	VkClearAttachment ca =
	{
		.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT |
		              VK_IMAGE_ASPECT_STENCIL_BIT,
		.colorAttachment = 0,
		.clearValue =
		{
			.depthStencil =
			{
				.depth   = 1.0f,
				.stencil = 0
			}
		}
	};

	VkCommandBuffer cb = vkk_renderer_commandBuffer(self);
	vkCmdClearAttachments(cb, 1, &ca, 1, &rect);
}

void vkk_renderer_viewport(vkk_renderer_t* self,
                           float x, float y,
                           float width, float height)
{
	ASSERT(self);
	ASSERT(self->mode == VKK_RENDERER_MODE_DRAW);

	VkViewport viewport =
	{
		.x        = x,
		.y        = y,
		.width    = width,
		.height   = height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	VkCommandBuffer cb = vkk_renderer_commandBuffer(self);
	vkCmdSetViewport(cb, 0, 1, &viewport);
}

void vkk_renderer_scissor(vkk_renderer_t* self,
                          uint32_t x, uint32_t y,
                          uint32_t width, uint32_t height)
{
	ASSERT(self);
	ASSERT(self->mode == VKK_RENDERER_MODE_DRAW);

	VkRect2D scissor =
	{
		.offset =
		{
			.x = x,
			.y = y
		},
		.extent =
		{
			.width  = width,
			.height = height
		}
	};

	VkCommandBuffer cb = vkk_renderer_commandBuffer(self);
	vkCmdSetScissor(cb, 0, 1, &scissor);
}


void vkk_renderer_draw(vkk_renderer_t* self,
                       uint32_t vertex_count,
                       uint32_t vertex_buffer_count,
                       vkk_buffer_t** vertex_buffers)
{
	ASSERT(self);
	ASSERT(vertex_buffer_count <= 16);
	ASSERT(self->mode == VKK_RENDERER_MODE_DRAW);

	VkDeviceSize vb_offsets[16] =
	{
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0
	};

	// fill in the vertex buffers
	int i = 0;
	int idx;
	VkBuffer vb_buffers[16];
	for(i = 0; i < vertex_buffer_count; ++i)
	{
		idx = vertex_buffers[i]->vbib_index;
		vb_buffers[i] = vertex_buffers[i]->buffer[idx];
	}

	VkCommandBuffer cb = vkk_renderer_commandBuffer(self);
	vkCmdBindVertexBuffers(cb, 0, vertex_buffer_count,
	                       vb_buffers, vb_offsets);
	vkCmdDraw(cb, vertex_count, 1, 0, 0);

	// update timestamps
	double ts = vkk_renderer_tsCurrent(self);
	if(ts != 0.0)
	{
		for(i = 0; i < vertex_buffer_count; ++i)
		{
			vertex_buffers[i]->ts = ts;
		}
	}
}

void vkk_renderer_drawIndexed(vkk_renderer_t* self,
                              uint32_t index_count,
                              uint32_t vertex_buffer_count,
                              vkk_indexType_e index_type,
                              vkk_buffer_t* index_buffer,
                              vkk_buffer_t** vertex_buffers)
{
	ASSERT(self);
	ASSERT(vertex_buffer_count <= 16);
	ASSERT(self->mode == VKK_RENDERER_MODE_DRAW);

	VkDeviceSize vb_offsets[16] =
	{
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0
	};

	// fill in the vertex buffers
	int i = 0;
	int idx;
	VkBuffer vb_buffers[16];
	for(i = 0; i < vertex_buffer_count; ++i)
	{
		idx = vertex_buffers[i]->vbib_index;
		vb_buffers[i] = vertex_buffers[i]->buffer[idx];
	}

	VkIndexType it_map[VKK_INDEX_TYPE_COUNT] =
	{
		VK_INDEX_TYPE_UINT16,
		VK_INDEX_TYPE_UINT32
	};

	idx = index_buffer->vbib_index;
	VkBuffer ib_buffer = index_buffer->buffer[idx];

	VkCommandBuffer cb = vkk_renderer_commandBuffer(self);
	vkCmdBindIndexBuffer(cb, ib_buffer, 0,
	                     it_map[index_type]);
	vkCmdBindVertexBuffers(cb, 0, vertex_buffer_count,
	                       vb_buffers, vb_offsets);
	vkCmdDrawIndexed(cb, index_count, 1, 0, 0, 0);

	// update timestamps
	double ts = vkk_renderer_tsCurrent(self);
	if(ts != 0.0)
	{
		for(i = 0; i < vertex_buffer_count; ++i)
		{
			vertex_buffers[i]->ts = ts;
		}
		index_buffer->ts = ts;
	}
}

void vkk_renderer_execute(vkk_renderer_t* self,
                          uint32_t secondary_count,
                          vkk_renderer_t** secondary_array)
{
	ASSERT(self);
	ASSERT(secondary_array);
	ASSERT(self->mode == VKK_RENDERER_MODE_EXECUTE);

	VkCommandBuffer cb_array[secondary_count];

	uint32_t i;
	for(i = 0; i < secondary_count; ++i)
	{
		vkk_renderer_t* sec = secondary_array[i];
		cb_array[i] = vkk_renderer_commandBuffer(sec);
	}

	VkCommandBuffer cb = vkk_renderer_commandBuffer(self);
	vkCmdExecuteCommands(cb, secondary_count, cb_array);
}
