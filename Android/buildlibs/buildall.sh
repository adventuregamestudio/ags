#!/bin/bash

for i in armeabi armeabi-v7a x86 mips
do
	cd $i
	chmod +x *.sh
	./freetype.sh
	./ogg.sh
	./tremor.sh
	./theora.sh
	./allegro.sh
	./dumb.sh
	cd ..
done
