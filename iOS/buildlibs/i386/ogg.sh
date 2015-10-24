#!/bin/bash

set -e

source ./setenv.sh

FILENAME=libogg-1.3.2
EXTENSION=tar.gz
BUILD_DIR=$FILENAME

rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR
tar -xf ../../../libsrc/$FILENAME.$EXTENSION -C $BUILD_DIR --strip-components 1

pushd $BUILD_DIR

# -O4 prevents adding ogg to a fat library http://stackoverflow.com/questions/11711100/cross-compiling-libogg-for-ios
CFLAGS="$CFLAGS -O3"
./configure $IOS_CONFIGURE_FLAGS
make
make install

popd
rm -rf $BUILD_DIR
