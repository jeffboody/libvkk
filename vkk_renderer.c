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
#include "vkk_engine.h"
#include "vkk_renderer.h"

/***********************************************************
* public                                                   *
***********************************************************/

int vkk_renderer_beginDefault(vkk_renderer_t* self,
                              float* clear_color)
{
	assert(self);
	assert(clear_color);

	vkk_engine_t* engine = self->engine;

	// check for the default renderer and valid callback
	if((self != engine->renderer) ||
	   (self->beginDefaultFn == NULL))
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

	return (*self->beginDefaultFn)(self, clear_color);
}

int vkk_renderer_beginOffscreen(vkk_renderer_t* self,
                                vkk_image_t* image,
                                float* clear_color)
{
	assert(self);
	assert(image);
	assert(clear_color);

	vkk_engine_t* engine = self->engine;

	// check for an offscreen renderer and valid callback
	if((self == engine->renderer) ||
	   (self->beginOffscreenFn == NULL))
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

	return (*self->beginOffscreenFn)(self, image,
	                                 clear_color);
}

void vkk_renderer_end(vkk_renderer_t* self)
{
	assert(self);
	assert(self->endFn);

	(*self->endFn)(self);
}

void vkk_renderer_surfaceSize(vkk_renderer_t* self,
                              uint32_t* _width,
                              uint32_t* _height)
{
	assert(self);
	assert(self->surfaceSizeFn);

	(*self->surfaceSizeFn)(self, _width, _height);
}

void vkk_renderer_updateBuffer(vkk_renderer_t* self,
                               vkk_buffer_t* buffer,
                               const void* buf)
{
	assert(self);

	vkk_engine_t* engine = self->engine;

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

	uint32_t swapchain_frame;
	swapchain_frame = (*self->swapchainFrameFn)(self);

	uint32_t idx;
	idx = (buffer->update == VKK_UPDATE_MODE_DEFAULT) ?
	      swapchain_frame : 0;

	void* data;
	if(vkMapMemory(engine->device,
	               buffer->memory[idx],
	               0, buffer->size, 0,
	               &data) == VK_SUCCESS)
	{
		memcpy(data, buf, buffer->size);
		vkUnmapMemory(engine->device,
		              buffer->memory[idx]);
	}
	else
	{
		LOGW("vkMapMemory failed");
	}
}

void vkk_renderer_bindGraphicsPipeline(vkk_renderer_t* self,
                                       vkk_graphicsPipeline_t* gp)
{
	assert(self);

	vkk_engine_t* engine = self->engine;

	VkCommandBuffer cb = (*self->commandBufferFn)(self);
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
	assert(us_count > 0);
	assert(us_array);

	vkk_engine_t* engine = self->engine;

	double ts = 0.0;
	if(self == engine->renderer)
	{
		ts = vkk_defaultRenderer_tsCurrent(self);
	}

	// allow for a constant and dynamic uniform set
	int             i;
	uint32_t        idx;
	VkDescriptorSet ds[2];
	uint32_t        swapchain_frame;
	swapchain_frame = (*self->swapchainFrameFn)(self);
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

	VkCommandBuffer cb = (*self->commandBufferFn)(self);
	vkCmdBindDescriptorSets(cb,
	                        VK_PIPELINE_BIND_POINT_GRAPHICS,
	                        pl->pl, first, us_count, ds,
	                        0, NULL);
}

void vkk_renderer_clearDepth(vkk_renderer_t* self)
{
	assert(self);

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

	VkCommandBuffer cb = (*self->commandBufferFn)(self);
	vkCmdClearAttachments(cb, 1, &ca, 1, &rect);
}

void vkk_renderer_viewport(vkk_renderer_t* self,
                           float x, float y,
                           float width, float height)
{
	assert(self);

	VkViewport viewport =
	{
		.x        = x,
		.y        = y,
		.width    = width,
		.height   = height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	VkCommandBuffer cb = (*self->commandBufferFn)(self);
	vkCmdSetViewport(cb, 0, 1, &viewport);
}

void vkk_renderer_scissor(vkk_renderer_t* self,
                          uint32_t x, uint32_t y,
                          uint32_t width, uint32_t height)
{
	assert(self);

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

	VkCommandBuffer cb = (*self->commandBufferFn)(self);
	vkCmdSetScissor(cb, 0, 1, &scissor);
}


void vkk_renderer_draw(vkk_renderer_t* self,
                       uint32_t vertex_count,
                       uint32_t vertex_buffer_count,
                       vkk_buffer_t** vertex_buffers)
{
	assert(self);

	vkk_engine_t* engine = self->engine;

	VkDeviceSize vb_offsets[16] =
	{
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0
	};
	VkBuffer vb_buffers[16];
	uint32_t swapchain_frame;
	swapchain_frame = (*self->swapchainFrameFn)(self);

	// fill in the vertex buffers
	int i = 0;
	int idx;
	for(i = 0; i < vertex_buffer_count; ++i)
	{
		idx = (vertex_buffers[i]->update == VKK_UPDATE_MODE_DEFAULT) ?
		      swapchain_frame : 0;
		vb_buffers[i] = vertex_buffers[i]->buffer[idx];
	}

	VkCommandBuffer cb = (*self->commandBufferFn)(self);
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

	vkk_engine_t* engine = self->engine;

	VkDeviceSize vb_offsets[16] =
	{
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0
	};
	VkBuffer vb_buffers[16];
	uint32_t swapchain_frame;
	swapchain_frame = (*self->swapchainFrameFn)(self);

	// fill in the vertex buffers
	int i = 0;
	int idx;
	for(i = 0; i < vertex_buffer_count; ++i)
	{
		idx = (vertex_buffers[i]->update == VKK_UPDATE_MODE_DEFAULT) ?
		      swapchain_frame : 0;
		vb_buffers[i] = vertex_buffers[i]->buffer[idx];
	}

	VkCommandBuffer cb = (*self->commandBufferFn)(self);
	VkIndexType it_map[VKK_INDEX_TYPE_COUNT] =
	{
		VK_INDEX_TYPE_UINT16,
		VK_INDEX_TYPE_UINT32
	};

	idx = (index_buffer->update == VKK_UPDATE_MODE_DEFAULT) ?
	      swapchain_frame : 0;
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
                       vkk_engine_t* engine,
                       vkk_renderer_beginDefaultFn beginDefaultFn,
                       vkk_renderer_beginOffscreenFn beginOffscreenFn,
                       vkk_renderer_endFn endFn,
                       vkk_renderer_surfaceSizeFn surfaceSizeFn,
                       vkk_renderer_renderPassFn renderPassFn,
                       vkk_renderer_commandBufferFn commandBufferFn,
                       vkk_renderer_swapchainFrameFn swapchainFrameFn)
{
	assert(self);
	assert(engine);
	assert(beginDefaultFn || beginOffscreenFn);
	assert(endFn);
	assert(surfaceSizeFn);
	assert(renderPassFn);
	assert(commandBufferFn);
	assert(swapchainFrameFn);

	self->engine           = engine;
	self->beginDefaultFn   = beginDefaultFn;
	self->beginOffscreenFn = beginOffscreenFn;
	self->endFn            = endFn;
	self->surfaceSizeFn    = surfaceSizeFn;
	self->renderPassFn     = renderPassFn;
	self->commandBufferFn  = commandBufferFn;
	self->swapchainFrameFn = swapchainFrameFn;
}

VkRenderPass vkk_renderer_renderPass(vkk_renderer_t* self)
{
	assert(self);
	assert(self->renderPassFn);

	return (*self->renderPassFn)(self);
}

VkCommandBuffer vkk_renderer_commandBuffer(vkk_renderer_t* self)
{
	assert(self);
	assert(self->commandBufferFn);

	return (*self->commandBufferFn)(self);
}

uint32_t vkk_renderer_swapchainFrame(vkk_renderer_t* self)
{
	assert(self);
	assert(self->swapchainFrameFn);

	return (*self->swapchainFrameFn)(self);
}
