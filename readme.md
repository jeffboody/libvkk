VKK Library
===========

The Vulkan Kit (VKK) is a high level graphics API layered
on the low level Vulkan graphics API.

Vulkan is very different from the former OpenGL graphics
standard due to its low level API. The low level API
provides developers with an unprecedented capability to
optimze algorithms for each apps workload. Optimized
algorithms can greatly impact an apps performance, memory
usage, rendering quality and portability. The low level
API also allows hardware vendors to provide a much simpler
driver thats easier to test resulting in fewer bugs and
better consistency across devices. However, an important
tradeoff of the low level API is that it requires
developers to implement many algorithms that were
historically included by the OpenGL graphics driver.
Example algorithms include memory management, internal
synchronization, data conversion, resource updates and
command pooling. These algoritms can be time consuming to
develop and may require significant expertise to implement
properly.

VKK simplifies development of graphics apps by exposing a
high level graphics API which includes support for an
important subset of commonly used Vulkan design concepts
and underlying graphics algorithms. These include
multithreaded rendering, graphics pipelines, secondary
command buffers and low overhead resource updates. The
underlying Vulkan implementation is abstracted allowing
the developer to focus on implementing graphics concepts.

The following sections describe the core, platform, vector
graphics and user interface APIs.

VKK Core
--------

The core graphics features exposed by VKK include.

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

See [VKK Core](core/readme.md) for more details.

VKK Platform
------------

The platform features exposed by VKK include.

* Single header file (vkk\_platform.h)
* Cross-platform support for Linux and Android
* Application lifecycle management
* Screen density events for UI scaling
* Keyboard, touch and joystick events
* Android sensor events (accelerometer, magnetometer, gyroscope, and GPS)
* Android permissions (location and storage)

See [VKK Platform](platform/readme.md) for more details.

VKK VG (Vector Graphics)
------------------------

The vector graphics features exposed by VKK VG include.

* Single header file (vkk\_vg.h)
* Line and polygon primitives
* Primitive geometry builders
* Context managed rendering state

See [VKK VG](vg/readme.md) for more details.

VKK UI (User Interface)
-----------------------

The user interface widgets exposed by VKK UI include.

* actionBar
* bulletbox
* checkbox
* hline
* infoPanel
* layer
* listbox
* radiolist
* sprite
* statusBar
* text
* textbox
* widget
* window

See [VKK UI](ui/readme.md) for more details.

Dependencies
============

The VKK library uses
[libcc](https://github.com/jeffboody/libcc)
for logging, memory tracking, data structures (e.g.
lists/maps), threading constructs and vector math
operations. Note that you may find it useful to use the
cc\_mat4f\_perspective() and cc\_mat4f\_orthoVK() functions
to prepare the perspective projection and orthographic
projection matrices since viewport origin has flipped from
OpenGL to Vulkan.

Resource file support is provided by
[libbfs](https://github.com/jeffboody/libbfs) and
[SQLite](https://github.com/jeffboody/libsqlite3).

Image support is provided by
[texgz](https://github.com/jeffboody/texgz),
[lodepng](https://github.com/lvandeve/lodepng) and
[myjpeg](https://github.com/jeffboody/jpeg).

XML support is provided by
[xmlstream](https://github.com/jeffboody/libxmlstream) and
[libexpat](https://github.com/jeffboody/libexpat).

Tesselation support for VKK VG is provided by
[libtess2](https://github.com/jeffboody/libtess2).

Android platform support is provided by
[vkk-java](https://github.com/jeffboody/vkk-java).

Resource File
=============

The app must provide a resource file which may contain
shaders, images, icons, fonts and any other resources.
The VKK VG/UI modules require special resources to be
packed into the app resource file. The resource file is
named resource.bfs and can be generated by the BFS tool
found at libbfs/bfs.

The resource file must be installed at the engine resource
path (where APP_DIR is defined by VKK_PLATFORM_INFO).

	Android: app/src/main/assets/resource.bfs
	Linux:   ~/APP_DIR/resource.bfs

The following
[script](https://github.com/jeffboody/gearsvk/blob/main/build-resource.sh)
is an example for how to generate a resource file.

Example Apps
============

Gears VK
--------

[Gears VK](https://github.com/jeffboody/gearsvk)
is a port of the famous "gears" demo to VKK.

![Gears VK](doc/gearsvk.jpg?raw=true "Gears VK")

Precomputed Atmospheric Scattering
----------------------------------

[Precomputed Atmospheric Scattering](https://github.com/jeffboody/precomputed_atmospheric_scattering/tree/master/atmosphere/demovk)
is another port of a sample app to VKK.

![Precomputed Atmospheric Scattering](doc/sky2.jpg?raw=true "Precomputed Atmospheric Scattering")

Trekking Maps
-------------

[Trekking Maps](https://www.3dgesoftware.com/)
is a free commercial app which is available on the Google
Play Store.

![Trekking Maps](doc/trekking-maps.jpg?raw=true "Trekking Maps")

VKK Modules
-----------

The
[VKK VG](https://github.com/jeffboody/libvkk/vg) and
[VKK UI](https://github.com/jeffboody/libvkk/ui) modules
are also built on top of the VKK Core library.

License
=======

The VKK library was developed by
[Jeff Boody](mailto:jeffboody@gmail.com)
under The MIT License.

	Copyright (c) 2019 Jeff Boody

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"),
	to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense,
	and/or sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included
	in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.
