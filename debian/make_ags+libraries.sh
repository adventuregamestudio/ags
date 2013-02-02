#!/bin/sh
set -e

# In the moment this script should only be used on Debian.
# That is because Debian currently uses glibc 2.13, while
# most other major Linux distributions (including Ubuntu)
# use version 2.15. Creating the allegro+libraries bundle
# on a distribution with glibc 2.15 will likely result in
# something that does not work on Debian.

BASEPATH=$(dirname $(dirname $(readlink -f $0)))

if test -d $BASEPATH/ags+libraries
  then
    echo Error: Directory $BASEPATH/ags+libraries already exists. Please remove it first.
    exit 2
fi

echo Creating ags+libraries in $BASEPATH/
echo "See debian/README.md for usage instructions."

set -x
mkdir -p $BASEPATH/ags+libraries/data/licenses

for bit in 32 64; do
  set +x
  if test $bit -eq 32
    then
      TRIPLET=i386-linux-gnu
    else
      TRIPLET=x86_64-linux-gnu
  fi
  set -x

  mkdir $BASEPATH/ags+libraries/data/lib$bit

  for library in \
    liballeg.so.4.4 \
    libaldmb.so.1 \
    libdumb.so.1 \
    libfreetype.so.6 \
    libogg.so.0 \
    libtheora.so.0 \
    libvorbis.so.0 \
    libvorbisfile.so.3 \
    allegro/4.4.2/alleg-alsadigi.so \
    allegro/4.4.2/alleg-alsamidi.so \
    allegro/4.4.2/modules.lst; do
      cp -L /usr/lib/$TRIPLET/$library $BASEPATH/ags+libraries/data/lib$bit
  done
done

for package in \
  liballegro4.4 \
  libdumb1 \
  libfreetype6 \
  libogg0 \
  libtheora0 \
  libvorbis0a; do
    cp /usr/share/doc/$package/copyright $BASEPATH/ags+libraries/data/licenses/$package-copyright
done

cp $BASEPATH/debian/copyright $BASEPATH/ags+libraries/data/licenses/ags-copyright
cp $BASEPATH/debian/ags+libraries/startgame $BASEPATH/ags+libraries/
cp $BASEPATH/debian/ags+libraries/README $BASEPATH/ags+libraries/

make --directory=$BASEPATH/Engine LDFLAGS="'-Wl,-rpath,\$\$ORIGIN/lib64'" -j5

cp $BASEPATH/Engine/ags $BASEPATH/ags+libraries/data/ags64
strip $BASEPATH/ags+libraries/data/ags64

# I compile and strip the 32 bit binary separately and make sure it is in $BASEPATH.
cp $BASEPATH/ags $BASEPATH/ags+libraries/data/ags32

cd $BASEPATH && tar -cf - ags+libraries/ | xz -9 -c - > ags+libraries_$(date +%Y%m%d).tar.xz 
