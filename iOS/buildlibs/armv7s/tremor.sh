#!/bin/bash

set -e

source ./setenv.sh

FILENAME=libtremor-20150108-r19427
EXTENSION=tar.bz2
BUILD_DIR=$FILENAME

rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR
tar -xf ../../../libsrc/$FILENAME.$EXTENSION -C $BUILD_DIR --strip-components 1

pushd $BUILD_DIR

export CFLAGS="$CFLAGS -mno-thumb -DLITTLE_ENDIAN -DBYTE_ORDER=LITTLE_ENDIAN -Dasm=__asm__"
./autogen.sh $IOS_CONFIGURE_FLAGS --disable-oggtest
make
make install

popd
rm -rf $BUILD_DIR
