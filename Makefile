TARGET  = libvkk.a
CLASSES = \
	platform/vkk_platformLinux   \
	core/vkk_buffer              \
	core/vkk_commandBuffer       \
	core/vkk_defaultRenderer     \
	core/vkk_engine              \
	core/vkk_graphicsPipeline    \
	core/vkk_image               \
	core/vkk_imageRenderer       \
	core/vkk_imageStreamRenderer \
	core/vkk_imageUploader       \
	core/vkk_memory              \
	core/vkk_memoryChunk         \
	core/vkk_memoryManager       \
	core/vkk_memoryPool          \
	core/vkk_pipelineLayout      \
	core/vkk_renderer            \
	core/vkk_secondaryRenderer   \
	core/vkk_uniformSet          \
	core/vkk_uniformSetFactory   \
	core/vkk_util
ifeq ($(VKK_USE_UI),1)
	CLASSES += \
		ui/vkk_uiActionBar \
		ui/vkk_uiBulletbox \
		ui/vkk_uiCheckbox  \
		ui/vkk_uiFont      \
		ui/vkk_uiHline     \
		ui/vkk_uiInfoPanel \
		ui/vkk_uiLayer     \
		ui/vkk_uiListbox   \
		ui/vkk_uiScreen    \
		ui/vkk_uiSprite    \
		ui/vkk_uiStatusBar \
		ui/vkk_uiRadiobox  \
		ui/vkk_uiRadiolist \
		ui/vkk_uiTextbox   \
		ui/vkk_uiText      \
		ui/vkk_uiTricolor  \
		ui/vkk_uiWidget    \
		ui/vkk_uiWindow
endif
ifeq ($(VKK_USE_VG),1)
	CLASSES += \
		vg/vkk_vgBuffer         \
		vg/vkk_vgContext        \
		vg/vkk_vgLineBuilder    \
		vg/vkk_vgLine           \
		vg/vkk_vgPolygonBuilder \
		vg/vkk_vgPolygonIdx     \
		vg/vkk_vgPolygon
endif
SOURCE  = $(CLASSES:%=%.c)
OBJECTS = $(SOURCE:.c=.o)
HFILES  = vkk.h $(CLASSES:%=%.h)
ifeq ($(VKK_USE_UI),1)
	HFILES += \
		vkk_ui.h
endif
ifeq ($(VKK_USE_VG),1)
	HFILES += \
		vkk_vg.h
endif
OPT     = -O2 -Wall -Wno-format-truncation
CFLAGS   = $(OPT) -I$(VULKAN_SDK)/include `sdl2-config --cflags`
LDFLAGS  = -L$(VULKAN_SDK)/lib -lvulkan `sdl2-config --libs` -lm
AR      = ar

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(AR) rcs $@ $(OBJECTS)

clean:
	rm -f $(OBJECTS) *~ \#*\# $(TARGET)

$(OBJECTS): $(HFILES)
