#!/bin/bash

# Needs a bit of work, but this will build the standalone toolchains necessary to build libs.

set -e

# Please update NDK_HOME to the path of ndk-bundle in your system
NDK_HOME=~/Android/Sdk/ndk-bundle

# android-14 is the minimum we can go with current Android SDK
PLATFORM=android-14

# standalone toolchains cannot share same directory
export NDK_STANDALONE=$NDK_HOME/platforms/$PLATFORM

# if you don't pass force, the directory is not rewritten
# verbose is needed to see why making the toolchain fails
for arch in arm x86 mips
do
    INSTALL_DIR=$NDK_STANDALONE/$arch
    mkdir -p $INSTALL_DIR
    $NDK_HOME/build/tools/make-standalone-toolchain.sh --force --verbose --platform=$PLATFORM --install-dir=$INSTALL_DIR --arch=$arch
done

