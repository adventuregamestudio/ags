#!/bin/bash

set -e 

source ./ndkenv

SRC_DIR=libtremor-20150108-r19427
rm -rf $SRC_DIR
mkdir $SRC_DIR
tar xf ../../../libsrc/libtremor-20150108-r19427.tar.bz2 --strip-components=1 -C $SRC_DIR

pushd $SRC_DIR

export CFLAGS="$NDK_CFLAGS -fsigned-char -I$NDK_ADDITIONAL_LIBRARY_PATH/include -DLITTLE_ENDIAN -DBYTE_ORDER=LITTLE_ENDIAN"
export LDFLAGS="$NDK_LDFLAGS -Wl,-L$NDK_ADDITIONAL_LIBRARY_PATH/lib"

./autogen.sh --host=$NDK_HOST_NAME --prefix=$NDK_ADDITIONAL_LIBRARY_PATH --disable-shared

make
make install

popd

rm -rf $SRC_DIR
