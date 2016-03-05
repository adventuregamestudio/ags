unset DEVROOT SDKROOT CFLAGS CC LD CPP CXX AR AS NM CXXCPP RANLIB LDFLAGS CPPFLAGS CXXFLAGS

SDK="iphoneos"
ARCH="armv7s"
IOS_HOST_NAME=armv7s-apple-darwin*

# ideally we have 5.1.1 but we get linker errors in xcode 7.0.1 (possibly a bug)
IOS_TARGET="6.0"

# warning: don't set SDKROOT, it is treated specially by xcode tools
SDK_PATH=$(xcrun --sdk $SDK --show-sdk-path)
PREFIX=$(pwd)/../../nativelibs/$ARCH
export IOS_ADDITIONAL_LIBRARY_PATH=$PREFIX

export AR=$(xcrun --sdk $SDK --find ar)
export AS=$(xcrun --sdk $SDK --find as)
export ASCPP=$(xcrun --sdk $SDK --find as)
export CC=$(xcrun --sdk $SDK --find gcc)
export CPP="$(xcrun --sdk $SDK --find gcc) -E"
export CXX=$(xcrun --sdk $SDK --find g++)
export CXXCPP="$(xcrun --sdk $SDK --find g++) -E"
export LD=$(xcrun --sdk $SDK --find ld)
export NM=$(xcrun --sdk $SDK --find nm)
export RANLIB=$(xcrun --sdk $SDK --find ranlib)
export STRIP=$(xcrun --sdk $SDK --find strip)

CROSS_FLAGS="-arch $ARCH -miphoneos-version-min=$IOS_TARGET -isysroot $SDK_PATH"
export CPPFLAGS="$CROSS_FLAGS"
export CFLAGS="$CROSS_FLAGS -I$PREFIX/include -fembed-bitcode"
export CXXFLAGS="$CROSS_FLAGS -I$PREFIX/include -fembed-bitcode"
export LDFLAGS="$CROSS_FLAGS -L$PREFIX/lib"
export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig:$SDK_PATH/usr/lib/pkgconfig"
export ACLOCAL_PATH="$PREFIX/share/aclocal:$SDK_PATH/usr/share/aclocal"

export IOS_CONFIGURE_FLAGS="--host=$IOS_HOST_NAME --prefix=$IOS_ADDITIONAL_LIBRARY_PATH --enable-static --disable-shared"
