export NDK_PLATFORM_ROOT=$NDK_ROOT/platforms/android-4/arch-arm/usr
export NDK_ADDITIONAL_LIBRARY_PATH=$(pwd)/../../nativelibs/armeabi
export NDK_HOST_NAME=arm-linux-androideabi

FILENAME=dumb-0.9.3
EXTENSION=tar.gz

wget -c http://downloads.sourceforge.net/project/dumb/dumb/0.9.3/dumb-0.9.3.tar.gz -O ../$FILENAME.$EXTENSION

tar -zxf ../$FILENAME.$EXTENSION

cd $FILENAME

patch -p0 < ../../../patches/armeabi/libdumb-0.9.3.patch

make
make install
