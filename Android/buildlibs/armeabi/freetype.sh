#!/bin/bash

# Set up build environment
source ../setenv.sh arm-linux-androideabi

# Download and extract the library source
FILENAME=freetype-2.4.6
EXTENSION=tar.bz2
wget -c http://download.savannah.gnu.org/releases/freetype/$FILENAME.$EXTENSION -O ../$FILENAME.$EXTENSION
tar -jxf ../$FILENAME.$EXTENSION

# Get the newest config files for autotools
source ../update-config.sh $FILENAME

# Build and install library
cd $FILENAME

export LDFLAGS="-Wl,-L$NDK_ADDITIONAL_LIBRARY_PATH/lib"
export CFLAGS="-mfloat-abi=softfp -marm -fsigned-char -std=c99 -I$NDK_ADDITIONAL_LIBRARY_PATH/include"
export LIBS="-lc"

./configure --host=$NDK_HOST_NAME --prefix=$NDK_ADDITIONAL_LIBRARY_PATH

make
make install
