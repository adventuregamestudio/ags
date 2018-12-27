#!/bin/sh
set -e

BINDMOUNT=
BIT=32

  set +x
  if test $BIT -eq 32
    then
      TRIPLET=i386-linux-gnu
    else
      TRIPLET=x86_64-linux-gnu
  fi
  set -x

  mkdir -p $BINDMOUNT/data/licenses
  mkdir $BINDMOUNT/data/lib$BIT

  for library in \
    liballeg.so.4.4 \
    libaldmb.so.1 \
    libdumb.so.1 \
    libfreetype.so.6 \
    libGLEW.so.2.0.0 \
    libGLEW.so.2.0 \
    libogg.so.0 \
    libtheora.so.0 \
    libvorbis.so.0 \
    libvorbisfile.so.3 \
    allegro/4.4.2/alleg-alsadigi.so \
    allegro/4.4.2/alleg-alsamidi.so \
    allegro/4.4.2/modules.lst; do
      cp -L /usr/lib/$TRIPLET/$library $BINDMOUNT/data/lib$BIT
  done


for package in \
  liballegro4.4 \
  libdumb1 \
  libfreetype6 \
  libogg0 \
  libtheora0 \
  libvorbis0a; do
    cp /usr/share/doc/$package/copyright $BINDMOUNT/data/licenses/$package-copyright
done
