export NDK_PLATFORM_ROOT=$NDK_ROOT/platforms/android-4/arch-arm/usr
export NDK_ADDITIONAL_LIBRARY_PATH=$(pwd)/../../nativelibs/armeabi
export NDK_HOST_NAME=arm-linux-androideabi

FILENAME=libtremor-svn

#svn co http://svn.xiph.org/trunk/Tremor $FILENAME

cd $FILENAME

# remove call to ./configure from the script
head --lines=-1 autogen.sh > autogenmod.sh

./autogenmod.sh

wget http://git.savannah.gnu.org/cgit/config.git/plain/config.sub -O config.sub
wget http://git.savannah.gnu.org/cgit/config.git/plain/config.guess -O config.guess

LDFLAGS="-Wl,-rpath-link=$NDK_PLATFORM_ROOT/lib,-L$NDK_ADDITIONAL_LIBRARY_PATH/lib,-L$NDK_PLATFORM_ROOT/lib" CFLAGS="-march=armv6 -marm -nostdlib -I$NDK_ADDITIONAL_LIBRARY_PATH/include -I$NDK_PLATFORM_ROOT/include" LIBS="-lc" ./configure --host=$NDK_HOST_NAME --prefix=$NDK_ADDITIONAL_LIBRARY_PATH
make
make install
