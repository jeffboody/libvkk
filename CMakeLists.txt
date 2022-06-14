cmake_minimum_required(VERSION 3.4.1)

# Compiler options
add_compile_options(-Wall -DVK_USE_PLATFORM_ANDROID_KHR)

if(VKK_USE_VKUI)
    set(SOURCE_VKUI
        vkui/vkui_actionBar.c
        vkui/vkui_bulletbox.c
        vkui/vkui_checkbox.c
        vkui/vkui_font.c
        vkui/vkui_hline.c
        vkui/vkui_infoPanel.c
        vkui/vkui_layer.c
        vkui/vkui_listbox.c
        vkui/vkui_screen.c
        vkui/vkui_sprite.c
        vkui/vkui_statusBar.c
        vkui/vkui_radiobox.c
        vkui/vkui_radiolist.c
        vkui/vkui_textbox.c
        vkui/vkui_text.c
        vkui/vkui_tricolor.c
        vkui/vkui_widget.c
        vkui/vkui_window.c)
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
            platform/vkk_android.c
            core/vkk_buffer.c
            core/vkk_commandBuffer.c
            core/vkk_defaultRenderer.c
            core/vkk_engine.c
            core/vkk_graphicsPipeline.c
            core/vkk_image.c
            core/vkk_imageRenderer.c
            core/vkk_imageStreamRenderer.c
            core/vkk_imageUploader.c
            core/vkk_memory.c
            core/vkk_memoryChunk.c
            core/vkk_memoryManager.c
            core/vkk_memoryPool.c
            core/vkk_pipelineLayout.c
            core/vkk_renderer.c
            core/vkk_secondaryRenderer.c
            core/vkk_uniformSet.c
            core/vkk_uniformSetFactory.c
            core/vkk_util.c
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
                      texgz
                      xmlstream

                      # NDK libraries
                      log
                      native_app_glue
                      z)
