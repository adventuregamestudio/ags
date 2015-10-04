#!/bin/bash

source ./ndkenv

SRC_DIR=libogg-1.3.2
rm -rf $SRC_DIR
mkdir $SRC_DIR
tar xf ../../../libsrc/libogg-1.3.2.tar.gz --strip-components=1 -C $SRC_DIR

pushd $SRC_DIR

export CFLAGS="$NDK_CFLAGS -fsigned-char"
export LDFLAGS="$NDK_LDFLAGS"

./configure --host=$NDK_HOST_NAME --prefix=$NDK_ADDITIONAL_LIBRARY_PATH  

make
make install

popd 

rm -rf $SRC_DIR
