#!/bin/bash

source ./ndkenv

SRC_DIR=lua-5.1.5
rm -rf $SRC_DIR
mkdir $SRC_DIR
tar xf ../../../libsrc/lua-5.1.5.tar.gz --strip-components=1 -C $SRC_DIR

pushd $SRC_DIR

# Apply platform patch
patch -p0 < ../../../patches/liblua.patch

export CC=${NDK_HOST_NAME}-gcc
export MYCFLAGS="$NDK_CFLAGS -fsigned-char -I$NDK_ADDITIONAL_LIBRARY_PATH/include"
export MYLDFLAGS="$NDK_LDFLAGS -Wl,-L$NDK_ADDITIONAL_LIBRARY_PATH/lib"

make generic 
make install INSTALL_TOP=$NDK_ADDITIONAL_LIBRARY_PATH

popd 

rm -rf $SRC_DIR
