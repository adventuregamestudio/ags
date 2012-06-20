#!/bin/bash

# Set up build environment
source ../setenv.sh i686-android-linux

# Download and extract the library source
FILENAME=allegro-4.4.2
EXTENSION=tar.gz
wget -c http://downloads.sourceforge.net/project/alleg/allegro/4.4.2/$FILENAME.$EXTENSION -O ../$FILENAME.$EXTENSION
tar -zxf ../$FILENAME.$EXTENSION

# Build and install library
cd $FILENAME

# Platform independent patch
patch -p0 < ../../../patches/liballegro-4.4.2.patch

cmake . -G "Unix Makefiles" \
	-DWANT_TESTS=off \
	-DWANT_EXAMPLES=off \
	-DWANT_TOOLS=off \
	-DWANT_LOGG=off \
	-DWANT_ALLEGROGL=off \
	-DSHARED=off \
	-DCMAKE_C_FLAGS="" \
	-DCMAKE_CXX_FLAGS="-fno-rtti -fno-exceptions" \
	-DCMAKE_LD_FLAGS="" \
	-DCMAKE_TOOLCHAIN_FILE=cmake/Toolchain-android-gcc.cmake \
	-DCMAKE_INSTALL_PREFIX=$(pwd)/../../../nativelibs/$NDK_PLATFORM_NAME

make
make install
