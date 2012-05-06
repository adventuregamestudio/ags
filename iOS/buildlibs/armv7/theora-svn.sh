#!/bin/bash

source ./setenv.sh

FILENAME=libtheora-svn

svn co http://svn.xiph.org/trunk/theora $FILENAME

cd $FILENAME

# remove call to ./configure from the script
head --lines=-1 autogen.sh > autogenmod.sh

chmod +x ./autogenmod.sh

./autogenmod.sh

rm ./config.sub
rm ./config.guess

wget http://git.savannah.gnu.org/cgit/config.git/plain/config.sub -O config.sub
wget http://git.savannah.gnu.org/cgit/config.git/plain/config.guess -O config.guess

wget https://raw.github.com/gabriel/ffmpeg-iphone-build/master/gas-preprocessor.pl -O lib/arm/gas-preprocessor.pl --no-check-certificate
chmod +x lib/arm/gas-preprocessor.pl

patch -p0 < ../../../patches/armv7/libtheora-svn.patch

export CCAS="perl ./arm/gas-preprocessor.pl $DEVROOT/usr/bin/arm-apple-darwin10-gcc-4.2.1"

export OGG_LIBS="-L$IOS_ADDITIONAL_LIBRARY_PATH/lib -logg"
export VORBIS_LIBS="-L$IOS_ADDITIONAL_LIBRARY_PATH/lib -lm -lvorbis -logg"
export LDFLAGS="-Wl,-L$SDKROOT/usr/lib,-L$IOS_ADDITIONAL_LIBRARY_PATH/lib"
export CFLAGS="-std=c99 -arch armv7 -mcpu=cortex-a8 -pipe -no-cpp-precomp -isysroot $SDKROOT -miphoneos-version-min=$IOS_DEPLOY_TGT -I$IOS_ADDITIONAL_LIBRARY_PATH/include -DIOS_ARMV7"
./configure --host=$IOS_HOST_NAME --prefix=$IOS_ADDITIONAL_LIBRARY_PATH --disable-examples
make install
