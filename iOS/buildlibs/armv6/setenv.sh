#!/bin/bash

unset DEVROOT SDKROOT CFLAGS CC LD CPP CXX AR AS NM CXXCPP RANLIB LDFLAGS CPPFLAGS CXXFLAGS

export IOS_BASE_SDK="4.3"
export IOS_DEPLOY_TGT="3.2"

export DEVROOT=/Developer/Platforms/iPhoneOS.platform/Developer
export SDKROOT=$DEVROOT/SDKs/iPhoneOS$IOS_BASE_SDK.sdk
export IOS_ADDITIONAL_LIBRARY_PATH=$(pwd)/../../nativelibs/armv6
export IOS_HOST_NAME=arm-apple-darwin10

export PATH="$PATH;$DEVROOT/usr/bin"

export CPP="$DEVROOT/usr/bin/cpp-4.2"
export CXX="$DEVROOT/usr/bin/g++-4.2"
export CXXCPP="$DEVROOT/usr/bin/cpp-4.2"
export CC="$DEVROOT/usr/bin/gcc-4.2"
export LD=$DEVROOT/usr/bin/ld
export AR=$DEVROOT/usr/bin/ar
export AS=$DEVROOT/usr/bin/as
export NM=$DEVROOT/usr/bin/nm
export RANLIB=$DEVROOT/usr/bin/ranlib
