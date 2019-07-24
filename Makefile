TARGET  = libvkk.a
CLASSES = \
	vkk_defaultRenderer \
	vkk_engine          \
	vkk_renderer        \
	vkk_util
SOURCE  = $(CLASSES:%=%.c)
OBJECTS = $(SOURCE:.c=.o)
HFILES  = vkk.h $(CLASSES:%=%.h)
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
