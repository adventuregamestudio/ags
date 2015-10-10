#!/bin/bash

set -e

source ./setenv.sh

FILENAME=lua-5.1.5
EXTENSION=tar.gz
BUILD_DIR=$FILENAME

rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR
tar -xf ../../../libsrc/$FILENAME.$EXTENSION -C $BUILD_DIR --strip-components 1

pushd $BUILD_DIR

patch -p0 < ../../../patches/liblua.patch
make generic MYCFLAGS="$CFLAGS" MYLDFLAGS="$LDFLAGS"
make install INSTALL_TOP=$IOS_ADDITIONAL_LIBRARY_PATH

popd

rm -rf $BUILD_DIR
