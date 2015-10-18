#!/bin/bash

set -e

pushd ../../libsrc
./download.sh
popd

for arch in armeabi armeabi-v7a x86 mips
do
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
