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

#ifndef vkk_H
#define vkk_H

#include <stdint.h>

/*
 * constants
 */

#define VKK_BLEND_MODE_DISABLED     0
#define VKK_BLEND_MODE_TRANSPARENCY 1
#define VKK_BLEND_MODE_COUNT        2

#define VKK_BUFFER_USAGE_UNIFORM 0
#define VKK_BUFFER_USAGE_VERTEX  1
#define VKK_BUFFER_USAGE_INDEX   2
#define VKK_BUFFER_USAGE_COUNT   3

#define VKK_IMAGE_FORMAT_RGBA8888 0
#define VKK_IMAGE_FORMAT_RGBA4444 1
#define VKK_IMAGE_FORMAT_RGB888   2
#define VKK_IMAGE_FORMAT_RGB565   3
#define VKK_IMAGE_FORMAT_RG88     4
#define VKK_IMAGE_FORMAT_R8       5
#define VKK_IMAGE_FORMAT_DEPTH    6
#define VKK_IMAGE_FORMAT_COUNT    7

#define VKK_IMAGE_CAPS_TEXTURE         1
#define VKK_IMAGE_CAPS_MIPMAP          2
#define VKK_IMAGE_CAPS_FILTER_LINEAR   4
#define VKK_IMAGE_CAPS_OFFSCREEN       8
#define VKK_IMAGE_CAPS_OFFSCREEN_BLEND 16

#define VKK_INDEX_TYPE_USHORT 0
#define VKK_INDEX_TYPE_UINT   1
#define VKK_INDEX_TYPE_COUNT  2

#define VKK_PRIMITIVE_TRIANGLE_LIST  0
#define VKK_PRIMITIVE_TRIANGLE_STRIP 1
#define VKK_PRIMITIVE_TRIANGLE_FAN   2
#define VKK_PRIMITIVE_TRIANGLE_COUNT 3

#define VKK_SAMPLER_FILTER_NEAREST 0
#define VKK_SAMPLER_FILTER_LINEAR  1
#define VKK_SAMPLER_FILTER_COUNT   2

#define VKK_SAMPLER_MIPMAP_MODE_NEAREST 0
#define VKK_SAMPLER_MIPMAP_MODE_LINEAR  1
#define VKK_SAMPLER_MIPMAP_MODE_COUNT   2

#define VKK_STAGE_DEPTH 0
#define VKK_STAGE_VS    1
#define VKK_STAGE_FS    2
#define VKK_STAGE_VSFS  3
#define VKK_STAGE_COUNT 4

#define VKK_UNIFORM_TYPE_BUFFER  0
#define VKK_UNIFORM_TYPE_SAMPLER 1
#define VKK_UNIFORM_TYPE_COUNT   2

#define VKK_UPDATE_MODE_STATIC    0
#define VKK_UPDATE_MODE_DEFAULT   1
#define VKK_UPDATE_MODE_OFFSCREEN 2
#define VKK_UPDATE_MODE_COUNT     3

#define VKK_VERTEX_FORMAT_FLOAT   0
#define VKK_VERTEX_FORMAT_SINT    1
#define VKK_VERTEX_FORMAT_SSHORT  2
#define VKK_VERTEX_FORMAT_UINT    3
#define VKK_VERTEX_FORMAT_USHORT  4
#define VKK_VERTEX_FORMAT_COUNT   5

/*
 * opaque objects
 */

typedef struct vkk_buffer_s            vkk_buffer_t;
typedef struct vkk_engine_s            vkk_engine_t;
typedef struct vkk_graphicsPipeline_s  vkk_graphicsPipeline_t;
typedef struct vkk_image_s             vkk_image_t;
typedef struct vkk_pipelineLayout_s    vkk_pipelineLayout_t;
typedef struct vkk_sampler_s           vkk_sampler_t;
typedef struct vkk_renderer_s          vkk_renderer_t;
typedef struct vkk_uniformSet_s        vkk_uniformSet_t;
typedef struct vkk_uniformSetFactory_s vkk_uniformSetFactory_t;

/*
 * parameter structures
 */

typedef struct
{
	uint32_t binding;
	int      type;

	union
	{
		vkk_buffer_t* buffer;
		vkk_image_t*  image;
	};
} vkk_uniformAttachment_t;

typedef struct
{
	uint32_t       binding;
	int            type;
	int            stage;
	vkk_sampler_t* sampler;
} vkk_uniformBinding_t;

typedef struct
{
	uint32_t location;
	uint32_t components;
	int      format;
} vkk_vertexBufferInfo_t;

typedef struct
{
	vkk_renderer_t*         renderer;
	vkk_pipelineLayout_t*   pl;
	const char*             vs;
	const char*             fs;
	uint32_t                vb_count;
	vkk_vertexBufferInfo_t* vbi;
	int                     primitive;
	int                     primitive_restart;
	int                     cull_back;
	int                     depth_test;
	int                     depth_write;
	int                     blend_mode;
} vkk_graphicsPipelineInfo_t;

/*
 * engine new/delete API
 *
 *  1) objects may be created from any thread using the
 *     engine handle
 *  2) objects may be deleted from any thread as long as
 *     any renderer which used the object has already called
 *     vkk_renderer_end().
 *  3) an object cannot be used by any thread once deleted
 *  4) image, sampler, uniformSetFactory and pipelineLayout
 *     may be shared between renderers
 *  5) images may only be bound to a single offscreen
 *     renderer at once
 *  6) images should not be bound to an offscreen renderer
 *     between begin/end commands
 *  7) regarding buffers and uniform sets
 *     a) sharing between renderers is only allowed when
 *        update is set to STATIC
 *     b) may only be updated by default renderer when
 *        update is set to DEFAULT
 *     c) may only be updated by offscreen renderer when
 *        update is set to OFFSCREEN
 *     d) when update is set to DEFAULT and the buffer
 *        usage is uniform then then the buffer must be
 *        updated once and only once per frame (to keep all
 *        bindings in a uniform set consistent)
 *     e) when update is set to DEFAULT and the buffer
 *        usage is vertex or index then the buffer may be
 *        updated zero or one times
 *  9) graphics pipelines are NOT shared between renderers
 * 10) CPU and GPU synchronization is handled automatically
 *     by the engine with the exception of the shutdown
 *     function
 * 11) call shutdown from the main thread prior to deleting
 *     the engine thus ensuring GPU rendering completes and
 *     worker threads are no longer blocked waiting for GPU
 *     events
 */

vkk_engine_t*            vkk_engine_new(void* app,
                                        const char* app_name,
                                        uint32_t app_version,
                                        const char* resource,
                                        const char* cache);
void                     vkk_engine_delete(vkk_engine_t** _self);
void                     vkk_engine_shutdown(vkk_engine_t* self);
vkk_renderer_t*          vkk_engine_newRenderer(vkk_engine_t* self,
                                                uint32_t width,
                                                uint32_t height,
                                                int format);
void                     vkk_engine_deleteRenderer(vkk_engine_t* self,
                                                   vkk_renderer_t** _renderer);
vkk_buffer_t*            vkk_engine_newBuffer(vkk_engine_t* self,
                                              int update,
                                              int usage,
                                              size_t size,
                                              const void* buf);
void                     vkk_engine_deleteBuffer(vkk_engine_t* self,
                                                 vkk_buffer_t** _buffer);
vkk_image_t*             vkk_engine_newImage(vkk_engine_t* self,
                                             uint32_t width,
                                             uint32_t height,
                                             int format,
                                             int mipmap,
                                             int stage,
                                             const void* pixels);
void                     vkk_engine_deleteImage(vkk_engine_t* self,
                                                vkk_image_t** _image);
vkk_sampler_t*           vkk_engine_newSampler(vkk_engine_t* self,
                                               int min_filter,
                                               int mag_filter,
                                               int mipmap_mode);
void                     vkk_engine_deleteSampler(vkk_engine_t* self,
                                                  vkk_sampler_t** _sampler);
vkk_uniformSetFactory_t* vkk_engine_newUniformSetFactory(vkk_engine_t* self,
                                                         int update,
                                                         uint32_t ub_count,
                                                         vkk_uniformBinding_t* ub_array);
void                     vkk_engine_deleteUniformSetFactory(vkk_engine_t* self,
                                                            vkk_uniformSetFactory_t** _usf);
vkk_uniformSet_t*        vkk_engine_newUniformSet(vkk_engine_t* self,
                                                  uint32_t set,
                                                  uint32_t ua_count,
                                                  vkk_uniformAttachment_t* ua_array,
                                                  vkk_uniformSetFactory_t* usf);
void                     vkk_engine_deleteUniformSet(vkk_engine_t* self,
                                                     vkk_uniformSet_t** _us);
vkk_pipelineLayout_t*    vkk_engine_newPipelineLayout(vkk_engine_t* self,
                                                      uint32_t usf_count,
                                                      vkk_uniformSetFactory_t** usf_array);
void                     vkk_engine_deletePipelineLayout(vkk_engine_t* self,
                                                         vkk_pipelineLayout_t** _pl);
vkk_graphicsPipeline_t*  vkk_engine_newGraphicsPipeline(vkk_engine_t* self,
                                                        vkk_graphicsPipelineInfo_t* gpi);
void                     vkk_engine_deleteGraphicsPipeline(vkk_engine_t* self,
                                                           vkk_graphicsPipeline_t** _gp);

/*
 * query API
 */


uint32_t VKK_MAKE_VERSION(uint32_t major, uint32_t minor,
                          uint32_t patch);
size_t   vkk_buffer_size(vkk_buffer_t* self);
int      vkk_engine_imageCaps(vkk_engine_t* self,
                              int format);
uint32_t vkk_engine_version(vkk_engine_t* self);
int      vkk_image_format(vkk_image_t* self);
size_t   vkk_image_size(vkk_image_t* self,
                        uint32_t* _width, uint32_t* height);

/*
 * default renderer API
 *
 * 1) call the default renderer from the main thread
 * 2) the default renderer is created and destroyed
 *    automatically by the engine
 * 3) the resize event triggered by native window system
 *    causes the default renderer surfaceSize to be updated
 */

int             vkk_engine_resize(vkk_engine_t* self);
vkk_renderer_t* vkk_engine_renderer(vkk_engine_t* self);

/*
 * rendering API
 *
 * 1) call vkk_renderer commands between
 *    begin/end on a single thread
 * 2) if begin succeeds then you must also call end
 * 3) the default renderer completes asynchronously
 *    (e.g. end is non-blocking) and should be called from
 *    the main thread
 * 4) the offscreen renderer completes synchronously
 *    (e.g. end is blocking) and should be called from a
 *    worker thread
 * 5) the depth buffer, viewport and scissor are initialized
 *    automatically by begin
 */

int  vkk_renderer_beginDefault(vkk_renderer_t* self,
                               float* clear_color);
int  vkk_renderer_beginOffscreen(vkk_renderer_t* self,
                                 vkk_image_t* image,
                                 float* clear_color);
void vkk_renderer_end(vkk_renderer_t* self);
void vkk_renderer_surfaceSize(vkk_renderer_t* self,
                              uint32_t* _width,
                              uint32_t* _height);
void vkk_renderer_updateBuffer(vkk_renderer_t* self,
                               vkk_buffer_t* buffer,
                               size_t size,
                               const void* buf);
void vkk_renderer_bindGraphicsPipeline(vkk_renderer_t* self,
                                       vkk_graphicsPipeline_t* gp);
void vkk_renderer_bindUniformSets(vkk_renderer_t* self,
                                  vkk_pipelineLayout_t* pl,
                                  uint32_t us_count,
                                  vkk_uniformSet_t** us_array);
void vkk_renderer_clearDepth(vkk_renderer_t* self);
void vkk_renderer_viewport(vkk_renderer_t* self,
                           float x,
                           float y,
                           float width,
                           float height);
void vkk_renderer_scissor(vkk_renderer_t* self,
                          uint32_t x,
                          uint32_t y,
                          uint32_t width,
                          uint32_t height);
void vkk_renderer_draw(vkk_renderer_t* self,
                       uint32_t vertex_count,
                       uint32_t vertex_buffer_count,
                       vkk_buffer_t** vertex_buffers);
void vkk_renderer_drawIndexed(vkk_renderer_t* self,
                              uint32_t vertex_count,
                              uint32_t vertex_buffer_count,
                              int index_type,
                              vkk_buffer_t* index_buffer,
                              vkk_buffer_t** vertex_buffers);

#endif
