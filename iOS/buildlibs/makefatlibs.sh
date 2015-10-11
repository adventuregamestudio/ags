#!/bin/bash

set -e

mkdir -p ../nativelibs/fat/lib

for i in dumb aldmb alleg freetype ogg theora vorbisidec lua
do
	lipo -create -output ../nativelibs/fat/lib/lib$i.a ../nativelibs/{armv7,armv7s,arm64,i386}/lib/lib$i.a
	ranlib ../nativelibs/fat/lib/lib$i.a
done
