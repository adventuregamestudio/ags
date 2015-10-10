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

# wget https://raw.github.com/gabriel/ffmpeg-iphone-build/master/gas-preprocessor.pl -O lib/arm/gas-preprocessor.pl --no-check-certificate
# chmod +x lib/arm/gas-preprocessor.pl
# patch -p0 < ../../../patches/armv7/libtheora-svn.patch
# export CCAS="perl ./arm/gas-preprocessor.pl $DEVROOT/usr/bin/arm-apple-darwin10-gcc-4.2.1"

./autogen.sh $IOS_CONFIGURE_FLAGS --disable-doc --disable-spec --disable-asm --disable-oggtest --disable-vorbistest --disable-encode --disable-examples
make install

popd
rm -rf $BUILD_DIR
