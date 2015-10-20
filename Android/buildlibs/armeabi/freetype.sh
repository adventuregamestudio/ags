#!/bin/bash

set -e 

source ./ndkenv

SRC_DIR=freetype-2.4.12
rm -rf $SRC_DIR
mkdir $SRC_DIR
tar xf ../../../libsrc/freetype-2.4.12.tar.bz2 --strip-components=1 -C $SRC_DIR

pushd $SRC_DIR

export CFLAGS="$NDK_CFLAGS -std=gnu99 -fsigned-char" 
export LDFLAGS="$NDK_LDFLAGS"

./configure --host=$NDK_HOST_NAME --prefix=$NDK_ADDITIONAL_LIBRARY_PATH --without-zlib --disable-shared

make
make install

popd

rm -rf $SRC_DIR
