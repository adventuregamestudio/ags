#!/bin/bash

set -e

pushd ../../libsrc
./download.sh
popd

# Please update NDK_HOME to the path of ndk-bundle in your system
NDK_HOME=~/Android/Sdk/ndk-bundle

# android-14 is the minimum we can go with current Android SDK
PLATFORM=android-14

# standalone toolchains cannot share same directory
export NDK_STANDALONE=$NDK_HOME/platforms/$PLATFORM

for arch in armeabi armeabi-v7a x86 mips
do
	rm -rf ../nativelibs/$arch
	mkdir -p ../nativelibs/$arch
	pushd $arch
	chmod +x *.sh
	./freetype.sh  
	./lua.sh	
	./ogg.sh	
	./tremor.sh  # ("vorbis") requires ogg
	./theora.sh  # requires ogg, "vorbis"
	./allegro.sh # requires ogg, "vorbis", theora
	./dumb.sh    # requires allegro
	popd
done
