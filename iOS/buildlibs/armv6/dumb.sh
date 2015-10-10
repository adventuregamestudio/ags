#!/bin/bash

set -e

source ./setenv.sh

FILENAME=dumb-0.9.3
EXTENSION=tar.gz
BUILD_DIR=$FILENAME

rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR
tar -xf ../../../libsrc/$FILENAME.$EXTENSION -C $BUILD_DIR --strip-components 1

pushd $BUILD_DIR

patch -p0 < ../../../patches/armv6/libdumb-0.9.3.patch
make
make install

popd
rm -rf $BUILD_DIR
