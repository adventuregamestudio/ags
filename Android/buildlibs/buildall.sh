#!/bin/bash

set -e

for arch in armeabi armeabi-v7a x86 mips
do
	pushd $arch
	chmod +x *.sh
	./freetype.sh
	./ogg.sh
	./tremor.sh
	./theora.sh
	./allegro.sh
	./dumb.sh
	./lua.sh
	popd
done
