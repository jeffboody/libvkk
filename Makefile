TARGET  = libvkk.a
CLASSES = \
	vkk_defaultRenderer   \
	vkk_engine            \
	vkk_offscreenRenderer \
	vkk_renderer          \
	vkk_util
ifeq ($(VKK_USE_VKUI),1)
	CLASSES += \
		vkui/vkui_bulletbox \
		vkui/vkui_checkbox  \
		vkui/vkui_font      \
		vkui/vkui_hline     \
		vkui/vkui_layer     \
		vkui/vkui_screen    \
		vkui/vkui_sprite    \
		vkui/vkui_text      \
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
