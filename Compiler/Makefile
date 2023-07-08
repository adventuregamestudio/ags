INCDIR = ../Common .
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
	../Common/script/cc_common.cpp \
	../Common/script/cc_script.cpp \
	../Common/util/bufferedstream.cpp \
	../Common/util/cmdlineopts.cpp \
	../Common/util/datastream.cpp \
	../Common/util/directory.cpp \
	../Common/util/file.cpp \
	../Common/util/path.cpp \
	../Common/util/filestream.cpp \
	../Common/util/stdio_compat.c \
	../Common/util/stream.cpp \
	../Common/util/string.cpp \
	../Common/util/string_compat.c \
	../Common/util/string_utils.cpp \
	../Common/util/textstreamreader.cpp \
	../Common/util/version.cpp

COMPILER_OBJS = \
	compiler.cpp \
	fmem.cpp \
	script/cc_compiledscript.cpp \
	script/cc_internallist.cpp \
	script/cc_macrotable.cpp \
	script/cc_symboltable.cpp \
	script/cc_treemap.cpp \
	script/cs_compiler.cpp \
	script/cs_parser.cpp \
	script/cs_parser_common.cpp \
	preproc/preprocessor.cpp

OBJS := main.cpp \
	$(COMMON_OBJS) \
	$(COMPILER_OBJS)

OBJS := $(OBJS:.cpp=.o)
OBJS := $(OBJS:.c=.o)

DEPFILES = $(OBJS:.o=.d)

-include config.mak

.PHONY: printflags clean install uninstall rebuild

all: printflags agscc

agscc: $(OBJS) 
	@echo "Linking..."
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(LIBS)

debug: CXXFLAGS += -UNDEBUG -D_DEBUG -Og -g -pg
debug: CFLAGS   += -UNDEBUG -D_DEBUG -Og -g -pg
debug: LDFLAGS  += -pg
debug: printflags agscc

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
	$(CMD_PREFIX) rm -f agscc $(OBJS) $(DEPFILES)

install: agscc
	mkdir -p $(PREFIX)/bin
	cp -t $(PREFIX)/bin agscc

uninstall:
	rm -f $(PREFIX)/bin/agscc
