cmake_minimum_required(VERSION 3.4.1)

# Compiler options
add_compile_options(-Wall -DVK_USE_PLATFORM_ANDROID_KHR)

if(VKK_USE_VKUI)
    set(SOURCE_VKUI
        vkui/vkui_bulletbox.c
        vkui/vkui_checkbox.c
        vkui/vkui_font.c
        vkui/vkui_hline.c
        vkui/vkui_layer.c
        vkui/vkui_listbox.c
        vkui/vkui_screen.c
        vkui/vkui_sprite.c
        vkui/vkui_radiobox.c
        vkui/vkui_radiolist.c
        vkui/vkui_textbox.c
        vkui/vkui_text.c
        vkui/vkui_tricolor.c
        vkui/vkui_viewbox.c
        vkui/vkui_widget.c)
endif()

# native_app_glue
add_library(native_app_glue
            STATIC

            # Source
            ${ANDROID_NDK}/sources/third_party/vulkan/src/common/vulkan_wrapper.cpp
            ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

# Submodule library
add_library(vkk
            STATIC

            # Source
            vkk_android.c
            vkk_buffer.c
            vkk_commandBuffer.c
            vkk_defaultRenderer.c
            vkk_engine.c
            vkk_graphicsPipeline.c
            vkk_image.c
            vkk_imageRenderer.c
            vkk_imageUploader.c
            vkk_memory.c
            vkk_memoryChunk.c
            vkk_memoryManager.c
            vkk_memoryPool.c
            vkk_pipelineLayout.c
            vkk_renderer.c
            vkk_secondaryRenderer.c
            vkk_uniformSet.c
            vkk_uniformSetFactory.c
            vkk_util.c
            ${SOURCE_VKUI})

# Includes
target_include_directories(vkk
                           PRIVATE

                           # PATH
                           ${ANDROID_NDK}/sources/third_party/vulkan/src/include/vulkan
                           ${ANDROID_NDK}/sources/third_party/vulkan/src/common
                           ${ANDROID_NDK}/sources/android/native_app_glue)

# Linking
target_link_libraries(vkk

                      # Submodule libraries
                      cc
                      pak
                      texgz
                      xmlstream

                      # NDK libraries
                      log
                      native_app_glue
                      z)
