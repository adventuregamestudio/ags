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
    libogg.so.0 \
    libtheora.so.0 \
    libvorbis.so.0 \
    libvorbisfile.so.3; do
      cp -L /usr/lib/$TRIPLET/$library $BINDMOUNT/data/lib$BIT
  done
  
  for library in \
    libSDL2-2.0.so.0; do
      cp -L /usr/local/lib/$library $BINDMOUNT/data/lib$BIT
  done

for package in \
  libogg0 \
  libtheora0 \
  libvorbis0a; do
    cp /usr/share/doc/$package/copyright $BINDMOUNT/data/licenses/$package-copyright
done

  cp /usr/local/share/doc/libSDL2-2.0/copyright $BINDMOUNT/data/licenses/libSDL2-2.0-copyright
