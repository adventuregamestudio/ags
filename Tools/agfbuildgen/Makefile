INCDIR = ../../Common ../../Tools ../../Tools/agfbuildgen ../../libsrc/allegro/include ../../libsrc/tinyxml2
LIBDIR =

LD_VERSION := $(shell $(LD) --version 2>/dev/null)

CFLAGS := -O2 -g \
	-fsigned-char -fno-strict-aliasing -fwrapv \
	-Wunused-result \
	-Wno-unused-value  \
	-Werror=write-strings -Werror=format -Werror=format-security \
	-DNDEBUG \
	-D_FILE_OFFSET_BITS=64 -DRTLD_NEXT \
	$(CFLAGS)

CXXFLAGS := -std=c++11 -Werror=delete-non-virtual-dtor $(CXXFLAGS)

PREFIX ?= /usr/local
CC ?= gcc
CXX ?= g++
AR ?= ar
CFLAGS   += $(addprefix -I,$(INCDIR))
CXXFLAGS += $(CFLAGS)
ASFLAGS  += $(CFLAGS)
CFLAGS   += -Werror=implicit-function-declaration

ifneq (,$(findstring GNU ld,$(LD_VERSION)))
    LDFLAGS += -rdynamic -Wl,--as-needed
endif
LDFLAGS  += $(addprefix -L,$(LIBDIR))

COMMON_OBJS = \
	../../Common/ac/characterinfo.cpp \
	../../Common/ac/inventoryiteminfo.cpp \
	../../Common/ac/mousecursor.cpp \
	../../Common/ac/wordsdictionary.cpp \
	../../Common/debug/debugmanager.cpp \
	../../Common/util/bufferedstream.cpp \
	../../Common/util/cmdlineopts.cpp \
	../../Common/util/directory.cpp \
	../../Common/util/file.cpp \
	../../Common/util/filestream.cpp \
	../../Common/util/path.cpp \
	../../Common/util/stdio_compat.c \
	../../Common/util/stream.cpp \
	../../Common/util/string.cpp \
	../../Common/util/string_compat.c \
	../../Common/util/string_utils.cpp \
	../../Common/util/textstreamreader.cpp \
	../../Common/util/textstreamwriter.cpp

TOOL_OBJS = \
	../../Tools/data/agfreader.cpp

MINIMAL_ALLEGRO_OBJS = \
	../../libsrc/allegro/src/allegro.c \
	../../libsrc/allegro/src/file.c \
	../../libsrc/allegro/src/unicode.c \
	../../libsrc/allegro/src/unix/ufile.c

TINYXML2 = \
	../../libsrc/tinyxml2/tinyxml2.cpp

AGFBUILDGEN_OBJS = \
	main.cpp \
	text_writer.cpp \
	ninja_writer.cpp \
	ninja_generator.cpp \
	makefile_writer.cpp \
	makefile_generator.cpp


OBJS := $(AGFBUILDGEN_OBJS) \
	$(COMMON_OBJS) \
	$(TOOL_OBJS) \
	$(MINIMAL_ALLEGRO_OBJS) \
	$(TINYXML2)
OBJS := $(OBJS:.cpp=.o)
OBJS := $(OBJS:.c=.o)

DEPFILES = $(OBJS:.o=.d)

-include config.mak

.PHONY: printflags clean install uninstall rebuild

all: printflags agfbuildgen

agfbuildgen: $(OBJS) 
	@echo "Linking..."
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(LIBS)

debug: CXXFLAGS += -UNDEBUG -D_DEBUG -Og -g -pg
debug: CFLAGS   += -UNDEBUG -D_DEBUG -Og -g -pg
debug: LDFLAGS  += -pg
debug: printflags agfbuildgen

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
	$(CMD_PREFIX) rm -f agfbuildgen $(OBJS) $(DEPFILES)

install: agfbuildgen
	mkdir -p $(PREFIX)/bin
	cp -t $(PREFIX)/bin agfbuildgen

uninstall:
	rm -f $(PREFIX)/bin/agfbuildgen
