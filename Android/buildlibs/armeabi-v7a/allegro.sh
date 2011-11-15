export NDK_PLATFORM_ROOT=$NDK_ROOT/platforms/android-4/arch-arm/usr
export NDK_ADDITIONAL_LIBRARY_PATH=$(pwd)/../../nativelibs/armeabi-v7a
export NDK_HOST_NAME=arm-linux-androideabi

FILENAME=allegro-4.4.2
EXTENSION=tar.gz

wget -c http://downloads.sourceforge.net/project/alleg/allegro/4.4.2/$FILENAME.$EXTENSION -O ../$FILENAME.$EXTENSION

tar -zxf ../$FILENAME.$EXTENSION

cd $FILENAME

patch -p0 < ../../../patches/armeabi-v7a/liballegro-4.4.2.patch

cmake -G "Unix Makefiles" -DWANT_TESTS=off -DWANT_EXAMPLES=off -DWANT_TOOLS=off -DWANT_LOGG=off -DWANT_ALLEGROGL=off -DSHARED=off -DCMAKE_TOOLCHAIN_FILE=cmake/Toolchain-android-gcc.cmake -DCMAKE_INSTALL_PREFIX=$NDK_ADDITIONAL_LIBRARY_PATH .

make
make install