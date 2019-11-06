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
#include "vkk_defaultRenderer.h"
#include "vkk_offscreenRenderer.h"
#include "vkk_engine.h"
#include "vkk_renderer.h"

/***********************************************************
* private                                                  *
***********************************************************/

static void
vkk_fillUniformAttachmentArray(vkk_uniformAttachment_t* dst,
                               uint32_t src_ua_count,
                               vkk_uniformAttachment_t* src,
                               vkk_uniformSetFactory_t* usf)
{
	assert(dst);
	assert(src);
	assert(usf);

	// dst binding/type already filled in
	// validate and fill buffer/image union
	uint32_t i;
	for(i = 0; i < src_ua_count; ++i)
	{
		uint32_t b = src[i].binding;
		assert(b < usf->ub_count);
		assert(b == dst[b].binding);
		assert((src[i].type == VKK_UNIFORM_TYPE_BUFFER_REF) ||
		       (src[i].type == VKK_UNIFORM_TYPE_SAMPLER_REF));
		assert(src[i].type == dst[b].type);

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
	assert(self);
	assert(us);
	assert(buffer);

	vkk_engine_t* engine = self->engine;

	uint32_t idx;
	idx = (buffer->update == VKK_UPDATE_MODE_DEFAULT) ?
	      swapchain_frame : 0;
	VkDescriptorBufferInfo db_info =
	{
		.buffer  = buffer->buffer[idx],
		.offset  = 0,
		.range   = buffer->size
	};

	idx = (us->usf->update == VKK_UPDATE_MODE_DEFAULT) ?
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
vkk_renderer_updateUniformSamplerRef(vkk_renderer_t* self,
                                     vkk_uniformSet_t* us,
                                     uint32_t swapchain_frame,
                                     vkk_sampler_t* sampler,
                                     vkk_image_t* image,
                                     uint32_t binding)
{
	assert(self);
	assert(us);
	assert(sampler);
	assert(image);

	vkk_engine_t* engine = self->engine;

	VkDescriptorImageInfo di_info =
	{
		.sampler     = sampler->sampler,
		.imageView   = image->image_view,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	uint32_t idx;
	idx = (us->usf->update == VKK_UPDATE_MODE_DEFAULT) ?
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

/***********************************************************
* public                                                   *
***********************************************************/

int vkk_renderer_beginDefault(vkk_renderer_t* self,
                              float* clear_color)
{
	assert(self);
	assert(clear_color);

	vkk_engine_t* engine = self->engine;

	// check for the default renderer
	if(self != engine->renderer)
	{
		LOGE("invalid");
		return 0;
	}

	vkk_engine_rendererLock(engine);
	if(engine->shutdown)
	{
		LOGE("invalid");
		vkk_engine_rendererUnlock(engine);
		return 0;
	}
	vkk_engine_rendererUnlock(engine);

	// flush the destruct workq
	cc_workq_flush(engine->workq_destruct);

	return vkk_defaultRenderer_begin(self, clear_color);
}

int vkk_renderer_beginOffscreen(vkk_renderer_t* self,
                                vkk_image_t* image,
                                float* clear_color)
{
	assert(self);
	assert(image);
	assert(clear_color);

	vkk_engine_t* engine = self->engine;

	// check for an offscreen renderer
	if(self == engine->renderer)
	{
		LOGE("invalid");
		return 0;
	}

	vkk_engine_rendererLock(engine);
	if(engine->shutdown)
	{
		LOGE("invalid");
		vkk_engine_rendererUnlock(engine);
		return 0;
	}
	vkk_engine_rendererUnlock(engine);

	return vkk_offscreenRenderer_begin(self, image,
	                                   clear_color);
}

void vkk_renderer_end(vkk_renderer_t* self)
{
	assert(self);

	vkk_engine_t* engine = self->engine;

	if(self == engine->renderer)
	{
		vkk_defaultRenderer_end(self);
	}
	else
	{
		vkk_offscreenRenderer_end(self);
	}
}

void vkk_renderer_surfaceSize(vkk_renderer_t* self,
                              uint32_t* _width,
                              uint32_t* _height)
{
	assert(self);

	vkk_engine_t* engine = self->engine;

	if(self == engine->renderer)
	{
		vkk_defaultRenderer_surfaceSize(self, _width, _height);
	}
	else
	{
		vkk_offscreenRenderer_surfaceSize(self, _width, _height);
	}
}

void vkk_renderer_updateBuffer(vkk_renderer_t* self,
                               vkk_buffer_t* buffer,
                               size_t size,
                               const void* buf)
{
	assert(self);

	vkk_engine_t* engine = self->engine;

	if(size > buffer->size)
	{
		LOGE("invalid size=%i buffer->size=%i",
		     (int) size, (int) buffer->size);
		return;
	}

	if(buffer->update == VKK_UPDATE_MODE_STATIC)
	{
		LOGW("invalid static update mode");
		return;
	}
	else if((buffer->update == VKK_UPDATE_MODE_DEFAULT) &&
	        (self != engine->renderer))
	{
		LOGW("invalid offscreen renderer");
		return;
	}
	else if((buffer->update == VKK_UPDATE_MODE_OFFSCREEN) &&
	        (self == engine->renderer))
	{
		LOGW("invalid default renderer");
		return;
	}

	uint32_t idx = 0;
	if(buffer->update == VKK_UPDATE_MODE_DEFAULT)
	{
		if((buffer->usage == VKK_BUFFER_USAGE_VERTEX) ||
		   (buffer->usage == VKK_BUFFER_USAGE_INDEX))
		{
			uint32_t count;
			count = (buffer->update == VKK_UPDATE_MODE_DEFAULT) ?
			        vkk_engine_swapchainImageCount(engine) : 1;
			buffer->vbib_index = (buffer->vbib_index + 1)%count;
			idx = buffer->vbib_index;
		}
		else if(self == engine->renderer)
		{
			idx = vkk_defaultRenderer_swapchainFrame(self);
		}
		else
		{
			idx = vkk_offscreenRenderer_swapchainFrame(self);
		}
	}

	void* data;
	if(vkMapMemory(engine->device, buffer->memory[idx],
	               0, size, 0, &data) == VK_SUCCESS)
	{
		memcpy(data, buf, size);
		vkUnmapMemory(engine->device, buffer->memory[idx]);
	}
	else
	{
		LOGW("vkMapMemory failed");
	}
}

void
vkk_renderer_updateUniformSetRefs(vkk_renderer_t* self,
                                  vkk_uniformSet_t* us,
                                  uint32_t ua_count,
                                  vkk_uniformAttachment_t* ua_array)
{
	assert(self);
	assert(us);
	assert(ua_array);

	vkk_engine_t*            engine = self->engine;
	vkk_uniformSetFactory_t* usf    = us->usf;

	vkk_fillUniformAttachmentArray(us->ua_array, ua_count,
	                               ua_array, usf);

	uint32_t swapchain_frame;
	if(self == engine->renderer)
	{
		swapchain_frame = vkk_defaultRenderer_swapchainFrame(self);
	}
	else
	{
		swapchain_frame = vkk_offscreenRenderer_swapchainFrame(self);
	}

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
		else if(ua_array[i].type == VKK_UNIFORM_TYPE_SAMPLER_REF)
		{
			uint32_t b = ua_array[i].binding;
			vkk_renderer_updateUniformSamplerRef(self, us,
			                                     swapchain_frame,
			                                     usf->ub_array[b].sampler,
			                                     ua_array[i].image,
			                                     ua_array[i].binding);
		}
	}
}

void vkk_renderer_bindGraphicsPipeline(vkk_renderer_t* self,
                                       vkk_graphicsPipeline_t* gp)
{
	assert(self);

	vkk_engine_t* engine = self->engine;

	VkCommandBuffer cb;
	if(self == engine->renderer)
	{
		cb = vkk_defaultRenderer_commandBuffer(self);
	}
	else
	{
		cb = vkk_offscreenRenderer_commandBuffer(self);
	}

	vkCmdBindPipeline(cb,
	                  VK_PIPELINE_BIND_POINT_GRAPHICS,
	                  gp->pipeline);

	// update timestamp
	if(self == engine->renderer)
	{
		gp->ts = vkk_defaultRenderer_tsCurrent(self);
	}
}

void vkk_renderer_bindUniformSets(vkk_renderer_t* self,
                                  vkk_pipelineLayout_t* pl,
                                  uint32_t us_count,
                                  vkk_uniformSet_t** us_array)
{
	assert(self);
	assert(pl);
	assert(us_array);

	vkk_engine_t* engine = self->engine;

	double ts = 0.0;
	if(self == engine->renderer)
	{
		ts = vkk_defaultRenderer_tsCurrent(self);
	}

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
	if(self == engine->renderer)
	{
		swapchain_frame = vkk_defaultRenderer_swapchainFrame(self);
	}
	else
	{
		swapchain_frame = vkk_offscreenRenderer_swapchainFrame(self);
	}

	for(i = 0; i < us_count; ++i)
	{
		idx   = (us_array[i]->usf->update == VKK_UPDATE_MODE_DEFAULT) ?
		        swapchain_frame : 0;
		ds[i] = us_array[i]->ds_array[idx];

		// update timestamps
		if(self == engine->renderer)
		{
			int j;
			for(j = 0; j < us_array[i]->ua_count; ++j)
			{
				vkk_uniformAttachment_t* ua;
				vkk_uniformBinding_t*    ub;
				ua = &(us_array[i]->ua_array[j]);
				ub = &(us_array[i]->usf->ub_array[j]);
				if(ua->type == VKK_UNIFORM_TYPE_BUFFER)
				{
					ua->buffer->ts = ts;
				}
				else if(ua->type == VKK_UNIFORM_TYPE_SAMPLER)
				{
					ua->image->ts   = ts;
					ub->sampler->ts = ts;
				}
			}
			us_array[i]->ts = ts;
		}
	}

	uint32_t first = us_array[0]->set;

	VkCommandBuffer cb;
	if(self == engine->renderer)
	{
		cb = vkk_defaultRenderer_commandBuffer(self);
	}
	else
	{
		cb = vkk_offscreenRenderer_commandBuffer(self);
	}

	vkCmdBindDescriptorSets(cb,
	                        VK_PIPELINE_BIND_POINT_GRAPHICS,
	                        pl->pl, first, us_count, ds,
	                        0, NULL);
}

void vkk_renderer_clearDepth(vkk_renderer_t* self)
{
	assert(self);

	vkk_engine_t* engine = self->engine;

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

	VkCommandBuffer cb;
	if(self == engine->renderer)
	{
		cb = vkk_defaultRenderer_commandBuffer(self);
	}
	else
	{
		cb = vkk_offscreenRenderer_commandBuffer(self);
	}

	vkCmdClearAttachments(cb, 1, &ca, 1, &rect);
}

void vkk_renderer_viewport(vkk_renderer_t* self,
                           float x, float y,
                           float width, float height)
{
	assert(self);

	vkk_engine_t* engine = self->engine;

	VkViewport viewport =
	{
		.x        = x,
		.y        = y,
		.width    = width,
		.height   = height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	VkCommandBuffer cb;
	if(self == engine->renderer)
	{
		cb = vkk_defaultRenderer_commandBuffer(self);
	}
	else
	{
		cb = vkk_offscreenRenderer_commandBuffer(self);
	}

	vkCmdSetViewport(cb, 0, 1, &viewport);
}

void vkk_renderer_scissor(vkk_renderer_t* self,
                          uint32_t x, uint32_t y,
                          uint32_t width, uint32_t height)
{
	assert(self);

	vkk_engine_t* engine = self->engine;

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

	VkCommandBuffer cb;
	if(self == engine->renderer)
	{
		cb = vkk_defaultRenderer_commandBuffer(self);
	}
	else
	{
		cb = vkk_offscreenRenderer_commandBuffer(self);
	}

	vkCmdSetScissor(cb, 0, 1, &scissor);
}


void vkk_renderer_draw(vkk_renderer_t* self,
                       uint32_t vertex_count,
                       uint32_t vertex_buffer_count,
                       vkk_buffer_t** vertex_buffers)
{
	assert(self);
	assert(vertex_buffer_count <= 16);

	vkk_engine_t* engine = self->engine;

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

	VkCommandBuffer cb;
	if(self == engine->renderer)
	{
		cb = vkk_defaultRenderer_commandBuffer(self);
	}
	else
	{
		cb = vkk_offscreenRenderer_commandBuffer(self);
	}

	vkCmdBindVertexBuffers(cb, 0, vertex_buffer_count,
	                       vb_buffers, vb_offsets);
	vkCmdDraw(cb, vertex_count, 1, 0, 0);

	// update timestamps
	if(self == engine->renderer)
	{
		double ts = vkk_defaultRenderer_tsCurrent(self);
		for(i = 0; i < vertex_buffer_count; ++i)
		{
			vertex_buffers[i]->ts = ts;
		}
	}
}

void vkk_renderer_drawIndexed(vkk_renderer_t* self,
                              uint32_t vertex_count,
                              uint32_t vertex_buffer_count,
                              int index_type,
                              vkk_buffer_t* index_buffer,
                              vkk_buffer_t** vertex_buffers)
{
	assert(self);
	assert(vertex_buffer_count <= 16);

	vkk_engine_t* engine = self->engine;

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

	VkCommandBuffer cb;
	if(self == engine->renderer)
	{
		cb = vkk_defaultRenderer_commandBuffer(self);
	}
	else
	{
		cb = vkk_offscreenRenderer_commandBuffer(self);
	}

	VkIndexType it_map[VKK_INDEX_TYPE_COUNT] =
	{
		VK_INDEX_TYPE_UINT16,
		VK_INDEX_TYPE_UINT32
	};

	idx = index_buffer->vbib_index;
	VkBuffer ib_buffer = index_buffer->buffer[idx];

	vkCmdBindIndexBuffer(cb, ib_buffer, 0,
	                     it_map[index_type]);
	vkCmdBindVertexBuffers(cb, 0, vertex_buffer_count,
	                       vb_buffers, vb_offsets);
	vkCmdDrawIndexed(cb, vertex_count, 1, 0, 0, 0);

	// update timestamps
	if(self == engine->renderer)
	{
		double ts = vkk_defaultRenderer_tsCurrent(self);
		for(i = 0; i < vertex_buffer_count; ++i)
		{
			vertex_buffers[i]->ts = ts;
		}
		index_buffer->ts = ts;
	}
}

void vkk_renderer_init(vkk_renderer_t* self,
                       vkk_engine_t* engine)
{
	assert(self);
	assert(engine);

	self->engine = engine;
}

VkRenderPass vkk_renderer_renderPass(vkk_renderer_t* self)
{
	assert(self);

	vkk_engine_t* engine = self->engine;

	if(self == engine->renderer)
	{
		return vkk_defaultRenderer_renderPass(self);
	}
	else
	{
		return vkk_offscreenRenderer_renderPass(self);
	}
}

VkCommandBuffer vkk_renderer_commandBuffer(vkk_renderer_t* self)
{
	assert(self);

	vkk_engine_t* engine = self->engine;

	if(self == engine->renderer)
	{
		return vkk_defaultRenderer_commandBuffer(self);
	}
	else
	{
		return vkk_offscreenRenderer_commandBuffer(self);
	}
}

uint32_t vkk_renderer_swapchainFrame(vkk_renderer_t* self)
{
	assert(self);

	vkk_engine_t* engine = self->engine;

	if(self == engine->renderer)
	{
		return vkk_defaultRenderer_swapchainFrame(self);
	}
	else
	{
		return vkk_offscreenRenderer_swapchainFrame(self);
	}
}
