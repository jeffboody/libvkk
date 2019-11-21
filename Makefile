TARGET  = libvkk.a
CLASSES = \
	vkk_buffer            \
	vkk_defaultRenderer   \
	vkk_engine            \
	vkk_graphicsPipeline  \
	vkk_image             \
	vkk_offscreenRenderer \
	vkk_pipelineLayout    \
	vkk_renderer          \
	vkk_sampler           \
	vkk_uniformSet        \
	vkk_uniformSetFactory \
	vkk_util
ifeq ($(VKK_USE_VKUI),1)
	CLASSES += \
		vkui/vkui_bulletbox \
		vkui/vkui_checkbox  \
		vkui/vkui_font      \
		vkui/vkui_hline     \
		vkui/vkui_layer     \
		vkui/vkui_listbox   \
		vkui/vkui_screen    \
		vkui/vkui_sprite    \
		vkui/vkui_radiobox  \
		vkui/vkui_radiolist \
		vkui/vkui_textbox   \
		vkui/vkui_text      \
		vkui/vkui_tricolor  \
		vkui/vkui_viewbox   \
		vkui/vkui_widget
endif
SOURCE  = $(CLASSES:%=%.c)
OBJECTS = $(SOURCE:.c=.o)
HFILES  = vkk.h $(CLASSES:%=%.h)
ifeq ($(VKK_USE_VKUI),1)
	HFILES += \
		vkui/vkui_key.h \
		vkui/vkui.h
endif
OPT     = -O2 -Wall
CFLAGS   = $(OPT) -I$(VULKAN_SDK)/include `sdl2-config --cflags`
LDFLAGS  = -L$(VULKAN_SDK)/lib -lvulkan `sdl2-config --libs` -lm
AR      = ar

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(AR) rcs $@ $(OBJECTS)

clean:
	rm -f $(OBJECTS) *~ \#*\# $(TARGET)

$(OBJECTS): $(HFILES)
