export NDK_PLATFORM_ROOT=$NDK_ROOT/platforms/android-4/arch-arm/usr
export NDK_ADDITIONAL_LIBRARY_PATH=$(pwd)/../../nativelibs/armeabi-v7a
export NDK_HOST_NAME=arm-linux-androideabi

FILENAME=libogg-1.3.0
EXTENSION=tar.gz

wget -c http://downloads.xiph.org/releases/ogg/$FILENAME.$EXTENSION -O ../$FILENAME.$EXTENSION

tar -zxf ../$FILENAME.$EXTENSION

cd $FILENAME

wget http://git.savannah.gnu.org/cgit/config.git/plain/config.sub -O config.sub
wget http://git.savannah.gnu.org/cgit/config.git/plain/config.guess -O config.guess

LDFLAGS="-Wl,--fix-cortex-a8,-rpath-link=$NDK_PLATFORM_ROOT/lib,-L$NDK_ADDITIONAL_LIBRARY_PATH/lib,-L$NDK_PLATFORM_ROOT/lib" CFLAGS="-march=armv7-a -mfloat-abi=softfp -mfpu=vfp -marm -nostdlib -I$NDK_ADDITIONAL_LIBRARY_PATH/include -I$NDK_PLATFORM_ROOT/include" LIBS="-lc" ./configure --host=$NDK_HOST_NAME --prefix=$NDK_ADDITIONAL_LIBRARY_PATH
make
make install
