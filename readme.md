About
=====

The Vulkan Kit (VKK) is a graphics library that exposes a
simplified graphics pipeline layered on the the Vulkan
graphics library. The features exposed by the Vulkan Kit
are limited to the subset necessary to port my own OpenGL
apps to Vulkan. These features include the following.

* Cross platform development with Linux and Android
* Rendering to native window surface
* Offscreen rendering (e.g. render to texture)
* Multithreaded rendering, creation and destruction
* Graphics pipeline with vertex and fragment shader stages
* Shader support for uniform buffers and samplers
* Uniform buffers may be updated dynamically
* 2D texturing with mipmapping and linear filtering
* Triangles are the only primitive supported
* Transparency, depth clearing, viewport and scissors
* Single header file interface (vkk.h)

The engine uses a resource file stored in the pak file
format for shader modules to simplify cross platform
resource management.

	https://github.com/jeffboody/libpak

The texgz library is useful for importing/exporting images.

	https://github.com/jeffboody/texgz

See the master-vkk and master-vkk-test branches of the
gearsvk app for an example which uses the Vulkan Kit.

	https://github.com/jeffboody/gearsvk

Send questions or comments to Jeff Boody.

	jeffboody@gmail.com
