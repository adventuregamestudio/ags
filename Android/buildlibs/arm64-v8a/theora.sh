#!/bin/bash

set -e 

source ./ndkenv

SRC_DIR=libtheora-20150828-gfbb2758
rm -rf $SRC_DIR
mkdir $SRC_DIR
tar xf ../../../libsrc/libtheora-20150828-gfbb2758.tar.bz2 --strip-components=1 -C $SRC_DIR

export CFLAGS="$NDK_CFLAGS -fsigned-char -I$NDK_ADDITIONAL_LIBRARY_PATH/include"
export LDFLAGS="$NDK_LDFLAGS -Wl,-L$NDK_ADDITIONAL_LIBRARY_PATH/lib"

pushd $SRC_DIR

# disable asflag-probe as it guess wrong arm arch
./autogen.sh --host=$NDK_HOST_NAME --prefix=$NDK_ADDITIONAL_LIBRARY_PATH --disable-examples --disable-asflag-probe --disable-encode --disable-shared --disable-oggtest --disable-vorbistest

make
make install

popd

rm -rf $SRC_DIR
