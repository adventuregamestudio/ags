#!/bin/bash

source ./setenv.sh

FILENAME=allegro-4.4.2
EXTENSION=tar.gz

wget -c http://downloads.sourceforge.net/project/alleg/allegro/4.4.2/$FILENAME.$EXTENSION -O ../$FILENAME.$EXTENSION

tar -zxf ../$FILENAME.$EXTENSION

cd $FILENAME

patch -p0 < ../../../patches/liballegro-4.4.2.patch
patch -p0 < ../../../patches/armv6/liballegro-4.4.2.patch

cmake -G "Unix Makefiles" -DWANT_TESTS=off -DWANT_EXAMPLES=off -DWANT_TOOLS=off -DWANT_LOGG=off -DWANT_ALLEGROGL=off -DSHARED=off -DCMAKE_CXX_FLAGS="-fno-rtti -fno-exceptions" -DCMAKE_TOOLCHAIN_FILE=cmake/Toolchain-ios-gcc.cmake -DCMAKE_INSTALL_PREFIX=$(pwd)/../../../nativelibs/armv6 .

make
make install
