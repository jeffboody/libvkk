About
=====

The Vulkan Kit (VKK) is a graphics library that exposes a
simplified graphics pipeline layered on the the Vulkan
graphics library. The features exposed by the VKK library
are limited to the subset necessary to port my own OpenGL
apps to Vulkan. These features include the following.

* Cross platform development with Linux and Android
* Rendering to the display
* Offscreen rendering (e.g. render to texture)
* Secondary rendering (e.g. render to command buffer)
* Multithreaded rendering, creation and destruction
* Graphics pipeline with vertex and fragment shader stages
* Graphics memory is automatically pooled and suballocated
* Shader support for uniform buffers and images
* 2D textures with optional mipmapping are supported
* Triangles are the only primitive supported
* Transparency, depth clearing, viewport and scissors
* Single header file graphics interface (vkk.h)
* Single header file platform interface (vkk\_platform.h)

The following sections describe the engine, objects,
shaders, threading/synchronization, platform interface and
miscelaneous topics.

Engine
======

The engine object is created/destroyed automatically by
the platform and is the main handle that the app uses to
interface with the VKK library. This interface includes
functions to query the engine version, resource path, the
meminfo, image capabilities and handle to the default
renderer. The interface also provides a function to send
commands to the platform.

The resource path maps to the internal data path on
Android or the current directory on Linux.

The meminfo can be used to determine the amount of
graphics memory allocated by the engine. A chunk represents
a block of Vulkan memory from which a pool will perform
suballocations and a slot is an individual suballocation.
The size\_chunks is the total amount of graphics memory
allocated and size\_slots is the amount of memory actually
used. Note that there is a compile time debug setting in
meminfo that can be used to print all allocations.

The imageCaps function allows the app the hardware
capabilities supported for a given format.

The default renderer is used to draw to the display.

The platformCmd function allows the app to send commands
to the platform. There are commands to turn on/off device
sensors, play sounds, show the soft keyboard, load a URL
in a browser and request platform permissions. On Linux,
all commands are currently ignored except the EXIT command.

See the _Resource File_ section for details on the
resource.pak file.

Buffers
=======

Buffer objects may be created by the app for uniform, vertex
and index buffers.

The app must specify how a buffer will be updated. The
update modes supported include STATIC, DEFAULT and
OFFSCREEN. These update modes allow the VKK library to
provide a simplified interface that combines buffer, memory
management and synchronization.

Uniform buffers support all GLSL types and layouts defined
by std140.

Vertex buffers may have FLOAT, SINT, SSHORT, UINT or USHORT
elements and may have 1-4 components per element. Note that
packing of multiple attributes into a single buffer is not
currently supported.

Index buffers may have USHORT or UINT elements.

See the _Renderer_ section for the definition of update
modes and rules for updating buffers.

Images
======

Image objects may be created by the app for textures and
offscreen rendering.

The image formats exposed include RGBA8888, RGBA4444,
RGB888, RGB565, RG88 and R8. However, you must query the
image capabilities to determine if the image format is
supported by hardware. Image capabilities flags include
TEXTURE, MIPMAP, FILTER\_LINEAR, OFFSCREEN and
OFFSCREEN\_BLEND. The OFFSCREEN\_BLEND flag indicates if
an image bound for offscreen rendering supports a graphics
pipeline with transparency blending.

Images whose width and height are a power-of-two may be
mipmapped.

The app must specify all shader stages that the image will
be used. These shader stages may include VS, FS or VSFS.

See the _Renderer_ section for rules on sharing an image
between renderers and for offscreen rendering.

Uniform Set
===========

Uniform set objects may be created by the app to encapsulate
a set of uniform variables described by a shader with the
same set index. When creating the uniform set you must
attach the buffers and images described by the uniform set
factory bindings. Note that the buffer and image references
are updated by the renderer rather than attached during
creation.

It is important to note that a uniform set implements the
update mode defined by the uniform set factory. The uniform
buffers attached to the uniform set must have the same update
mode or be STATIC. These update modes allow the VKK library
to provide a simplified interface that combines buffer,
memory management and synchronization.

See the _Shaders_ section for details on the set index.

See the _Uniform Set Factory_ section to create a uniform
set factory.

See the _Renderer_ section for details on binding uniform
sets, updating uniform set references, definition of update
modes and rules for updating buffers.

Uniform Set Factory
===================

Uniform set factory objects may be created by the app which
describe the binding indexes for a particular set index of
a shader. This binding description is used to manage an
internal pool of Vulkan descriptor sets and enables the app
to create uniform sets.

The app must specify the binding type which may include
BUFFER, IMAGE, BUFFER\_REF or IMAGE\_REF. The BUFFER and
IMAGE types indicate that the same buffer or image will
always be attached to a uniform set created by the uniform
set factory. The BUFFER\_REF and IMAGE\_REF types indicate
that the renderer MUST update the uniform set reference
for each frame. References are useful for the app to swap
the buffer/image rather than updating that buffer/image at
draw time. An example use case for an image reference is an
animated GIF.

The app must specify all shader stages that the uniform set
will be used. These shader stages may include VS, FS or
VSFS.

When the uniform binding type is IMAGE or IMAGE\_REF then the
app must also specify the sampler information. This includes
the min\_filter, mag\_filter and mipmap\_mode.

See the _Shaders_ section for details on the set and
binding indexes.

See the _Renderer_ section for updating uniform set
references and for rules on sharing the uniform set factory
between renderers.

Pipeline Layout
===============

Pipeline layout objects may be created by the app which
describe the collection of sets and bindings which may be
used by shaders when a graphics pipeline is bound.

See the _Shaders_ section for guidelines on choosing a
strategy for selecting the set and binding indexes.

See the _Renderer_ section for rules on sharing the pipeline
layout between renderers.

Graphics Pipeline
=================

Graphics pipeline objects may be created by the app which
encapsulates the graphics state required for rendering. The
graphics pipeline state includes the renderer, pipeline
layout, shaders, vertex buffer state, primitive state,
depth state and blend state.

Graphics pipelines may be swapped during rendering by simply
binding a new graphics pipeline object.

Graphics pipelines are interchangeable when the pipeline
layout and the renderer are the same.

See the _Renderer_ section for binding a graphics pipeline.

Renderer
========

Renderer objects are avilable to the app for three different
rendering cases. The default renderer allows the app to
render to the display, the offscreen renderer allows the app
to render to an image and the secondary renderer allows the
app to record commands to a secondary command buffer. The
default renderer object is created/destroyed automatically
by the engine while the offscreen/secondary renderer objects
may be created by the app.

The rendering commands are issued between begin and end
functions where the begin function is specific to the
renderer type. If the begin function succeeds then the app
must also call the end function. The default/offscreen
renderers accepts a rendering mode which determines the type
of rendering commands that may be issued. The PRIMARY
rendering mode allows all rendering commands except for
vkk\_renderer\_drawSecondary(). The SECONDARY rendering mode
only allows vkk\_renderer\_drawSecondary() and
vkk\_renderer\_surfaceSize(). Note that the depth buffer,
viewport and scissor are initialized automatically by the
begin function.

The app may utilize secondary rendering to optimize
performance by creating multiple secondary renderers to
record various parts of the scene in parallel. The drawing
doesn't actually occur until the secondary renderers are
drawn into a SECONDARY renderer with the
vkk\_renderer\_drawSecondary() function. A secondary
renderer may only be drawn once per frame into a SECONDARY
renderer. The commands recorded to the secondary command
buffer are only valid for the current frame and must be
re-recorded for subsequent frames. And finally, the
vkk\_renderer\_beginSecondary() function should only be
called if the corresponding SECONDARY renderer begin
function was successful.

Images may only be used as a render target for a single
offscreen renderer at once. Images must not be in use
by another renderer (e.g. used between the renderer
begin/end functions) prior to beginning offscreen
rendering with the image.

Renderers may share images, uniform set factories and
pipeline layouts. Renderers may share buffers and uniform
sets only when update is set to STATIC. Renderers may not
share graphics pipelines.

The renderer provides a minimal interface for performing
rendering operations. These operations allow the app to
update buffers/uniform set references, bind graphics
pipelines/uniform sets, clear depth, set
viewport/scissor and draw geometry.

The vkk\_renderer\_updateBuffer() function may be used to
update uniform, vertex and index buffers. The default
renderer may only update buffers when the update mode is
set to DEFAULT and offscreen renderers may only update
buffers when the update mode is set to OFFSCREEN. When a
uniform buffer is declared with an update mode of DEFAULT or
OFFSCREEN then the app must update the buffer once and only
once every frame. The app must update the entire uniform
buffer. The rules for updating a vertex/index buffer are
subtly different. When a vertex/index buffer is declared
with an update mode of DEFAULT or OFFSCREEN then the app
may update the buffer zero or one time per frame. The app
may update a subset of the vertex/index buffer. The partial
updates for vertex/index buffers allows the app to avoid
reallocating a vertex/index buffer per frame when the
underlying geometry may be changing shape.

The vkk\_renderer\_updateUniformSetRefs() function may be
used to update references for a uniform set (BUFFER\_REF and
IMAGE\_REF). When a uniform set includes such a reference
then they muse be updated once and only once per frame.

The vkk\_renderer\_bindGraphicsPipeline() function may be
used to bind a new graphics pipeline state.

The vkk\_renderer\_bindUniformSets() function may be used to
to bind multiple uniform sets simultaneously. The function
takes an array of uniform sets whose set indexes must be in
order and without gaps. If a shader requires sets
{ 0, 1, 3 } then vkk\_renderer\_bindUniformSets() must be
called twice with the uniform sets { 0, 1 } and { 3 } to
meet the requirements of the underlying Vulkan API. If
a shader for the bound graphics pipeline does not require
a particular uniform set then doens't need to be bound.

The app may overwite the viewport or scissor with the
vkk\_renderer\_viewport() and vkk\_renderer\_scissor()
functions. Note that you may find it useful to use the
cc\_mat4f\_perspective() and cc\_mat4f\_orthoVK() functions
to prepare the perspective projection and orthographic
projection matrices since viewport origin has flipped from
OpenGL to Vulkan.

The app may issue drawing commands with the
vkk\_renderer\_draw() and vkk\_renderer\_drawIndexed()
functions.

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
    renderer which used the object has already called end
3.  Objects must not be used by any thread once deleted
4.  Object deletion is a non-blocking operation
5.  Images should be created in a worker thread since the
    GPU is required to transfer the pixels and optionally
    mipmap the image
6.  The rendering begin function may block while waiting for
    the next framebuffer to become available
7.  Synchronization across separate renderers is not
    required
8.  Synchronization for a specific renderer is required when
    called from multiple threads
9.  Default and secondary renderers complete asynchronously
10. Offscreen renderers completes synchronously
11. GPU synchronization is handled automatically by the
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

The platform event callbacks provides Android specific
sensor events for accelerometer, magnetometer, gyroscope and
GPS sensors. Additional events are generated for human input
devices such as keyboards, mice, touchscreens and joysticks.
And finally, events are also generated for Android specific
changes to screen density and permissions.

See the _Dependencies_ section for details on the
VKKNativeActivity interface required for Android.

See the _Known Issues_ section for joystick events.

Dependencies
============

The engine uses libcc for logging, memory tracking, data
structures (e.g. lists/maps), threading constructs and
vector math operations.

	https://github.com/jeffboody/libcc

The engine uses a resource file stored in the pak file
format for shader modules to simplify cross platform
resource management.

	https://github.com/jeffboody/libpak

On Android, the app must extend the VKKNativeActivity define
the app native library and pass platform events to the
platform independent interface.

	https://github.com/jeffboody/vkk-java

Resource File
=============

The app should provide a resource file which may contain
shaders, images, icons, fonts and any other resources.
The resource file is named resource.pak and can be
generated by the pak tool found at libpak/pak. The
resource file should be installed at the engine resource
path.

The following example shows how to generate a resource
file.

	pak -c resource.pak readme.txt
	pak -a resource.pak shader_vert.spv
	pak -a resource.pak shader_frag.spv

See the _Engine_ section for the platform specific resource
path location.

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
