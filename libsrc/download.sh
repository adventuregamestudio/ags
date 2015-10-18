#!/bin/bash 

set -e

WGET="wget --no-clobber --quiet"

#$WGET http://download.gna.org/allegro/allegro/4.4.2/allegro-4.4.2.tar.gz
$WGET https://s3-ap-southeast-2.amazonaws.com/ags-shared/allegro-4.4.2.tar.gz

#$WGET http://downloads.sourceforge.net/project/dumb/dumb/0.9.3/dumb-0.9.3.tar.gz
$WGET https://s3-ap-southeast-2.amazonaws.com/ags-shared/dumb-0.9.3.tar.gz

#$WGET http://download.savannah.gnu.org/releases/freetype/freetype-2.4.12.tar.bz2
$WGET https://s3-ap-southeast-2.amazonaws.com/ags-shared/freetype-2.4.12.tar.bz2

#$WGET http://downloads.xiph.org/releases/ogg/libogg-1.3.2.tar.gz
$WGET https://s3-ap-southeast-2.amazonaws.com/ags-shared/libogg-1.3.2.tar.gz

$WGET https://s3-ap-southeast-2.amazonaws.com/ags-shared/libtheora-20150828-gfbb2758.tar.bz2

$WGET https://s3-ap-southeast-2.amazonaws.com/ags-shared/libtremor-20150108-r19427.tar.bz2

#$WGET http://www.lua.org/ftp/lua-5.1.5.tar.gz
$WGET https://s3-ap-southeast-2.amazonaws.com/ags-shared/lua-5.1.5.tar.gz

sha1sum --quiet --check sha1sums
