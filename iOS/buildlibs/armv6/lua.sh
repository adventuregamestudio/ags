#!/bin/bash

# Set up build environment
source ./setenv.sh

# Download and extract the library source
FILENAME=lua-5.1.4
EXTENSION=tar.gz
wget -c http://www.lua.org/ftp/$FILENAME.$EXTENSION -O ../$FILENAME.$EXTENSION
tar -zxf ../$FILENAME.$EXTENSION

# Build and install library
cd $FILENAME

# Apply platform patch
patch -p0 < ../../../patches/liblua.patch

make generic \
MYCFLAGS="-arch armv6 -pipe -no-cpp-precomp -isysroot $SDKROOT -miphoneos-version-min=$IOS_DEPLOY_TGT -I$IOS_ADDITIONAL_LIBRARY_PATH/include" \
MYLDFLAGS="-arch armv6 -pipe -no-cpp-precomp -isysroot $SDKROOT -miphoneos-version-min=$IOS_DEPLOY_TGT -Wl,-L$SDKROOT/usr/lib,-L$IOS_ADDITIONAL_LIBRARY_PATH/lib"

make install INSTALL_TOP=$IOS_ADDITIONAL_LIBRARY_PATH
