UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
include Makefile-defs.osx
else
include Makefile-defs.linux
endif

include Makefile-objs
OBJS_C_CPP = $(ALFONT) $(ALMP3) $(ALOGG) $(APEG) $(AASTR) $(HQ2X3X) $(CDA) $(PLUGINS) $(BASE) $(BASE_PLATFORM)
OBJS_C = $(OBJS_C_CPP:.cpp=.o)
OBJS = $(OBJS_C:.c=.o)

OBJS_COMMON_CPP = $(COMMON) $(COMMON_PLATFORM)
OBJS_COMMON = $(OBJS_COMMON_CPP:.cpp=.o)

all: ags

ags: $(OBJS) common.a 
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

common.a: $(OBJS_COMMON)
	$(AR) rcs $@ $^
%.o: %.m
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.mm
	$(CXX) $(CXXFLAGS) -c -o $@ $<

.PHONY: clean install uninstall

clean:
	rm -f ags common.a $(OBJS) $(OBJS_COMMON)

install: ags
	mkdir -p $(PREFIX)/bin
	cp -t $(PREFIX)/bin ags

uninstall:
	rm -f $(PREFIX)/bin/ags
