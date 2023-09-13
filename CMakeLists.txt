cmake_minimum_required(VERSION 3.4.1)

# Compiler options
add_compile_options(-Wall -DVK_USE_PLATFORM_ANDROID_KHR)

if(VKK_ENGINE_DISABLE_MSAA)
	add_compile_options(-DVKK_ENGINE_DISABLE_MSAA)
endif()

if(VKK_USE_UI)
    set(SOURCE_UI
        ui/vkk_uiActionBar.c
        ui/vkk_uiActionButton.c
        ui/vkk_uiActionPopup.c
        ui/vkk_uiBulletBox.c
        ui/vkk_uiCheckBox.c
        ui/vkk_uiFilePicker.c
        ui/vkk_uiFont.c
        ui/vkk_uiGraphicsBox.c
        ui/vkk_uiInfoPanel.c
        ui/vkk_uiInputWindow.c
        ui/vkk_uiLayer.c
        ui/vkk_uiListBox.c
        ui/vkk_uiRadioBox.c
        ui/vkk_uiRadioList.c
        ui/vkk_uiScreen.c
        ui/vkk_uiSeparator.c
        ui/vkk_uiSprite.c
        ui/vkk_uiStatusBar.c
        ui/vkk_uiTextBox.c
        ui/vkk_uiText.c
        ui/vkk_uiTricolor.c
        ui/vkk_uiWidget.c
        ui/vkk_uiWindow.c)
endif()

if(VKK_USE_VG)
    set(SOURCE_VG
        vg/vkk_vgBuffer.c
        vg/vkk_vgLineBuilder.c
        vg/vkk_vgLine.c
        vg/vkk_vgPolygonBuilder.c
        vg/vkk_vgPolygonIdx.c
        vg/vkk_vgPolygon.c
        vg/vkk_vgRenderer.c)
    set(LINK_VG
        tess2)
endif()

# native_app_glue
add_library(native_app_glue
            STATIC

            # Source
            ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

# Submodule library
add_library(vkk
            STATIC

            # Source
            platform/vkk_platformAndroid.c
            core/vkk_buffer.c
            core/vkk_commandBuffer.c
            core/vkk_computePipeline.c
            core/vkk_compute.c
            core/vkk_defaultRenderer.c
            core/vkk_engine.c
            core/vkk_graphicsPipeline.c
            core/vkk_image.c
            core/vkk_imageRenderer.c
            core/vkk_imageStreamRenderer.c
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
            core/vkk_xferManager.c
            ${SOURCE_UI}
            ${SOURCE_VG})

# Includes
target_include_directories(vkk
                           PRIVATE

                           # PATH
                           ${ANDROID_NDK}/sources/third_party/vulkan/src/include
                           ${ANDROID_NDK}/sources/android/native_app_glue)

# Linking
target_link_libraries(vkk

                      # Submodule libraries
                      bfs
                      cc
                      texgz
                      ${LINK_VG}
                      xmlstream

                      # NDK libraries
                      log
                      native_app_glue
                      vulkan
                      z)
