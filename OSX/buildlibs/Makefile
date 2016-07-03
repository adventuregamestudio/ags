#!/bin/bash

LIBSRCDIR:=$(shell pwd)/../../libsrc
PATCHESDIR:=$(shell pwd)/../patches
PREFIX:=$(shell pwd)/opt
BUILD:=$(shell pwd)/build
PKG_CONFIG_PATH:=${PREFIX}/lib/pkgconfig
export PKG_CONFIG_PATH
PLAT_FLAGS:=-mmacosx-version-min=10.7 -arch i386 -arch x86_64
	
.PHONY: libs prepare lua freetype ogg vorbis theora allegro dumb install

libs: lua freetype ogg vorbis theora allegro dumb

install: libs
	rm -rf ../lib
	rm -rf ../include
	mkdir -p ../lib
	cp $(PREFIX)/lib/*.a ../lib
	cp -a $(PREFIX)/include ../include

prepare:
	cd $(LIBSRCDIR) && ./download.sh
	./checkenv.sh
	mkdir -p $(PREFIX)
	mkdir -p $(BUILD)

lua: prepare
	tar xf $(LIBSRCDIR)/lua-5.1.5.tar.gz -C $(BUILD)
	cd $(BUILD)/lua-5.1.5 && \
		patch -p1 < $(PATCHESDIR)/lua-5.1.5.patch && \
		make macosx INSTALL_TOP="$(PREFIX)" PLAT_LDFLAGS="$(PLAT_FLAGS)" PLAT_CFLAGS="$(PLAT_FLAGS)" && \
		make install INSTALL_TOP="$(PREFIX)"

freetype: prepare
	tar xf $(LIBSRCDIR)/freetype-2.4.12.tar.bz2 -C $(BUILD)
	cd $(BUILD)/freetype-2.4.12 && \
		./configure CFLAGS="$(PLAT_FLAGS)" LDFLAGS="$(PLAT_FLAGS)" \
		    --prefix=$(PREFIX) \
		    --enable-static --disable-shared \
		    --enable-biarch-config && \
		make && \
		make install 

ogg: prepare
	tar xf $(LIBSRCDIR)/libogg-1.3.2.tar.gz -C $(BUILD)
	cd $(BUILD)/libogg-1.3.2 && \
		./configure CFLAGS="$(PLAT_FLAGS)" LDFLAGS="$(PLAT_FLAGS)" \
		    --prefix=$(PREFIX) \
		    --enable-static --disable-shared && \
		make && \
		make install 

vorbis: prepare ogg
	tar xf $(LIBSRCDIR)/libvorbis-1.3.5.tar.gz -C $(BUILD)
	cd $(BUILD)/libvorbis-1.3.5 && \
		./configure CFLAGS="$(PLAT_FLAGS)" LDFLAGS="$(PLAT_FLAGS)" \
		    --prefix=$(PREFIX) \
		    --enable-static --disable-shared \
		    --disable-docs --disable-examples --disable-oggtest && \
		make && \
		make install 

theora: prepare ogg vorbis
	tar xf $(LIBSRCDIR)/libtheora-20160525-g50df933.tar.bz2 -C $(BUILD)
	cd $(BUILD)/libtheora-20160525-g50df933 && \
		./autogen.sh CFLAGS="$(PLAT_FLAGS)" LDFLAGS="$(PLAT_FLAGS)"  \
		    --prefix=$(PREFIX) \
		    --enable-static --disable-shared \
		    --with-ogg=$(PREFIX) --with-vorbis=$(PREFIX) \
		    --disable-examples \
		    --disable-asm \
		    --disable-vorbistest \
		    --disable-oggtest \
		    --disable-doc   \
		    --disable-spec  && \
		make && \
		make install

# - you can add `-DCMAKE_BUILD_TYPE=Debug` to the cmake options but remember it will produce the file `liballeg-debug.a`
# - to update patch: git diff 4.4.2 > ../ags/OSX/patches/allegro-4.4.2.patch
allegro: prepare
	rm -rf $(BUILD)/allegro-4.4.2
	tar xf $(LIBSRCDIR)/allegro-4.4.2.tar.gz -C $(BUILD)
	cd $(BUILD)/allegro-4.4.2 && \
		patch -p1 < $(PATCHESDIR)/allegro-4.4.2.patch && \
		mkdir -p build && \
		pushd build && \
		cmake \
		'-DCMAKE_OSX_ARCHITECTURES=x86_64;i386' \
		-D CMAKE_OSX_DEPLOYMENT_TARGET=10.7 \
		-D SHARED=off \
		-D WANT_ALLEGROGL=off -D WANT_LOADPNG=off -D WANT_LOGG=off -D WANT_JPGALLEG=off -D WANT_EXAMPLES=off -D WANT_TOOLS=off -D WANT_TESTS=off \
		-D WANT_DOCS=off \
		-D CMAKE_INSTALL_PREFIX=$(PREFIX) \
		-G Xcode .. && \
		xcodebuild -project ALLEGRO.xcodeproj -target ALL_BUILD -configuration Release && \
		xcodebuild -project ALLEGRO.xcodeproj -target install -configuration Release

define DUMB_CONF
include make/unix.inc
ALL_TARGETS := core core-headers
ALL_TARGETS += allegro allegro-headers
PREFIX := $(PREFIX)
PLAT_CFLAGS := -mmacosx-version-min=10.7 -arch i386 -arch x86_64 -I\$(PREFIX)/include
endef
export DUMB_CONF

dumb: prepare allegro
	tar xf $(LIBSRCDIR)/dumb-0.9.3.tar.gz -C $(BUILD)
	cd $(BUILD)/dumb-0.9.3 && \
		patch -p1 < $(PATCHESDIR)/dumb-0.9.3.patch && \
		echo "$$DUMB_CONF" > make/config.txt && \
		make && \
		make install
