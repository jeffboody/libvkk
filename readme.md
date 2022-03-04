Introduction
============

The Vulkan Kit (VKK) is a high level graphics API layered
on the low level Vulkan graphics API.

Vulkan is very different from the former OpenGL graphics
standard due to its low level API. The low level API
provides developers unprecedented capability to optimze
algorithms for each apps workload. Optimized algorithms can
greatly impact an apps performance, memory usage, rendering
quality and portability. The low level API also allows
hardware vendors to provide a much simpler driver thats
easier to test resulting in fewer bugs and better
consistency across devices. An important tradeoff of the
low level API is that it requires developers to implement
many algorithms that were historically included by the
OpenGL graphics driver. Example algorithms include memory
management, internal synchronization, data conversion,
resource updates and command pooling. However, the benefits
of the low level API comes at a high development cost for
graphics apps which may not need to optimize every aspect of
rendering.

VKK simplifies development of graphics apps by exposing a
high level graphics API which includes support for an
important subset of Vulkan design concepts. These include
multithreaded rendering, graphics pipelines, secondary
command buffers and low overhead resource updates. The
underlying Vulkan implementation is abstracted allowing the
developer to focus on implementing graphics concepts. VKK
supports a minimal subset of commonly used graphics features
and object types. VKK also provides a platform interface to
simplify cross-platform development.

The graphics features exposed by VKK include.

* Single header file (vkk.h)
* Rendering to the display
* Image rendering (e.g. render to texture)
* Image stream rendering (e.g. render to texture)
* Secondary rendering (e.g. render to command buffer)
* Multithreaded rendering, creation and destruction
* Graphics pipeline with vertex and fragment shader stages
* Graphics memory is automatically pooled and suballocated
* Shader support for uniform buffers and images
* 2D and 3D images with optional mipmapping are supported
* Triangles are the only primitive supported
* Transparency, depth clearing, viewport and scissors

The graphics object types exposed by VKK include.

* Engine
* Buffers
* Images
* Uniform Sets
* Uniform Set Factories
* Graphics Pipelines
* Pipeline Layouts
* Renderers
* SPIR-V Shaders

The platform features exposed by VKK include.

* Single header file (vkk\_platform.h)
* Cross-platform support for Linux and Android
* Application lifecycle management
* Screen density events for UI scaling
* Keyboard, touch and joystick events
* Android sensor events (accelerometer, magnetometer, gyroscope, and GPS)
* Android permissions (location and storage)

The following sections describe the concepts, features and
objects referenced above.

Engine
======

The engine object is created/destroyed automatically by
the platform and is the main handle that the app uses to
interface with VKK. This interface includes functions to
query the engine state and send commands to the platform.

The vkk\_engine\_version() function allows the app to query
the engine version.

	typedef struct
	{
		unsigned int major:10;
		unsigned int minor:10;
		unsigned int patch:12;
	} vkk_version_t;

	void vkk_engine_version(vkk_engine_t* self,
	                        const vkk_version_t** version);

The vkk\_engine\_appInfo() function allows the app to query
the app info which was passed to the platform.

	void vkk_engine_appInfo(vkk_engine_t* self,
	                        const char** app_name,
	                        const vkk_version_t** app_version);

The vkk\_engine\_internalPath() and
vkk\_engine\_externalPath() functions provide a locations
for the app to store offline data. A resource file is
provided by the app during the build process and is stored
in the internal path.

	const char* vkk_engine_internalPath(vkk_engine_t* self);

	const char* vkk_engine_externalPath(vkk_engine_t* self);

The vkk\_engine\_meminfo() function can be used to determine
the amount of graphics memory allocated by the engine. A
chunk represents a block of Vulkan memory from which a pool
will perform suballocations. A slot is an individual
suballocation from a chunk. The size\_chunks is the total
amount of graphics memory allocated and size\_slots is the
amount of memory actually used. Note that there is a compile
time debug setting in vkk\_engine\_meminfo() that can be
used to print all allocations.

	void vkk_engine_meminfo(vkk_engine_t* self,
	                        size_t* _count_chunks,
	                        size_t* _count_slots,
	                        size_t* _size_chunks,
	                        size_t* _size_slots);

The vkk\_engine\_imageCaps() function allows the app to
query the capabilities supported for a given image format.
Image capabilities flags include texture, mipmap,
filter\_linear, target and target\_blend. The
target\_blend flag indicates if an image bound for
target rendering supports a graphics pipeline with
transparency blending.

	typedef enum
	{
		VKK_IMAGE_FORMAT_RGBA8888 = 0,
		VKK_IMAGE_FORMAT_RGBA4444 = 1,
		VKK_IMAGE_FORMAT_RGBAF32  = 2,
		VKK_IMAGE_FORMAT_RGBAF16  = 3,
		VKK_IMAGE_FORMAT_RGB888   = 4,
		VKK_IMAGE_FORMAT_RGB565   = 5,
		VKK_IMAGE_FORMAT_RGBF32   = 6,
		VKK_IMAGE_FORMAT_RGBF16   = 7,
		VKK_IMAGE_FORMAT_RG88     = 8,
		VKK_IMAGE_FORMAT_RGF32    = 9,
		VKK_IMAGE_FORMAT_RGF16    = 10,
		VKK_IMAGE_FORMAT_R8       = 11,
		VKK_IMAGE_FORMAT_RF32     = 12,
		VKK_IMAGE_FORMAT_RF16     = 13,
		VKK_IMAGE_FORMAT_DEPTH    = 14,
	} vkk_imageFormat_e;

	typedef struct
	{
		unsigned int texture:1;
		unsigned int mipmap:1;
		unsigned int filter_linear:1;
		unsigned int target:1;
		unsigned int target_blend:1;
		unsigned int pad:17;
	} vkk_imageCaps_t;

	void vkk_engine_imageCaps(vkk_engine_t* self,
	                          vkk_imageFormat_e format,
	                          vkk_imageCaps_t* caps);

The vkk\_engine\_defaultRenderer() function can be used to
query for the renderer that can draw to the display.

	vkk_renderer_t* vkk_engine_defaultRenderer(vkk_engine_t* self);

The vkk\_engine\_platformCmd() function allows the app to
send commands to the platform. There are commands to turn
on/off device sensors, play sounds, show the soft keyboard,
load a URL in a browser and request platform permissions. On
Linux, all commands are currently ignored except the
VKK\_PLATFORM\_CMD\_EXIT command.

	typedef enum vkk_platformCmd_s
	{
		VKK_PLATFORM_CMD_ACCELEROMETER_OFF  = 1,
		VKK_PLATFORM_CMD_ACCELEROMETER_ON   = 2,
		VKK_PLATFORM_CMD_CHECK_PERMISSIONS  = 3,
		VKK_PLATFORM_CMD_EXIT               = 4,
		VKK_PLATFORM_CMD_GPS_OFF            = 5,
		VKK_PLATFORM_CMD_GPS_ON             = 6,
		VKK_PLATFORM_CMD_GPS_RECORD         = 7,
		VKK_PLATFORM_CMD_GPS_PAUSE          = 8,
		VKK_PLATFORM_CMD_GYROSCOPE_OFF      = 9,
		VKK_PLATFORM_CMD_GYROSCOPE_ON       = 10,
		VKK_PLATFORM_CMD_LOADURL            = 11,
		VKK_PLATFORM_CMD_MAGNETOMETER_OFF   = 12,
		VKK_PLATFORM_CMD_MAGNETOMETER_ON    = 13,
		VKK_PLATFORM_CMD_PLAY_CLICK         = 14,
		VKK_PLATFORM_CMD_PLAY_NOTIFY        = 15,
		VKK_PLATFORM_CMD_FINE_LOCATION_PERM = 16,
		VKK_PLATFORM_CMD_SOFTKEY_HIDE       = 17,
		VKK_PLATFORM_CMD_SOFTKEY_SHOW       = 18,
		VKK_PLATFORM_CMD_DOCUMENT_CREATE    = 19,
		VKK_PLATFORM_CMD_DOCUMENT_OPEN      = 20,
	} vkk_platformCmd_e;

	void vkk_engine_platformCmd(vkk_engine_t* self,
	                            vkk_platformCmd_e cmd,
	                            const char* msg);

Some commands accept a message in the form of a json string
for additional arguments as follows.

	VKK_PLATFORM_CMD_LOADURL
	{"url":"https://www.google.com"}

	VKK_PLATFORM_CMD_DOCUMENT_CREATE
	# title is optional
	# r=read, w=write, t=trim, a=append
	{"title":"test.txt",
	 "type":"text:plain",
	 "mode":"r|w|wt|wa|rw|rwt"}

	VKK_PLATFORM_CMD_DOCUMENT_OPEN
	# r=read, w=write, t=trim, a=append
	{"type":"text:plain",
	 "mode":"r|w|wt|wa|rw|rwt"}

See the _Renderer_ secton for more details on the default
renderer.

See the _Resource File_ section for more details on the
resource file.

Buffers
=======

Buffers objects may be created by the app for uniform
buffers, vertex buffers and index arrays.

The vkk\_buffer\_new() and vkk\_buffer\_delete() functions
can be used to create/destroy buffer objects.

	typedef enum
	{
		VKK_UPDATE_MODE_STATIC       = 0,
		VKK_UPDATE_MODE_ASYNCHRONOUS = 1,
		VKK_UPDATE_MODE_SYNCHRONOUS  = 2,
	} vkk_updateMode_e;

	typedef enum
	{
		VKK_BUFFER_USAGE_UNIFORM = 0,
		VKK_BUFFER_USAGE_VERTEX  = 1,
		VKK_BUFFER_USAGE_INDEX   = 2,
	} vkk_bufferUsage_e;

	vkk_buffer_t* vkk_buffer_new(vkk_engine_t* engine,
	                             vkk_updateMode_e update,
	                             vkk_bufferUsage_e usage,
	                             size_t size,
	                             const void* buf);
	void vkk_buffer_delete(vkk_buffer_t** _self);

The vkk\_buffer\_size() function allows the app to query the
buffer size.

	size_t vkk_buffer_size(vkk_buffer_t* self);

See the _Renderer_ section for details on updating buffers
and drawing with index/vertex buffers.

See the _Uniform Set_ section for details on attaching a
buffer to a uniform set.

Images
======

Image objects may be created by the app for textures and
image rendering. However, you must query the image
capabilities to determine if the image format is supported.
The VKK\_IMAGE\_FORMAT\_DEPTH format and stage are only
used internally by the engine. Images whose width, height
and depth are a power-of-two may be mipmapped. The stage
flag indicates if the image will be used as a texture for
vertex shaders and/or fragment shaders. The pixels may be
NULL for image rendering.

The vkk\_image\_new() and vkk\_image\_delete() functions can
be used to create/destroy image objects. Note that the F16
image formats require that the pixels be specified as floats
which are then converted internally to half floats.

	typedef enum
	{
		VKK_IMAGE_FORMAT_RGBA8888 = 0,
		VKK_IMAGE_FORMAT_RGBA4444 = 1,
		VKK_IMAGE_FORMAT_RGBAF32  = 2,
		VKK_IMAGE_FORMAT_RGBAF16  = 3,
		VKK_IMAGE_FORMAT_RGB888   = 4,
		VKK_IMAGE_FORMAT_RGB565   = 5,
		VKK_IMAGE_FORMAT_RGBF32   = 6,
		VKK_IMAGE_FORMAT_RGBF16   = 7,
		VKK_IMAGE_FORMAT_RG88     = 8,
		VKK_IMAGE_FORMAT_RGF32    = 9,
		VKK_IMAGE_FORMAT_RGF16    = 10,
		VKK_IMAGE_FORMAT_R8       = 11,
		VKK_IMAGE_FORMAT_RF32     = 12,
		VKK_IMAGE_FORMAT_RF16     = 13,
		VKK_IMAGE_FORMAT_DEPTH    = 14,
	} vkk_imageFormat_e;

	typedef enum
	{
		VKK_STAGE_DEPTH = 0,
		VKK_STAGE_VS    = 1,
		VKK_STAGE_FS    = 2,
		VKK_STAGE_VSFS  = 3,
	} vkk_stage_e;

	vkk_image_t* vkk_image_new(vkk_engine_t* engine,
	                           uint32_t width,
	                           uint32_t height,
	                           uint32_t depth,
	                           vkk_imageFormat_e format,
	                           int mipmap,
	                           vkk_stage_e stage,
	                           const void* pixels);
	void         vkk_image_delete(vkk_image_t** _self);

The vkk\_image\_format() function allows the app to query
the image format.

	vkk_imageFormat_e vkk_image_format(vkk_image_t* self);

The vkk\_image\_size() function allows the app to query the
image size, width and height.

	size_t vkk_image_size(vkk_image_t* self,
	                      uint32_t* _width,
	                      uint32_t* _height,
	                      uint32_t* _depth);

See the _Engine_ section for details on querying image
capabilities.

See the _Renderer_ section for rules on sharing an image
between renderers and for image rendering.

See the _Uniform Set_ section for details on attaching a
image to a uniform set.

Uniform Set
===========

Uniform set objects may be created by the app to encapsulate
a set of uniform variables described by a shader with the
same set index.  When creating the uniform set you must
attach the buffers and images described by the uniform set
factory bindings. Buffer and image references are used when
a different buffer or image may be required each frame.  An
example use case for an image reference is for an animated
GIF. The references are updated by the renderer rather than
attached during creation.

The vkk\_uniformSet\_new() and vkk\_uniformSet\_delete()
functions can be used to create/destroy uniform set objects.

	typedef enum
	{
		VKK_UNIFORM_TYPE_BUFFER     = 0,
		VKK_UNIFORM_TYPE_IMAGE      = 1,
		VKK_UNIFORM_TYPE_BUFFER_REF = 2,
		VKK_UNIFORM_TYPE_IMAGE_REF  = 3,
	} vkk_uniformType_e;

	typedef struct
	{
		uint32_t          binding;
		vkk_uniformType_e type;

		union
		{
			vkk_buffer_t* buffer;
			vkk_image_t*  image;
		};
	} vkk_uniformAttachment_t;

	vkk_uniformSet_t* vkk_uniformSet_new(vkk_engine_t* engine,
	                                     uint32_t set,
	                                     uint32_t ua_count,
	                                     vkk_uniformAttachment_t* ua_array,
	                                     vkk_uniformSetFactory_t* usf);
	void              vkk_uniformSet_delete(vkk_uniformSet_t** _self);

It is important to note that a uniform set implements the
update mode defined by the uniform set factory. The uniform
buffers attached to the uniform set must have the same update
mode or be VKK\_UPDATE\_MODE\_STATIC.

See the _Renderer_ section for details on binding uniform
sets and updating uniform set references.

See the _Shaders_ section for details on the set and binding
indexes.

Uniform Set Factory
===================

Uniform set factory objects may be created by the app which
describes the binding indexes for a particular set index of
a shader.

A uniformBinding is required for every buffer, image, buffer
reference and image reference in the set. The stage flag
indicates if the uniform will be used by vertex shaders
and/or fragment shaders. When the uniform binding type is
for an image then the app must also specify the
sampler filtering and mipmapping modes.

	typedef enum
	{
		VKK_UNIFORM_TYPE_BUFFER     = 0,
		VKK_UNIFORM_TYPE_IMAGE      = 1,
		VKK_UNIFORM_TYPE_BUFFER_REF = 2,
		VKK_UNIFORM_TYPE_IMAGE_REF  = 3,
	} vkk_uniformType_e;

	typedef enum
	{
		VKK_STAGE_DEPTH = 0,
		VKK_STAGE_VS    = 1,
		VKK_STAGE_FS    = 2,
		VKK_STAGE_VSFS  = 3,
	} vkk_stage_e;

	typedef enum
	{
		VKK_SAMPLER_FILTER_NEAREST = 0,
		VKK_SAMPLER_FILTER_LINEAR  = 1,
	} vkk_samplerFilter_e;

	typedef enum
	{
		VKK_SAMPLER_MIPMAP_MODE_NEAREST = 0,
		VKK_SAMPLER_MIPMAP_MODE_LINEAR  = 1,
	} vkk_samplerMipmapMode_e;

	typedef struct
	{
		vkk_samplerFilter_e     min_filter;
		vkk_samplerFilter_e     mag_filter;
		vkk_samplerMipmapMode_e mipmap_mode;
	} vkk_samplerInfo_t;

	typedef struct
	{
		uint32_t          binding;
		vkk_uniformType_e type;
		vkk_stage_e       stage;
		vkk_samplerInfo_t si;
	} vkk_uniformBinding_t;

The vkk\_uniformSet\_new() and vkk\_uniformSet\_delete()
functions can be used to create/destroy uniform set objects.

	typedef enum
	{
		VKK_UPDATE_MODE_STATIC       = 0,
		VKK_UPDATE_MODE_ASYNCHRONOUS = 1,
		VKK_UPDATE_MODE_SYNCHRONOUS  = 2,
	} vkk_updateMode_e;

	vkk_uniformSetFactory_t* vkk_uniformSetFactory_new(vkk_engine_t* engine,
	                                                   vkk_updateMode_e update,
	                                                   uint32_t ub_count,
	                                                   vkk_uniformBinding_t* ub_array);
	void                     vkk_uniformSetFactory_delete(vkk_uniformSetFactory_t** _self);

See the _Pipeline Layout_ section to create a pipeline
layout from an array of uniform set factories.

See the _Renderer_ section for for rules on sharing the
uniform set factory between renderers.

See the _Shaders_ section for details on the set and
binding indexes.

See the _Uniform Set_ section to create uniform sets from a
uniform set factory.

Pipeline Layout
===============

Pipeline layout objects may be created by the app which
describe the collection of sets and bindings that may be
used by a graphics pipeline and its associated shaders.

The vkk\_pipelineLayout\_new() and
vkk\_pipelineLayout\_delete() functions can be used to
create/destroy pipeline layout objects. The uniform set
factories which where created with uniformBindings include
the information necessary to describe the collection of sets
and bindings for the pipeline layout.

	vkk_pipelineLayout_t* vkk_pipelineLayout_new(vkk_engine_t* engine,
	                                             uint32_t usf_count,
	                                             vkk_uniformSetFactory_t** usf_array);
	void                  vkk_pipelineLayout_delete(vkk_pipelineLayout_t** _self);

See the _Graphics Pipeline_ section for attaching a
pipeline layout to a graphics pipeline.

See the _Renderer_ section for rules on sharing the pipeline
layout between renderers.

See the _Shaders_ section for guidelines on choosing a
strategy for selecting the set and binding indexes.

Graphics Pipeline
=================

Graphics pipeline objects may be created by the app which
describes the graphics state required for rendering.
Graphics state may be swapped during rendering by simply
binding a new graphics pipeline object. Graphics pipelines
are interchangeable when the pipeline layout and the
renderer are the same. The following graphics state is
described by a graphics pipeline object.

The vertexBufferInfo encodes the location index, vertex
format and number of components per vertex. Only 1-4
components are supported.

	typedef enum
	{
		VKK_VERTEX_FORMAT_FLOAT  = 0,
		VKK_VERTEX_FORMAT_SINT   = 1,
		VKK_VERTEX_FORMAT_SSHORT = 2,
		VKK_VERTEX_FORMAT_UINT   = 3,
		VKK_VERTEX_FORMAT_USHORT = 4,
	} vkk_vertexFormat_e;

	typedef struct
	{
		uint32_t           location;
		uint32_t           components;
		vkk_vertexFormat_e format;
	} vkk_vertexBufferInfo_t;

The primitives supported include triangle lists, triangle
strips and triangle fans. The primitive restart flag may be
used with indexed rendering to allow a -1 index to cause
the rasterizer to begin a new primitive. Primitive restart
can be useful to draw multiple primitives with a single draw
call.

	typedef enum
	{
		VKK_PRIMITIVE_TRIANGLE_LIST  = 0,
		VKK_PRIMITIVE_TRIANGLE_STRIP = 1,
		VKK_PRIMITIVE_TRIANGLE_FAN   = 2,
	} vkk_primitive_e;

The graphics pipeline supports a transparency blending mode
(e.g. one minus src alpha).

	typedef enum
	{
		VKK_BLEND_MODE_DISABLED     = 0,
		VKK_BLEND_MODE_TRANSPARENCY = 1,
	} vkk_blendMode_e;

The vertex and fragment shaders are specified by providing
a path to the SPIR-V shader in the resource file. The
cull\_back, depth\_test and depth\_write flags enable the
corresponding functionality.

	typedef struct
	{
		vkk_renderer_t*         renderer;
		vkk_pipelineLayout_t*   pl;
		const char*             vs;
		const char*             fs;
		uint32_t                vb_count;
		vkk_vertexBufferInfo_t* vbi;
		vkk_primitive_e         primitive;
		int                     primitive_restart;
		int                     cull_back;
		int                     depth_test;
		int                     depth_write;
		vkk_blendMode_e         blend_mode;
	} vkk_graphicsPipelineInfo_t;

The vkk\_graphicsPipeline\_new() and
vkk\_graphicsPipeline\_delete() functions can be used to
create/destroy graphics pipeline objects.

	vkk_graphicsPipeline_t* vkk_graphicsPipeline_new(vkk_engine_t* engine,
	                                                 vkk_graphicsPipelineInfo_t* gpi);
	void                    vkk_graphicsPipeline_delete(vkk_graphicsPipeline_t** _self);

See the _Renderer_ section for binding a graphics pipeline.

See the _Resource File_ section for including shaders in
the resource file.

Renderer
========

Renderer objects are avilable to the app for several
different rendering cases. The default renderer allows the
app to render to the display, the image renderer allows the
app to render to an image, the image stream renderer allows
the app to render to a stream of image references and the
secondary renderer allows the app to record commands to
secondary command buffers. The default renderer object is
created/destroyed automatically by the engine while the
remaining renderer objects may be created/destroyed by the
following functions.

	typedef enum
	{
		VKK_IMAGE_FORMAT_RGBA8888 = 0,
		VKK_IMAGE_FORMAT_RGBA4444 = 1,
		VKK_IMAGE_FORMAT_RGBAF32  = 2,
		VKK_IMAGE_FORMAT_RGBAF16  = 3,
		VKK_IMAGE_FORMAT_RGB888   = 4,
		VKK_IMAGE_FORMAT_RGB565   = 5,
		VKK_IMAGE_FORMAT_RGBF32   = 6,
		VKK_IMAGE_FORMAT_RGBF16   = 7,
		VKK_IMAGE_FORMAT_RG88     = 8,
		VKK_IMAGE_FORMAT_RGF32    = 9,
		VKK_IMAGE_FORMAT_RGF16    = 10,
		VKK_IMAGE_FORMAT_R8       = 11,
		VKK_IMAGE_FORMAT_RF32     = 12,
		VKK_IMAGE_FORMAT_RF16     = 13,
		VKK_IMAGE_FORMAT_DEPTH    = 14,
	} vkk_imageFormat_e;

	typedef enum
	{
		VKK_STAGE_DEPTH = 0,
		VKK_STAGE_VS    = 1,
		VKK_STAGE_FS    = 2,
		VKK_STAGE_VSFS  = 3,
	} vkk_stage_e;

	vkk_renderer_t* vkk_renderer_newImage(vkk_engine_t* engine,
	                                      uint32_t width,
	                                      uint32_t height,
	                                      vkk_imageFormat_e format);
	vkk_renderer_t* vkk_renderer_newImageStream(vkk_renderer_t* consumer,
	                                            uint32_t width,
	                                            uint32_t height,
	                                            vkk_imageFormat_e format,
	                                            int mipmap,
	                                            vkk_stage_e stage);
	vkk_renderer_t* vkk_renderer_newSecondary(vkk_renderer_t* executor);
	void            vkk_renderer_delete(vkk_renderer_t** _self);

The rendering commands are issued between begin() and
end() functions. The begin() and end() functions serve as
synchronization points which greatly simplify the app
interface when compared with the underlying Vulkan
implementation. If the begin() function succeeds then the
app must also call the end() function. When one renderer
depends on another, its dependent renderer must be started
first and ended in the reverse order. For example an image
stream which produces images for the default renderer must
perform the following sequence.

	vkk_renderer_beginDefault(default, ...);
	vkk_renderer_beginImageStream(image_stream, ...);
	...
	vkk_renderer_end(image_stream);
	vkk_renderer_end(default);

The drawing commands for the image stream/secondary
renderers can be issued in a different thread once both
begin() functions complete.

The rendering mode determines the type of rendering commands
that may be issued. The VKK\_RENDERER\_MODE\_DRAW rendering
mode allows all rendering commands except for the
vkk\_renderer\_execute() function. The
VKK\_RENDERER\_MODE\_EXECUTE rendering mode only allows the
vkk\_renderer\_execute() and vkk\_renderer\_surfaceSize()
functions. The vkk\_renderer\_beginImage() function accepts
an image that will be used as a render target. Note that
the depth buffer, viewport and scissor are initialized
automatically by the begin() functions.

	typedef enum
	{
		VKK_RENDERER_MODE_DRAW    = 0,
		VKK_RENDERER_MODE_EXECUTE = 1,
	} vkk_rendererMode_e;

	int          vkk_renderer_beginDefault(vkk_renderer_t* self,
	                                       vkk_rendererMode_e mode,
	                                       float* clear_color);
	int          vkk_renderer_beginImage(vkk_renderer_t* self,
	                                     vkk_rendererMode_e mode,
	                                     vkk_image_t* image,
	                                     float* clear_color);
	vkk_image_t* vkk_renderer_beginImageStream(vkk_renderer_t* self,
	                                           vkk_rendererMode_e mode,
	                                           float* clear_color);
	int          vkk_renderer_beginSecondary(vkk_renderer_t* self);
	void         vkk_renderer_end(vkk_renderer_t* self);

The image renderer images may only be used as a render
target for a single image renderer at once. Images must
not be in use by another renderer (e.g. used between the
renderer begin()/end() functions) prior to beginning image
rendering with the image. Images must match the size and
format of the image renderer.

The image stream renderer returns an image reference that
may be used by the consumer for the current frame even
while the image stream rendering is still in progress.

The app may utilize secondary rendering to optimize
performance by creating multiple secondary renderers to
record various parts of the scene in parallel. The drawing
doesn't actually occur until the secondary renderers
commands are executeted by the vkk\_renderer\_execute()
function. A secondary renderer may only be drawn once per
frame into the execute renderer. The commands recorded to
the secondary command buffer are only valid for the current
frame and must be re-recorded for subsequent frames.

Renderers may share images, uniform set factories and
pipeline layouts. Renderers may share buffers and uniform
sets when update is set to VKK\_UPDATE\_MODE\_STATIC or
when one renderer is the consumer/executor of the other.
Renderers may not share graphics pipelines.

The vkk\_renderer\_surfaceSize() function allows the app to
query the renderer for the surface size.

	void vkk_renderer_surfaceSize(vkk_renderer_t* self,
	                              uint32_t* _width,
	                              uint32_t* _height);

The vkk\_renderer\_updateMode() function may be used to
query the non-static update mode supported by the renderer.

	typedef enum
	{
		VKK_UPDATE_MODE_STATIC       = 0,
		VKK_UPDATE_MODE_ASYNCHRONOUS = 1,
		VKK_UPDATE_MODE_SYNCHRONOUS  = 2,
	} vkk_updateMode_e;

	vkk_updateMode_e vkk_renderer_updateMode(vkk_renderer_t* self);

The vkk\_renderer\_updateBuffer() function may be used to
update uniform, vertex and index buffers. A buffer is
considered updatable when its update mode is not
VKK\_UPDATE\_MODE\_STATIC. Uniform buffers which are
updatable must be updated once and only once per frame and
the entire buffer must be updated. The rules for updating a
vertex/index buffer are subtly different. When a
vertex/index buffer is updatable then the app may update
the buffer zero or one time per frame. The app may update a
subset of the vertex/index buffer but only the portion of
the buffer which was updated is valid. The partial updates
for vertex/index buffers allows the app to avoid
reallocating a vertex/index buffer per frame when the
underlying geometry may be changing shape.

	void vkk_renderer_updateBuffer(vkk_renderer_t* self,
	                               vkk_buffer_t* buffer,
	                               size_t size,
	                               const void* buf);

The vkk\_renderer\_updateUniformSetRefs() function may be
used to update uniform set references. When a uniform set
includes such a reference then they must be updated once
and only once per frame.

	typedef enum
	{
		VKK_UNIFORM_TYPE_BUFFER     = 0,
		VKK_UNIFORM_TYPE_IMAGE      = 1,
		VKK_UNIFORM_TYPE_BUFFER_REF = 2,
		VKK_UNIFORM_TYPE_IMAGE_REF  = 3,
	} vkk_uniformType_e;

	typedef struct
	{
		uint32_t          binding;
		vkk_uniformType_e type;

		union
		{
			vkk_buffer_t* buffer;
			vkk_image_t*  image;
		};
	} vkk_uniformAttachment_t;

	void vkk_renderer_updateUniformSetRefs(vkk_renderer_t* self,
	                                       vkk_uniformSet_t* us,
	                                       uint32_t ua_count,
	                                       vkk_uniformAttachment_t* ua_array);

The vkk\_renderer\_bindGraphicsPipeline() function may be
used to bind a new graphics pipeline state.

	void vkk_renderer_bindGraphicsPipeline(vkk_renderer_t* self,
	                                       vkk_graphicsPipeline_t* gp);

The vkk\_renderer\_bindUniformSets() function may be used to
bind multiple uniform sets simultaneously. The function
takes an array of uniform sets whose set indexes must be in
order and without gaps. If a shader requires sets
{ 0, 1, 3 } then the vkk\_renderer\_bindUniformSets()
function must be called twice with the uniform sets { 0, 1 }
and { 3 } to meet the requirements of the underlying Vulkan
API. Only the uniform sets for the shaders referenced by
the current graphics pipeline need to be bound.

Note that _Qualcomm_ devices seem to drop rendering
commands when the set indexes are not continuous so it is
recommended to bind unused sets to avoid this scenario.

	void vkk_renderer_bindUniformSets(vkk_renderer_t* self,
	                                  uint32_t us_count,
	                                  vkk_uniformSet_t** us_array);

The vkk\_renderer\_clearDepth() function can be called to
reset the depth buffer.

	void vkk_renderer_clearDepth(vkk_renderer_t* self);

The app may override the viewport or scissor with the
vkk\_renderer\_viewport() and vkk\_renderer\_scissor()
functions.

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

The following drawing functions may be called by the app to
issue drawing commands. Note that the vertex format of the
vertex buffer must match the currently bound graphics
pipeline.

	typedef enum
	{
		VKK_INDEX_TYPE_USHORT = 0,
		VKK_INDEX_TYPE_UINT   = 1,
	} vkk_indexType_e;

	void vkk_renderer_draw(vkk_renderer_t* self,
	                       uint32_t vertex_count,
	                       uint32_t vertex_buffer_count,
	                       vkk_buffer_t** vertex_buffers);
	void vkk_renderer_drawIndexed(vkk_renderer_t* self,
	                              uint32_t index_count,
	                              uint32_t vertex_buffer_count,
	                              vkk_indexType_e index_type,
	                              vkk_buffer_t* index_buffer,
	                              vkk_buffer_t** vertex_buffers);

The following execute function may be called by the app to
issue drawing commands stored in secondary command buffers.

	void vkk_renderer_execute(vkk_renderer_t* self,
	                          uint32_t secondary_count,
	                          vkk_renderer_t** secondary_array);

See the _Engine_ section to query the default renderer.

See the _Graphics Pipeline_ section for attaching a
renderer to the graphics pipeline.

See the _Threading/Synchronization_ section for threading
and synchronization rules regarding the renderer.

Shaders
=======

The VKK library accepts SPIR-V shader input which can be
generated from GLSL shaders with the glslangValidator tool
which is available from the LunarG SDK.

	https://www.lunarg.com/

The following command may be used to generate the SPIR-V
and the resulting binary should be packed into the
resource file (see also the _Resource File_ section).

	glslangValidator -V shader.vert -o shader_vert.spv

When porting apps from OpenGL you may need to modify your
GLSL shaders to include the layout qualifiers. Unlike
OpenGL, the Vulkan API does not allow the app to query a
uniform or attriubute location. The layout qualifiers are
required for the VKK library to specify how the shader
inputs (attributes and uniforms) are mapped from the Vulkan
API to the SPIR-V shaders. Here are a few examples.

For vertex attributes:

	layout(location=0) in vec4 xyuv;

For uniform buffers:

	layout(std140, set=0, binding=0) uniform uniformMvp
	{
		mat4 mvp;
	};

For uniform samplers:

	layout(set=2, binding=1) uniform sampler2D image;

The location index corresponds to the value defined by
vkk\_vertexBufferInfo\_t and also must match the array index
of vertex_buffers data passed into
vkk\_renderer\_draw()/vkk\_renderer\_drawIndexed().

The set index corresponds to the valued passed into
vkk\_uniformSet\_new().

The binding index corresponds to the value defined by
vkk\_uniformAttachment\_t and vkk\_uniformBinding\_t.

The developer is responsible for assigning the location,
set and binding indices. Unfortunately the assignment
process is a black art and there may be many different
viable strategies but generally depends on achieving the
following goals.

1. Try to minimize the number of graphics pipelines required
2. Try to share uniform data across multiple shaders
3. Try to minimize the number of calls to bind uniform sets

As with Vulkan, a shader is not required to make use of all
uniform sets/bindings defined by its corresponding pipeline
layout.

The std140 layout for uniform buffers defines the rules for
packing elements within the buffer. In practice, this
generally means that the array stride between elements
should be rounded up to vec4. For example, rather than
passing a mat3 matrix you should instead pass a mat4 matrix
then cast it to a mat3 in the shader. The VKK library does
not restrict the types of layouts supported however it seems
that std140 is most portable so this is recommended.

Note that the VKK library restricts the number of sets
supported to 4 (set 0-3) since this is the minimum guaranteed
by the Vulkan spec (see maxBoundDescriptorSets).

Threading/Synchronization
=========================

The app should observe the following threading and
synchronization rules.

1.  Objects may be created from any thread
2.  Objects may be deleted from any thread as long as any
    renderer which used the object has already called end()
3.  Objects must not be used by any thread once deleted
4.  Object deletion is a non-blocking operation
5.  Images should be created in a worker thread since the
    GPU is required to transfer the pixels and optionally
    mipmap the image
6.  The rendering begin() function may block while waiting
    for the next framebuffer to become available
7.  Synchronization across separate renderers is not
    required
8.  Synchronization for a specific renderer is required when
    called from multiple threads
9.  The default renderer completes asynchronously
10. Image renderers completes synchronously
11. Image stream and secondary renderers may complete
    synchronously or asynchronously depending on their
    corresponding consumer or executor renderers
12. GPU synchronization is handled automatically by the
    engine

Platform
========

The platform provides a cross-platform interface which
encapsulates the SDL2 interface on Linux and NativeActivity
interface on Android.

The platform interface requires for the app to declare a
VKK\_PLATFORM\_INFO variable which contains the app
name/version information and create/destroy/pause/draw/event
callback functions. These callback functions are always
called from the main thread except for GPS events which are
called from the UI thread. This is because the app may have
requested GPS recording which can cause events to be
delivered when the app main thread is paused.

	typedef void* (*vkk_platformOnCreate_fn)(vkk_engine_t* engine);
	typedef void  (*vkk_platformOnDestroy_fn)(void** _priv);
	typedef void  (*vkk_platformOnPause_fn)(void* priv);
	typedef void  (*vkk_platformOnDraw_fn)(void* priv);
	typedef void  (*vkk_platformOnEvent_fn)(void* priv,
	                                        vkk_event_t* event);

	typedef struct
	{
		const char*              app_name;
		const char*              app_dir;
		vkk_version_t            app_version;
		vkk_platformOnCreate_fn  onCreate;
		vkk_platformOnDestroy_fn onDestroy;
		vkk_platformOnPause_fn   onPause;
		vkk_platformOnDraw_fn    onDraw;
		vkk_platformOnEvent_fn   onEvent;
	} vkk_platformInfo_t;

	extern vkk_platformInfo_t VKK_PLATFORM_INFO;

The platform event callbacks provides Android specific
sensor events for accelerometer, magnetometer, gyroscope and
GPS sensors. Additional events are generated for human input
devices such as keyboards, mice, touchscreens and joysticks.
And finally, events are also generated for Android specific
changes to screen density, the content rect and permissions.

	typedef enum
	{
		VKK_EVENT_TYPE_UNDEFINED         = -1,
		VKK_EVENT_TYPE_ACCELEROMETER     = 0,
		VKK_EVENT_TYPE_ACTION_DOWN       = 1,
		VKK_EVENT_TYPE_ACTION_MOVE       = 2,
		VKK_EVENT_TYPE_ACTION_UP         = 3,
		VKK_EVENT_TYPE_AXIS_MOVE         = 4,
		VKK_EVENT_TYPE_BUTTON_DOWN       = 5,
		VKK_EVENT_TYPE_BUTTON_UP         = 6,
		VKK_EVENT_TYPE_DENSITY           = 7,
		VKK_EVENT_TYPE_DOCUMENT          = 8,
		VKK_EVENT_TYPE_GPS               = 9,
		VKK_EVENT_TYPE_GYROSCOPE         = 10,
		VKK_EVENT_TYPE_KEY_DOWN          = 11,
		VKK_EVENT_TYPE_KEY_UP            = 12,
		VKK_EVENT_TYPE_MAGNETOMETER      = 13,
		VKK_EVENT_TYPE_CONTENT_RECT      = 14,
		VKK_EVENT_TYPE_PERMISSION_STATUS = 15,
		VKK_EVENT_TYPE_LOW_MEMORY        = 16,
	} vkk_eventType_e;

	typedef struct
	{
		float ax;
		float ay;
		float az;
		int   rotation;
	} vkk_eventAccelerometer_t;

	typedef struct
	{
		int count;
		cc_vec2f_t coord[VKK_EVENT_ACTION_COUNT];
	} vkk_eventAction_t;

	typedef struct
	{
		int        id;
		vkk_axis_e axis;
		float      value;
	} vkk_eventAxis_t;

	typedef struct
	{
		int          id;
		vkk_button_e button;
	} vkk_eventButton_t;

	typedef struct
	{
		char uri[256];
		int  fd;
	} vkk_eventDocument_t;

	typedef struct
	{
		double lat;
		double lon;
		float  accuracy;
		float  altitude;
		float  speed;
		float  bearing;
	} vkk_eventGps_t;

	typedef struct
	{
		float ax;
		float ay;
		float az;
	} vkk_eventGyroscope_t;

	typedef struct
	{
		int keycode;
		int meta;
		int repeat;
	} vkk_eventKey_t;

	typedef struct
	{
		float mx;
		float my;
		float mz;
		float gfx;
		float gfy;
		float gfz;
	} vkk_eventMagnetometer_t;

	typedef struct
	{
		int t;
		int l;
		int b;
		int r;
	} vkk_eventContentRect_t;

	typedef enum
	{
		VKK_PERMISSION_FINE_LOCATION       = 1,
		VKK_PERMISSION_READ_STORAGE        = 2,
		VKK_PERMISSION_WRITE_STORAGE       = 3,
	} vkk_permission_e;

	typedef struct
	{
		vkk_permission_e permission;
		int status;
	} vkk_eventPermission_t;

	typedef struct
	{
		vkk_eventType_e type;
		double ts;
		union
		{
			vkk_eventAccelerometer_t accelerometer;
			vkk_eventAction_t        action;
			vkk_eventAxis_t          axis;
			vkk_eventButton_t        button;
			vkk_eventDocument_t      document;
			float                    density;
			vkk_eventGps_t           gps;
			vkk_eventGyroscope_t     gyroscope;
			vkk_eventKey_t           key;
			vkk_eventMagnetometer_t  magnetometer;
			vkk_eventContentRect_t   content_rect;
			vkk_eventPermission_t    permission;
		};
	} vkk_event_t;

The keycodes use the standard ASCII keycode except for the
following special keys.

	#define VKK_KEYCODE_ENTER     0x00D
	#define VKK_KEYCODE_ESCAPE    0x01B
	#define VKK_KEYCODE_BACKSPACE 0x008
	#define VKK_KEYCODE_DELETE    0x07F
	#define VKK_KEYCODE_UP        0x100
	#define VKK_KEYCODE_DOWN      0x101
	#define VKK_KEYCODE_LEFT      0x102
	#define VKK_KEYCODE_RIGHT     0x103
	#define VKK_KEYCODE_HOME      0x104
	#define VKK_KEYCODE_END       0x105
	#define VKK_KEYCODE_PGUP      0x106
	#define VKK_KEYCODE_PGDOWN    0x107
	#define VKK_KEYCODE_INSERT    0x108

The meta field is a mask containing the following values.

	#define VKK_META_ALT     0x00000032
	#define VKK_META_ALT_L   0x00000010
	#define VKK_META_ALT_R   0x00000020
	#define VKK_META_CTRL    0x00007000
	#define VKK_META_CTRL_L  0x00002000
	#define VKK_META_CTRL_R  0x00004000
	#define VKK_META_SHIFT   0x000000C1
	#define VKK_META_SHIFT_L 0x00000040
	#define VKK_META_SHIFT_R 0x00000080
	#define VKK_META_CAPS    0x00100000

See the _Dependencies_ section for details on the
VKKNativeActivity interface required for Android.

See the _Engine_ section for details on platform commands.

See the _Known Issues_ section for joystick events.

Dependencies
============

The engine uses libcc for logging, memory tracking, data
structures (e.g. lists/maps), threading constructs and
vector math operations. Note that you may find it useful to
use the cc\_mat4f\_perspective() and cc\_mat4f\_orthoVK()
functions to prepare the perspective projection and
orthographic projection matrices since viewport origin has
flipped from OpenGL to Vulkan.

	https://github.com/jeffboody/libcc

The engine uses a resource file stored in the bfs file
format for shader modules to simplify cross-platform
resource management.

	https://github.com/jeffboody/libbfs

On Android, the app must extend the VKKNativeActivity define
the app native library and pass platform events to the
platform independent interface.

	https://github.com/jeffboody/vkk-java

Resource File
=============

The app should provide a resource file which may contain
shaders, images, icons, fonts and any other resources.
The resource file is named resource.bfs and can be
generated by the bfs tool found at libbfs/bfs. The
resource file should be installed at the engine resource
path.

The following example shows how to generate a resource
file.

	bfs resource.bfs blobSet readme.txt
	bfs resource.bfs blobSet shader_vert.spv
	bfs resource.bfs blobSet shader_frag.spv

See the _Engine_ section for details on the resource path.

Known Issues
============

1. Samsung soft keyboard events not recognized (use Google
   Gboard as workaround)
2. Fullscreen does not show/hide properly on Samsung phones
3. Joystick events are a work-in-progress

VKUI Library
============

The VKUI library is an experimental widget library that is
built on top of the VKK library.

VKUI must be enabled by setting/exporting the VKK\_USE\_VKUI
variable.

See vkui/vkui.h for more details.

Example
=======

See the master-vkk branch of the gearsvk app for an example
which uses the VKK Library.

	https://github.com/jeffboody/gearsvk

Note that you may compare this implementation with the
master branch which uses Vulkan directly.

Feedback
========

Send feedback to Jeff Boody.

	jeffboody@gmail.com
