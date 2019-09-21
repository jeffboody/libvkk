cmake_minimum_required(VERSION 3.4.1)

# Compiler options
add_compile_options(-Wall)

if(VKK_USE_VKUI)
    set(SOURCE_VKUI
        vkui/vkui_bulletbox.c
        vkui/vkui_font.c
        vkui/vkui_screen.c
        vkui/vkui_sprite.c
        vkui/vkui_text.c
        vkui/vkui_widget.c)
endif()

# Submodule library
add_library(vkk
            STATIC

            # Source
            vkk_defaultRenderer.c
            vkk_engine.c
            vkk_offscreenRenderer.c
            vkk_renderer.c
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
                      z)
