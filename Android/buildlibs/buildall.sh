#!/bin/bash

set -e

pushd ../../libsrc
./download.sh
popd

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
	./lua.sh	
	./ogg.sh	
	./tremor.sh  # ("vorbis") requires ogg
	./theora.sh  # requires ogg, "vorbis"
	./allegro.sh # requires ogg, "vorbis", theora
	./dumb.sh    # requires allegro
	popd
done
