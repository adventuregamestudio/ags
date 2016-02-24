#!/bin/bash

set -e

source ./setenv.sh

FILENAME=allegro-4.4.2
EXTENSION=tar.gz
BUILD_DIR=$FILENAME

rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR
tar -xf ../../../libsrc/$FILENAME.$EXTENSION -C $BUILD_DIR --strip-components 1

pushd $BUILD_DIR

patch -p0 < ../../../patches/liballegro-4.4.2.patch
patch -p0 < ../../../patches/i386/liballegro-4.4.2.patch
cmake -G "Unix Makefiles" \
-DCMAKE_C_FLAGS="$CFLAGS" \
-DCMAKE_LD_FLAGS="$LDFLAGS" \
-DWANT_TESTS=off -DWANT_EXAMPLES=off -DWANT_TOOLS=off -DWANT_LOGG=off -DWANT_ALLEGROGL=off -DSHARED=off \
-DCMAKE_CXX_FLAGS="$CXXFLAGS -fno-rtti -fno-exceptions" \
-DCMAKE_TOOLCHAIN_FILE=cmake/Toolchain-ios-gcc.cmake \
-DCMAKE_INSTALL_PREFIX=$IOS_ADDITIONAL_LIBRARY_PATH .

make
make install

popd
rm -rf $BUILD_DIR
