#!/bin/bash

source ./setenv.sh

FILENAME=freetype-2.4.6
EXTENSION=tar.bz2

wget -c http://download.savannah.gnu.org/releases/freetype/$FILENAME.$EXTENSION -O ../$FILENAME.$EXTENSION

tar -jxf ../$FILENAME.$EXTENSION

cd $FILENAME

wget http://git.savannah.gnu.org/cgit/config.git/plain/config.sub -O config.sub
wget http://git.savannah.gnu.org/cgit/config.git/plain/config.guess -O config.guess

export LDFLAGS="-Wl,-L$SDKROOT/usr/lib"
export CFLAGS="-std=c99 -arch armv6 -pipe -no-cpp-precomp -isysroot $SDKROOT -miphoneos-version-min=$IOS_DEPLOY_TGT -I$IOS_ADDITIONAL_LIBRARY_PATH/include"
./configure --host=$IOS_HOST_NAME --prefix=$IOS_ADDITIONAL_LIBRARY_PATH
make
make install
