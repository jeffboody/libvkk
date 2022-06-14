TARGET  = libvkk.a
CLASSES = \
	platform/vkk_linux           \
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
ifeq ($(VKK_USE_VKUI),1)
	CLASSES += \
		vkui/vkui_actionBar \
		vkui/vkui_bulletbox \
		vkui/vkui_checkbox  \
		vkui/vkui_font      \
		vkui/vkui_hline     \
		vkui/vkui_infoPanel \
		vkui/vkui_layer     \
		vkui/vkui_listbox   \
		vkui/vkui_screen    \
		vkui/vkui_sprite    \
		vkui/vkui_statusBar \
		vkui/vkui_radiobox  \
		vkui/vkui_radiolist \
		vkui/vkui_textbox   \
		vkui/vkui_text      \
		vkui/vkui_tricolor  \
		vkui/vkui_widget    \
		vkui/vkui_window
endif
SOURCE  = $(CLASSES:%=%.c)
OBJECTS = $(SOURCE:.c=.o)
HFILES  = vkk.h $(CLASSES:%=%.h)
ifeq ($(VKK_USE_VKUI),1)
	HFILES += \
		vkui.h
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
