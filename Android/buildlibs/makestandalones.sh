#!/bin/bash

# Needs a bit of work, but this will build the standalone toolchains necessary to build libs.

set -e

# Last committed pre-compiled libs were with R8
NDK_HOME=/opt/android-ndk-r10e

# android-9 is minimum to support mips and x86
PLATFORM=android-9

# standalone toolchains cannot share same directory
NDK_STANDALONE=$HOME/ndk-r10e-standalone/$PLATFORM

for arch in arm x86 mips
do
    INSTALL_DIR=$NDK_STANDALONE/$arch
    mkdir -p $INSTALL_DIR
    $NDK_HOME/build/tools/make-standalone-toolchain.sh --platform=$PLATFORM --install-dir=$INSTALL_DIR --arch=$arch
done

