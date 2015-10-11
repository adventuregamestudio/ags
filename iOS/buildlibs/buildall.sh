#!/bin/bash

set -e

for arch in armv7 armv7s i386 # future: arm64 x86_64
do
	rm -rf ../nativelibs/$arch
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
