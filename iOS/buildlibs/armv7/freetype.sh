#!/bin/bash

set -e

source ./setenv.sh

FILENAME=freetype-2.4.12
EXTENSION=tar.bz2
BUILD_DIR=$FILENAME

rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR
tar -xf ../../../libsrc/$FILENAME.$EXTENSION -C $BUILD_DIR --strip-components 1

pushd $BUILD_DIR

export CFLAGS="$CFLAGS -std=c99"
./configure $IOS_CONFIGURE_FLAGS --without-zlib
make
make install

popd
rm -rf $BUILD_DIR
