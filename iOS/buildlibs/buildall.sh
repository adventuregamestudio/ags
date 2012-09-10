#!/bin/bash

for i in armv6 armv7 i386
do
	cd $i
	chmod +x *.sh
	./freetype.sh
	./ogg.sh
	./tremor.sh
	./theora-svn.sh
	./allegro.sh
	./dumb.sh
	./lua.sh
	cd ..
done
