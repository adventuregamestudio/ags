#!/bin/bash

set -e 

source ./ndkenv

SRC_DIR=allegro-4.4.2
rm -rf $SRC_DIR
mkdir $SRC_DIR
tar xf ../../../libsrc/allegro-4.4.2.tar.gz --strip-components=1 -C $SRC_DIR

pushd $SRC_DIR

# Platform independent patch
patch -p0 < ../../../patches/liballegro-4.4.2.patch

cmake . -G "Unix Makefiles" \
	-DWANT_TESTS=off \
	-DWANT_EXAMPLES=off \
	-DWANT_TOOLS=off \
	-DWANT_LOGG=off \
	-DWANT_ALLEGROGL=off \
	-DSHARED=off \
	-DCMAKE_C_FLAGS="$NDK_CFLAGS -fsigned-char" \
	-DCMAKE_CXX_FLAGS="-fno-rtti -fno-exceptions" \
	-DCMAKE_LD_FLAGS="$NDK_LDFLAGS" \
	-DCMAKE_TOOLCHAIN_FILE=cmake/Toolchain-android-gcc.cmake \
	-DCMAKE_INSTALL_PREFIX=$NDK_ADDITIONAL_LIBRARY_PATH

make 
make install

popd

rm -rf $SRC_DIR

