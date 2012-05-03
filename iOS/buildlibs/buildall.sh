#!/bin/bash

cd armv6
chmod +x *.sh
./freetype.sh
./ogg.sh
./tremor.sh
./theora-svn.sh
./allegro.sh
./dumb.sh

cd ..

cd armv7
chmod +x *.sh
./freetype.sh
./ogg.sh
./tremor.sh
./theora-svn.sh
./allegro.sh
./dumb.sh

cd ..

cd i386
chmod +x *.sh
./freetype.sh
./ogg.sh
./tremor.sh
./theora-svn.sh
./allegro.sh
./dumb.sh

cd ..

./makefatlibs.sh
