#!/bin/bash

set -e

source ./setenv.sh

FILENAME=libtheora-20150827-gfbb2758
EXTENSION=tar.bz2
BUILD_DIR=$FILENAME

rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR
tar -xf ../../../libsrc/$FILENAME.$EXTENSION -C $BUILD_DIR --strip-components 1

pushd $BUILD_DIR

./autogen.sh $IOS_CONFIGURE_FLAGS --disable-doc --disable-spec --disable-asm --disable-oggtest --disable-vorbistest --disable-encode --disable-examples
make install

popd
rm -rf $BUILD_DIR
