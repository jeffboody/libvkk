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

#include "vkk_engine.h"
#include "vkk_renderer.h"

/***********************************************************
* public                                                   *
***********************************************************/

int vkk_renderer_begin(vkk_renderer_t* self,
                       float* clear_color)
{
	assert(self);
	assert(self->beginFn);

	vkk_engine_t* engine = self->engine;

	vkk_engine_rendererLock(engine);
	if(engine->shutdown)
	{
		vkk_engine_rendererUnlock(engine);
		return 0;
	}
	vkk_engine_rendererUnlock(engine);

	return (*self->beginFn)(self, clear_color);
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
	assert(self->updateBufferFn);

	(*self->updateBufferFn)(self, buffer, buf);
}

void vkk_renderer_bindGraphicsPipeline(vkk_renderer_t* self,
                                       vkk_graphicsPipeline_t* gp)
{
	assert(self);
	assert(self->bindGraphicsPipelineFn);

	(*self->bindGraphicsPipelineFn)(self, gp);
}

void vkk_renderer_bindUniformSets(vkk_renderer_t* self,
                                  vkk_pipelineLayout_t* pl,
                                  uint32_t us_count,
                                  vkk_uniformSet_t** us_array)
{
	assert(self);
	assert(self->bindUniformSetsFn);

	(*self->bindUniformSetsFn)(self, pl, us_count, us_array);
}

void vkk_renderer_clearDepth(vkk_renderer_t* self)
{
	assert(self);
	assert(self->clearDepthFn);

	(*self->clearDepthFn)(self);
}

void vkk_renderer_viewport(vkk_renderer_t* self,
                           float x, float y,
                           float width, float height)
{
	assert(self);
	assert(self->viewportFn);

	(*self->viewportFn)(self, x, y, width, height);
}

void vkk_renderer_scissor(vkk_renderer_t* self,
                          uint32_t x, uint32_t y,
                          uint32_t width, uint32_t height)
{
	assert(self);
	assert(self->scissorFn);

	(*self->scissorFn)(self, x, y, width, height);
}


void vkk_renderer_draw(vkk_renderer_t* self,
                       uint32_t vertex_count,
                       uint32_t vertex_buffer_count,
                       vkk_buffer_t** vertex_buffers)
{
	assert(self);
	assert(self->drawFn);

	(*self->drawFn)(self, vertex_count, vertex_buffer_count,
	                vertex_buffers);
}

void vkk_renderer_drawIndexed(vkk_renderer_t* self,
                              uint32_t vertex_count,
                              uint32_t vertex_buffer_count,
                              int index_type,
                              vkk_buffer_t* index_buffer,
                              vkk_buffer_t** vertex_buffers)
{
	assert(self);
	assert(self->drawIndexedFn);

	(*self->drawIndexedFn)(self,
	                       vertex_count, vertex_buffer_count,
	                       index_type, index_buffer,
	                       vertex_buffers);
}

void vkk_renderer_init(vkk_renderer_t* self,
                       vkk_engine_t* engine,
                       vkk_renderer_beginFn beginFn,
                       vkk_renderer_endFn endFn,
                       vkk_renderer_surfaceSizeFn surfaceSizeFn,
                       vkk_renderer_updateBufferFn updateBufferFn,
                       vkk_renderer_bindGraphicsPipelineFn bindGraphicsPipelineFn,
                       vkk_renderer_bindUniformSetsFn bindUniformSetsFn,
                       vkk_renderer_clearDepthFn clearDepthFn,
                       vkk_renderer_viewportFn viewportFn,
                       vkk_renderer_scissorFn scissorFn,
                       vkk_renderer_drawFn drawFn,
                       vkk_renderer_drawIndexedFn drawIndexedFn,
                       vkk_renderer_renderPassFn renderPassFn,
                       vkk_renderer_swapchainImageCountFn swapchainImageCountFn,
                       vkk_renderer_currentTimestampFn currentTimestampFn,
                       vkk_renderer_expiredTimestampLockedFn expiredTimestampLockedFn)
{
	assert(self);
	assert(engine);
	assert(beginFn);
	assert(endFn);
	assert(surfaceSizeFn);
	assert(updateBufferFn);
	assert(bindGraphicsPipelineFn);
	assert(bindUniformSetsFn);
	assert(clearDepthFn);
	assert(viewportFn);
	assert(scissorFn);
	assert(drawFn);
	assert(drawIndexedFn);
	assert(renderPassFn);
	assert(swapchainImageCountFn);
	assert(currentTimestampFn);
	assert(expiredTimestampLockedFn);

	self->engine                   = engine;
	self->beginFn                  = beginFn;
	self->endFn                    = endFn;
	self->surfaceSizeFn            = surfaceSizeFn;
	self->updateBufferFn           = updateBufferFn;
	self->bindGraphicsPipelineFn   = bindGraphicsPipelineFn;
	self->bindUniformSetsFn        = bindUniformSetsFn;
	self->clearDepthFn             = clearDepthFn;
	self->viewportFn               = viewportFn;
	self->scissorFn                = scissorFn;
	self->drawFn                   = drawFn;
	self->drawIndexedFn            = drawIndexedFn;
	self->renderPassFn             = renderPassFn;
	self->swapchainImageCountFn    = swapchainImageCountFn;
	self->currentTimestampFn       = currentTimestampFn;
	self->expiredTimestampLockedFn = expiredTimestampLockedFn;
}

VkRenderPass vkk_renderer_renderPass(vkk_renderer_t* self)
{
	assert(self);
	assert(self->renderPassFn);

	return (*self->renderPassFn)(self);
}

uint32_t
vkk_renderer_swapchainImageCount(vkk_renderer_t* self)
{
	assert(self);
	assert(self->swapchainImageCountFn);

	return (*self->swapchainImageCountFn)(self);
}

double
vkk_renderer_currentTimestamp(vkk_renderer_t* self)
{
	assert(self);

	return (*self->currentTimestampFn)(self);
}

double
vkk_renderer_expiredTimestampLocked(vkk_renderer_t* self)
{
	assert(self);

	return (*self->expiredTimestampLockedFn)(self);
}
