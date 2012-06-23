#!/bin/bash

# Set up build environment
source ../setenv.sh arm-linux-androideabi

# Checkout the library source
FILENAME=libtheora-svn
svn co http://svn.xiph.org/trunk/theora $FILENAME

# Remove call to ./configure from the autogen script
cd $FILENAME
head --lines=-1 autogen.sh > autogenmod.sh

chmod +x ./autogenmod.sh

./autogenmod.sh

# Get the newest config files for autotools
rm config.guess
rm config.sub
cd ..
source ../update-config.sh $FILENAME

# Build and install library
cd $FILENAME

export LDFLAGS="-Wl,-L$NDK_ADDITIONAL_LIBRARY_PATH/lib,--fix-cortex-a8"
export CFLAGS="-march=armv7-a -mfloat-abi=softfp -mfpu=vfp -marm -fsigned-char -I$NDK_ADDITIONAL_LIBRARY_PATH/include"
export LIBS="-lc"

./configure --host=$NDK_HOST_NAME --prefix=$NDK_ADDITIONAL_LIBRARY_PATH --disable-examples

make
make install
