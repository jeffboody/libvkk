export CC_USE_MATH = 1
export CC_USE_RNG  = 1

TARGET   = xsq-test
CLASSES  = xsq_test
SOURCE   = $(TARGET).c $(CLASSES:%=%.c)
OBJECTS  = $(TARGET).o $(CLASSES:%=%.o)
HFILES   = $(CLASSES:%=%.h)
OPT      = -O2 -Wall -Wno-format-truncation
CFLAGS   = \
	$(OPT) -I.             \
	`sdl2-config --cflags` \
	-I$(VULKAN_SDK)/include
LDFLAGS  = -Llibvkk -lvkk -Llibbfs -lbfs -Llibcc -lcc -Llibsqlite3 -lsqlite3 -L$(VULKAN_SDK)/lib -lvulkan -L/usr/lib `sdl2-config --libs` -ldl -lpthread -lz -lm
CCC      = gcc

all: $(TARGET)

$(TARGET): $(OBJECTS) libvkk libbfs libcc libsqlite3
	$(CCC) $(OPT) $(OBJECTS) -o $@ $(LDFLAGS)

.PHONY: libvkk libbfs libcc libsqlite3

libvkk:
	$(MAKE) -C libvkk

libbfs:
	$(MAKE) -C libbfs

libcc:
	$(MAKE) -C libcc

libsqlite3:
	$(MAKE) -C libsqlite3

clean:
	rm -f $(OBJECTS) *~ \#*\# $(TARGET)
	$(MAKE) -C libvkk clean
	$(MAKE) -C libbfs clean
	$(MAKE) -C libcc clean
	$(MAKE) -C libsqlite3 clean
	rm libvkk libbfs libcc pcg-c-basic libsqlite3

$(OBJECTS): $(HFILES)
