#!/bin/bash

source ./setenv.sh

FILENAME=libogg-1.3.0
EXTENSION=tar.gz

wget -c http://downloads.xiph.org/releases/ogg/$FILENAME.$EXTENSION -O ../$FILENAME.$EXTENSION

tar -zxf ../$FILENAME.$EXTENSION

cd $FILENAME

wget http://git.savannah.gnu.org/cgit/config.git/plain/config.sub -O config.sub
wget http://git.savannah.gnu.org/cgit/config.git/plain/config.guess -O config.guess

export LDFLAGS="-Wl,-L$SDKROOT/usr/lib,-L$IOS_ADDITIONAL_LIBRARY_PATH/lib"
export CFLAGS="-std=c99 -arch i586 -pipe -no-cpp-precomp -isysroot $SDKROOT -miphoneos-version-min=$IOS_DEPLOY_TGT -I$IOS_ADDITIONAL_LIBRARY_PATH/include"
./configure --host=$IOS_HOST_NAME --prefix=$IOS_ADDITIONAL_LIBRARY_PATH
make
make install
