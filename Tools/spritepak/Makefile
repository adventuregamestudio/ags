INCDIR = ../../Common ../../libsrc/miniz ../../libsrc/allegro/include
LIBDIR =

CFLAGS := -O2 -g \
	-fsigned-char -fno-strict-aliasing -fwrapv \
	-Wunused-result \
	-Wno-unused-value  \
	-Werror=write-strings -Werror=format -Werror=format-security \
	-DNDEBUG \
	-D_FILE_OFFSET_BITS=64 -DRTLD_NEXT \
	-DALLEGRO_STATICLINK
	$(CFLAGS)

CXXFLAGS := -std=c++11 -Werror=delete-non-virtual-dtor $(CXXFLAGS)

PREFIX ?= /usr/local
CC ?= gcc
CXX ?= g++
AR ?= ar
CFLAGS   += $(addprefix -I,$(INCDIR))
CXXFLAGS += $(CFLAGS)
ASFLAGS  += $(CFLAGS)
LDFLAGS  += -rdynamic -Wl,--as-needed $(addprefix -L,$(LIBDIR))
CFLAGS   += -Werror=implicit-function-declaration

COMMON_OBJS = \
	../../Common/ac/spritefile.cpp \
	../../Common/gfx/bitmapdata.cpp \
	../../Common/gfx/image_file.cpp \
	../../Common/util/bufferedstream.cpp \
	../../Common/util/cmdlineopts.cpp \
	../../Common/util/compress.cpp \
	../../Common/util/directory.cpp \
	../../Common/util/file.cpp \
	../../Common/util/filestream.cpp \
	../../Common/util/lzw.cpp \
	../../Common/util/memorystream.cpp \
	../../Common/util/path.cpp \
	../../Common/util/stdio_compat.c \
	../../Common/util/stream.cpp \
	../../Common/util/string.cpp \
	../../Common/util/string_compat.c \
	../../Common/util/string_utils.cpp

MINIMAL_ALLEGRO_OBJS = \
	../../libsrc/allegro/src/color.c

MINIZ_OBJS = \
	../../libsrc/miniz/miniz.c

OBJS := main.cpp \
	commands.cpp \
	$(COMMON_OBJS) \
	$(MINIZ_OBJS)
OBJS := $(OBJS:.cpp=.o)
OBJS := $(OBJS:.c=.o)

DEPFILES = $(OBJS:.o=.d)

-include config.mak

.PHONY: printflags clean install uninstall rebuild

all: printflags spritepak

spritepak: $(OBJS) 
	@echo "Linking..."
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(LIBS)

debug: CXXFLAGS += -UNDEBUG -D_DEBUG -Og -g -pg
debug: CFLAGS   += -UNDEBUG -D_DEBUG -Og -g -pg
debug: LDFLAGS  += -pg
debug: printflags spritepak

-include $(DEPFILES)

%.o: %.c
	@echo $@
	$(CMD_PREFIX) $(CC) $(CFLAGS) -MD -c -o $@ $<

%.o: %.cpp
	@echo $@
	$(CMD_PREFIX) $(CXX) $(CXXFLAGS) -MD -c -o $@ $<

printflags:
	@echo "CFLAGS =" $(CFLAGS) "\n"
	@echo "CXXFLAGS =" $(CXXFLAGS) "\n"
	@echo "LDFLAGS =" $(LDFLAGS) "\n"
	@echo "LIBS =" $(LIBS) "\n"

rebuild: clean all

clean:
	@echo "Cleaning..."
	$(CMD_PREFIX) rm -f spritepak $(OBJS) $(DEPFILES)

install: spritepak
	mkdir -p $(PREFIX)/bin
	cp -t $(PREFIX)/bin spritepak

uninstall:
	rm -f $(PREFIX)/bin/spritepak
