#!/bin/bash

set -e

for arch in armv6 armv7 i386 # future: armv7s x86_64
do
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
