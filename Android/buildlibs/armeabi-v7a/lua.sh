#!/bin/bash

# Set up build environment
source ../setenv.sh arm-linux-androideabi

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
MYLDFLAGS="-Wl,-L$NDK_ADDITIONAL_LIBRARY_PATH/lib,--fix-cortex-a8" \
MYCFLAGS="-march=armv7-a -mfloat-abi=softfp -mfpu=vfp -marm -fsigned-char -I$NDK_ADDITIONAL_LIBRARY_PATH/include"

make install INSTALL_TOP=$NDK_ADDITIONAL_LIBRARY_PATH
