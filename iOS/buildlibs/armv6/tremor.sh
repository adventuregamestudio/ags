#!/bin/bash

source ./setenv.sh

FILENAME=libtremor-svn

svn co http://svn.xiph.org/trunk/Tremor $FILENAME

cd $FILENAME

# remove call to ./configure from the script
head --lines=-1 autogen.sh > autogenmod.sh

chmod +x ./autogenmod.sh

./autogenmod.sh

rm ./config.sub
rm ./config.guess

wget http://git.savannah.gnu.org/cgit/config.git/plain/config.sub -O config.sub
wget http://git.savannah.gnu.org/cgit/config.git/plain/config.guess -O config.guess

export LDFLAGS="-Wl,-L$SDKROOT/usr/lib,-L$IOS_ADDITIONAL_LIBRARY_PATH/lib"
export CFLAGS="-std=c99 -arch armv6 -pipe -no-cpp-precomp -isysroot $SDKROOT -miphoneos-version-min=$IOS_DEPLOY_TGT -I$IOS_ADDITIONAL_LIBRARY_PATH/include -DLITTLE_ENDIAN -DBYTE_ORDER=LITTLE_ENDIAN -Dasm=__asm__"
./configure --host=$IOS_HOST_NAME --prefix=$IOS_ADDITIONAL_LIBRARY_PATH
make
make install
