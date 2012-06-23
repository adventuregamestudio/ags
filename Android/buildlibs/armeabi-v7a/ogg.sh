#!/bin/bash

# Set up build environment
source ../setenv.sh arm-linux-androideabi

# Download and extract the library source
FILENAME=libogg-1.3.0
EXTENSION=tar.gz
wget -c http://downloads.xiph.org/releases/ogg/$FILENAME.$EXTENSION -O ../$FILENAME.$EXTENSION
tar -zxf ../$FILENAME.$EXTENSION

# Get the newest config files for autotools
source ../update-config.sh $FILENAME

# Build and install library
cd $FILENAME

export LDFLAGS="-Wl,-L$NDK_ADDITIONAL_LIBRARY_PATH/lib,--fix-cortex-a8"
export CFLAGS="-march=armv7-a -mfloat-abi=softfp -mfpu=vfp -marm -fsigned-char -I$NDK_ADDITIONAL_LIBRARY_PATH/include"
export LIBS="-lc"

./configure --host=$NDK_HOST_NAME --prefix=$NDK_ADDITIONAL_LIBRARY_PATH

make
make install
