#!/bin/bash

source ./ndkenv

SRC_DIR=dumb-0.9.3
rm -rf $SRC_DIR
mkdir $SRC_DIR
tar xf ../../../libsrc/dumb-0.9.3.tar.gz --strip-components=1 -C $SRC_DIR

pushd $SRC_DIR

patch -p1 < ../../../patches/libdumb-0.9.3.patch

make
make install

popd

rm -rf $SRC_DIR
