cmake_minimum_required(VERSION 3.4.1)

# Compiler options
add_compile_options(-Wall)

# Submodule library
add_library(vkk
            STATIC

            # Source
            vkk_defaultRenderer.c
            vkk_engine.c
            vkk_offscreenRenderer.c
            vkk_renderer.c
            vkk_util.c)

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

                      # NDK libraries
                      log)
