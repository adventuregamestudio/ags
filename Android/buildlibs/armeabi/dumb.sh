#!/bin/bash

# Set up build environment
source ../setenv.sh arm-linux-androideabi

# Download and extract the library source
FILENAME=dumb-0.9.3
EXTENSION=tar.gz
wget -c http://downloads.sourceforge.net/project/dumb/dumb/0.9.3/dumb-0.9.3.tar.gz -O ../$FILENAME.$EXTENSION
tar -zxf ../$FILENAME.$EXTENSION

# Build and install library
cd $FILENAME

# Apply platform patch
patch -p0 < ../../../patches/$NDK_PLATFORM_NAME/libdumb-0.9.3.patch

make
make install
