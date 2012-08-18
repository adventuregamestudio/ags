UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
include Makefile-defs.osx
else
include Makefile-defs.linux
endif

BASE_subdirs = \
ac \
ac/dynobj \
debug \
device \
font \
gfx \
gui \
main \
media \
media/audio \
media/video \
platform/base \
plugin \
resource \
script \
setup

BASE_cpp_sources := $(wildcard $(addsuffix /*.cpp,$(BASE_subdirs)))
BASE := $(patsubst %.cpp,%.o,$(BASE_cpp_sources))

COMMON_subdirs = \
../Common/ac \
../Common/ac/dynobj \
../Common/font \
../Common/gui \
../Common/platform/base \
../Common/script \
../Common/util

COMMON_cpp_sources := $(wildcard $(addsuffix /*.cpp,$(COMMON_subdirs)))
COMMON := $(patsubst %.cpp,%.o,$(COMMON_cpp_sources)) $(COMMON_PLATFORM)

PLUGINS = ../Plugins/AGSflashlight/agsflashlight.o ../Plugins/agsblend/AGSBlend.o ../Plugins/ags_snowrain/ags_snowrain.o

HQ2X3X = libsrc/hq2x/hq2x3x.o

ALFONT = libsrc/alfont-2.0.9/alfont.o

ALMP3 = libsrc/almp3/almp3.o libsrc/almp3-2.0.5/decoder/common.o libsrc/almp3-2.0.5/decoder/dct64_i386.o libsrc/almp3-2.0.5/decoder/decode_i386.o libsrc/almp3-2.0.5/decoder/interface.o libsrc/almp3-2.0.5/decoder/layer2.o libsrc/almp3-2.0.5/decoder/layer3.o libsrc/almp3-2.0.5/decoder/tabinit.o

ALOGG = libsrc/alogg/alogg.o

APEG = libsrc/apeg-1.2.1/adisplay.o libsrc/apeg-1.2.1/getbits.o libsrc/apeg-1.2.1/getblk.o libsrc/apeg-1.2.1/gethdr.o libsrc/apeg-1.2.1/getpic.o libsrc/apeg-1.2.1/idct.o libsrc/apeg-1.2.1/motion.o libsrc/apeg-1.2.1/mpeg1dec.o libsrc/apeg-1.2.1/ogg.o libsrc/apeg-1.2.1/recon.o libsrc/apeg-1.2.1/audio/apegcommon.o libsrc/apeg-1.2.1/audio/aaudio.o libsrc/apeg-1.2.1/audio/dct64.o libsrc/apeg-1.2.1/audio/decode_1to1.o libsrc/apeg-1.2.1/audio/decode_2to1.o libsrc/apeg-1.2.1/audio/decode_4to1.o libsrc/apeg-1.2.1/audio/layer1.o libsrc/apeg-1.2.1/audio/layer2.o libsrc/apeg-1.2.1/audio/layer3.o libsrc/apeg-1.2.1/audio/mpg123.o libsrc/apeg-1.2.1/audio/readers.o libsrc/apeg-1.2.1/audio/tabinit.o libsrc/apeg-1.2.1/audio/vbrhead.o

AASTR = libsrc/aastr-0.1.1/AAROT.o libsrc/aastr-0.1.1/aastr.o libsrc/aastr-0.1.1/aautil.o

CDA = libsrc/libcda-0.5/linux.o

OBJS = $(ALFONT) $(ALMP3) $(ALOGG) $(APEG) $(AASTR) $(HQ2X3X) $(CDA) $(PLUGINS) $(BASE) $(BASE_PLATFORM)



all: ags

ags: $(OBJS) common.a 
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

common.a: $(COMMON)
	$(AR) rcs $@ $^
%.o: %.m
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.mm
	$(CXX) $(CXXFLAGS) -c -o $@ $<

.PHONY: clean install uninstall

clean:
	rm -f ags common.a $(OBJS) $(COMMON)

install: ags
	mkdir -p $(PREFIX)/bin
	cp -t $(PREFIX)/bin ags

uninstall:
	rm -f $(PREFIX)/bin/ags
