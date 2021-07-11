INCDIR = ../../Common ../../Tools ../../libsrc/tinyxml2
LIBDIR =

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
LDFLAGS  += -rdynamic -Wl,--as-needed $(addprefix -L,$(LIBDIR))
CFLAGS   += -Werror=implicit-function-declaration

COMMON_OBJS = \
	../../Common/debug/debugmanager.cpp \
	../../Common/util/bufferedstream.cpp \
	../../Common/util/datastream.cpp \
	../../Common/util/file.cpp \
	../../Common/util/filestream.cpp \
	../../Common/util/memorystream.cpp \
	../../Common/util/path.cpp \
	../../Common/util/stdio_compat.c \
	../../Common/util/stream.cpp \
	../../Common/util/string.cpp \
	../../Common/util/string_compat.c \
	../../Common/util/string_utils.cpp \
	../../Common/util/textstreamreader.cpp

TOOL_OBJS = \
	../../Tools/data/agfreader.cpp \
	../../Tools/data/dialogscriptconv.cpp \
	../../Tools/data/script_utils.cpp

TINYXML2 = \
	../../libsrc/tinyxml2/tinyxml2.cpp

OBJS := main.cpp \
	$(COMMON_OBJS) \
	$(TOOL_OBJS) \
	$(TINYXML2)
OBJS := $(OBJS:.cpp=.o)
OBJS := $(OBJS:.c=.o)

DEPFILES = $(OBJS:.o=.d)

-include config.mak

.PHONY: printflags clean install uninstall rebuild

all: printflags agf2dlgasc

agf2dlgasc: $(OBJS) 
	@echo "Linking..."
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(LIBS)

debug: CXXFLAGS += -UNDEBUG -D_DEBUG -Og -g -pg
debug: CFLAGS   += -UNDEBUG -D_DEBUG -Og -g -pg
debug: LDFLAGS  += -pg
debug: printflags agf2dlgasc

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
	$(CMD_PREFIX) rm -f agf2dlgasc $(OBJS) $(DEPFILES)

install: agf2dlgasc
	mkdir -p $(PREFIX)/bin
	cp -t $(PREFIX)/bin agf2dlgasc

uninstall:
	rm -f $(PREFIX)/bin/agf2dlgasc
