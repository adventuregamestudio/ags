#!/bin/bash

source ./setenv.sh

FILENAME=dumb-0.9.3
EXTENSION=tar.gz

wget -c http://downloads.sourceforge.net/project/dumb/dumb/0.9.3/dumb-0.9.3.tar.gz -O ../$FILENAME.$EXTENSION

tar -zxf ../$FILENAME.$EXTENSION

cd $FILENAME

patch -p0 < ../../../patches/armv7/libdumb-0.9.3.patch

make
make install
