#!/bin/bash

set -e

pushd ../../libsrc
./download.sh
popd

for arch in armv7 armv7s arm64 i386 x86_64
do
	rm -rf ../nativelibs/$arch
	mkdir -p ../nativelibs/$arch
	pushd $arch
	chmod +x *.sh
	./lua.sh
	./freetype.sh
	./ogg.sh
	./tremor.sh
	./theora.sh
	./allegro.sh
	./dumb.sh
	popd
done
