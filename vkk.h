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

#define VKK_UNIFORM_TYPE_BUFFER     0
#define VKK_UNIFORM_TYPE_IMAGE      1
#define VKK_UNIFORM_TYPE_BUFFER_REF 2
#define VKK_UNIFORM_TYPE_IMAGE_REF  3
#define VKK_UNIFORM_TYPE_COUNT      4

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

#define VKK_RENDERER_MODE_PRIMARY   1
#define VKK_RENDERER_MODE_SECONDARY 2

/*
 * opaque objects
 */

typedef struct vkk_buffer_s            vkk_buffer_t;
typedef struct vkk_engine_s            vkk_engine_t;
typedef struct vkk_graphicsPipeline_s  vkk_graphicsPipeline_t;
typedef struct vkk_image_s             vkk_image_t;
typedef struct vkk_pipelineLayout_s    vkk_pipelineLayout_t;
typedef struct vkk_renderer_s          vkk_renderer_t;
typedef struct vkk_uniformSet_s        vkk_uniformSet_t;
typedef struct vkk_uniformSetFactory_s vkk_uniformSetFactory_t;

/*
 * parameter structures
 */

typedef struct
{
	uint32_t major:10;
	uint32_t minor:10;
	uint32_t patch:12;
} vkk_version_t;

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
	int min_filter;
	int mag_filter;
	int mipmap_mode;
} vkk_samplerInfo_t;

typedef struct
{
	uint32_t          binding;
	int               type;
	int               stage;
	vkk_samplerInfo_t si;
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
 * engine API
 */

void            vkk_engine_version(vkk_engine_t* self,
                                   vkk_version_t* version);
void            vkk_engine_platformCmd(vkk_engine_t* self,
                                       int cmd,
                                       const char* msg);
const char*     vkk_engine_resourcePath(vkk_engine_t* self);
void            vkk_engine_meminfo(vkk_engine_t* self,
                                   size_t* _count_chunks,
                                   size_t* _count_slots,
                                   size_t* _size_chunks,
                                   size_t* _size_slots);
int             vkk_engine_imageCaps(vkk_engine_t* self,
                                     int format);
vkk_renderer_t* vkk_engine_defaultRenderer(vkk_engine_t* self);

/*
 * buffer API
 */

vkk_buffer_t* vkk_buffer_new(vkk_engine_t* engine,
                             int update,
                             int usage,
                             size_t size,
                             const void* buf);
void          vkk_buffer_delete(vkk_buffer_t** _self);
size_t        vkk_buffer_size(vkk_buffer_t* self);

/*
 * image API
 */

vkk_image_t* vkk_image_new(vkk_engine_t* engine,
                           uint32_t width,
                           uint32_t height,
                           int format,
                           int mipmap,
                           int stage,
                           const void* pixels);
void         vkk_image_delete(vkk_image_t** _self);
int          vkk_image_format(vkk_image_t* self);
size_t       vkk_image_size(vkk_image_t* self,
                            uint32_t* _width,
                            uint32_t* height);

/*
 * uniform set API
 */

vkk_uniformSet_t* vkk_uniformSet_new(vkk_engine_t* engine,
                                     uint32_t set,
                                     uint32_t ua_count,
                                     vkk_uniformAttachment_t* ua_array,
                                     vkk_uniformSetFactory_t* usf);
void              vkk_uniformSet_delete(vkk_uniformSet_t** _self);

/*
 * uniform set factory API
 */

vkk_uniformSetFactory_t* vkk_uniformSetFactory_new(vkk_engine_t* engine,
                                                   int update,
                                                   uint32_t ub_count,
                                                   vkk_uniformBinding_t* ub_array);
void                     vkk_uniformSetFactory_delete(vkk_uniformSetFactory_t** _self);

/*
 * pipeline layout API
 */

vkk_pipelineLayout_t* vkk_pipelineLayout_new(vkk_engine_t* engine,
                                             uint32_t usf_count,
                                             vkk_uniformSetFactory_t** usf_array);
void                  vkk_pipelineLayout_delete(vkk_pipelineLayout_t** _self);

/*
 * graphics pipeline API
 */

vkk_graphicsPipeline_t* vkk_graphicsPipeline_new(vkk_engine_t* engine,
                                                 vkk_graphicsPipelineInfo_t* gpi);
void                    vkk_graphicsPipeline_delete(vkk_graphicsPipeline_t** _self);

/*
 * rendering API
 */

vkk_renderer_t* vkk_renderer_newOffscreen(vkk_engine_t* engine,
                                          uint32_t width,
                                          uint32_t height,
                                          int format);
vkk_renderer_t* vkk_renderer_newSecondary(vkk_renderer_t* primary);
void            vkk_renderer_delete(vkk_renderer_t** _self);
int             vkk_renderer_beginDefault(vkk_renderer_t* self,
                                          int mode,
                                          float* clear_color);
int             vkk_renderer_beginOffscreen(vkk_renderer_t* self,
                                            int mode,
                                            vkk_image_t* image,
                                            float* clear_color);
int             vkk_renderer_beginSecondary(vkk_renderer_t* self);
void            vkk_renderer_end(vkk_renderer_t* self);
void            vkk_renderer_surfaceSize(vkk_renderer_t* self,
                                         uint32_t* _width,
                                         uint32_t* _height);
void            vkk_renderer_updateBuffer(vkk_renderer_t* self,
                                          vkk_buffer_t* buffer,
                                          size_t size,
                                          const void* buf);
void            vkk_renderer_updateUniformSetRefs(vkk_renderer_t* self,
                                                  vkk_uniformSet_t* us,
                                                  uint32_t ua_count,
                                                  vkk_uniformAttachment_t* ua_array);
void            vkk_renderer_bindGraphicsPipeline(vkk_renderer_t* self,
                                                  vkk_graphicsPipeline_t* gp);
void            vkk_renderer_bindUniformSets(vkk_renderer_t* self,
                                             vkk_pipelineLayout_t* pl,
                                             uint32_t us_count,
                                             vkk_uniformSet_t** us_array);
void            vkk_renderer_clearDepth(vkk_renderer_t* self);
void            vkk_renderer_viewport(vkk_renderer_t* self,
                                      float x,
                                      float y,
                                      float width,
                                      float height);
void            vkk_renderer_scissor(vkk_renderer_t* self,
                                     uint32_t x,
                                     uint32_t y,
                                     uint32_t width,
                                     uint32_t height);
void            vkk_renderer_draw(vkk_renderer_t* self,
                                  uint32_t vertex_count,
                                  uint32_t vertex_buffer_count,
                                  vkk_buffer_t** vertex_buffers);
void            vkk_renderer_drawIndexed(vkk_renderer_t* self,
                                         uint32_t index_count,
                                         uint32_t vertex_buffer_count,
                                         int index_type,
                                         vkk_buffer_t* index_buffer,
                                         vkk_buffer_t** vertex_buffers);
void            vkk_renderer_drawSecondary(vkk_renderer_t* self,
                                           uint32_t secondary_count,
                                           vkk_renderer_t** secondary_array);

#endif
