UNAME := $(shell uname)

ifeq ($(V), 1)
CMD_PREFIX :=
else
CMD_PREFIX := @
endif

include Makefile-objs

ifeq ($(UNAME), Darwin)
include Makefile-defs.osx
else
include Makefile-defs.linux
endif

ifneq ($(USE_MIDI_PATCH), 1)
AL_MIDI_PATCH :=
endif

OBJS_C_CPP_MM = $(ALFONT) $(ALMP3) $(ALOGG) $(APEG) $(AASTR) $(HQ2X3X) $(AL_MIDI_PATCH) $(CDA) $(ALDUMB) $(GLAD) $(PLUGINS) $(BASE) $(BASE_PLATFORM)
OBJS := $(OBJS_C_CPP_MM:.mm=.o)
OBJS := $(OBJS:.cpp=.o)
OBJS := $(OBJS:.c=.o)

OBJS_COMMON_CPP = $(COMMON)
OBJS_COMMON = $(OBJS_COMMON_CPP:.cpp=.o)
DEPFILES = $(OBJS:.o=.d) $(OBJS_COMMON:.o=.d)

-include config.mak

.PHONY: printflags clean install uninstall rebuild

all: printflags ags

ags: $(OBJS) common.a 
	@echo "Linking engine..."
	$(CMD_PREFIX) $(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(LIBS)

debug: CXXFLAGS += -UNDEBUG -D_DEBUG -Og -g -pg
debug: CFLAGS   += -UNDEBUG -D_DEBUG -Og -g -pg
debug: MMFLAGS  += -UNDEBUG -D_DEBUG -Og -g -pg
debug: LDFLAGS  += -pg
debug: printflags ags

common.a: $(OBJS_COMMON)
	@echo "Linking common library..."
	$(CMD_PREFIX) $(AR) rcs $@ $^

-include $(DEPFILES)

%.o: %.c
	@echo $@
	$(CMD_PREFIX) $(CC) $(CFLAGS) -MD -c -o $@ $<

%.o: %.mm
	@echo $@
	$(CMD_PREFIX) $(CC) $(MMFLAGS) -MD -c -o $@ $<

%.o: %.cpp
	@echo $@
	$(CMD_PREFIX) $(CXX) $(CXXFLAGS) -MD -c -o $@ $<

printflags:
	@echo "CFLAGS =" $(CFLAGS) "\n"
	@echo "CXXFLAGS =" $(CXXFLAGS) "\n"
	@echo "MMFLAGS =" $(MMFLAGS) "\n"
	@echo "LDFLAGS =" $(LDFLAGS) "\n"
	@echo "LIBS =" $(LIBS) "\n"

rebuild: clean all

clean:
	@echo "Cleaning..."
	$(CMD_PREFIX) rm -f ags common.a $(OBJS) $(OBJS_COMMON) $(DEPFILES)

install: ags
	mkdir -p $(PREFIX)/bin
	cp -t $(PREFIX)/bin ags

uninstall:
	rm -f $(PREFIX)/bin/ags
