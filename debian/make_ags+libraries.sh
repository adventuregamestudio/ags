#!/bin/sh
set -e

# This script uses Debian 7 (wheezy) chroots to build ags
# on Debian or Ubuntu.
# That is because Debian 7 uses glibc 2.13, while
# most other major Linux distributions (including Ubuntu)
# use higher versions. Building ags
# on a distribution with glibc > 2.13 will likely result in
# something that does not work on Debian 7.

# Preliminaries for running this script:
# This script is intended for use on Debian or Ubuntu
# Install ubuntu-dev-scripts and cowbuilder:

# sudo apt-get install ubuntu-dev-tools cowbuilder

# Create the (chroot) environments that will be used for building ags:

# cowbuilder-dist wheezy i386 create
# cowbuilder-dist wheezy amd64 create

# The only other thing you should take care of is that your ags
# source tree is git clean. That means: check 'git status', commit any
# changes and maybe run 'git clean -dfx'.

BASEPATH=$(dirname $(dirname $(readlink -f $0)))

if test -d $BASEPATH/ags+libraries
  then
    echo Error: Directory $BASEPATH/ags+libraries already exists. Please remove it first.
    exit 2
fi

echo Creating ags+libraries in $BASEPATH/
echo "See debian/README.md for usage instructions."

set -x
mkdir -p $BASEPATH/ags+libraries
set +x

sed -i -r "4s/.*/BINDMOUNT=$(echo $BASEPATH | sed -e 's/[\/&]/\\&/g')\/ags+libraries/" $BASEPATH/debian/ags+libraries/hooks/B00_copy_libs.sh
sed -i -r "5s/.*/BIT=32/" $BASEPATH/debian/ags+libraries/hooks/B00_copy_libs.sh

cd $BASEPATH
VERSION=$(dpkg-parsechangelog | grep -x "Version:.*" | sed 's@Version: \(.\+\)@\1@')
debian/rules get-orig-source
debuild -us -uc -S

DEB_BUILD_OPTIONS="rpath=$ORIGIN/lib32" cowbuilder-dist wheezy i386 build $BASEPATH/../ags_$VERSION.dsc --buildresult $BASEPATH/ags+libraries --hookdir $BASEPATH/debian/ags+libraries/hooks --bindmounts "$BASEPATH/ags+libraries" 

cd $BASEPATH/ags+libraries
ar p $BASEPATH/ags+libraries/ags_${VERSION}_i386.deb data.tar.gz | tar zx
sudo cp $BASEPATH/ags+libraries/usr/bin/ags $BASEPATH/ags+libraries/data/ags32
rm -rf $BASEPATH/ags+libraries/ags_* $BASEPATH/ags+libraries/ags-dbg_* $BASEPATH/ags+libraries/usr

sed -i -r "5s/.*/BIT=64/" $BASEPATH/debian/ags+libraries/hooks/B00_copy_libs.sh
DEB_BUILD_OPTIONS="rpath=$ORIGIN/lib64" cowbuilder-dist wheezy amd64 build $BASEPATH/../ags_$VERSION.dsc --buildresult $BASEPATH/ags+libraries --hookdir $BASEPATH/debian/ags+libraries/hooks --bindmounts "$BASEPATH/ags+libraries"

cd $BASEPATH/ags+libraries
ar p $BASEPATH/ags+libraries/ags_${VERSION}_amd64.deb data.tar.gz | tar zx
sudo cp $BASEPATH/ags+libraries/usr/bin/ags $BASEPATH/ags+libraries/data/ags64
rm -rf $BASEPATH/ags+libraries/ags_* $BASEPATH/ags+libraries/ags-dbg_* $BASEPATH/ags+libraries/usr last_operation.log

sed -i -r "4s/.*/BINDMOUNT=/" $BASEPATH/debian/ags+libraries/hooks/B00_copy_libs.sh
sed -i -r "5s/.*/BIT=32/" $BASEPATH/debian/ags+libraries/hooks/B00_copy_libs.sh

sudo cp $BASEPATH/debian/copyright $BASEPATH/ags+libraries/data/licenses/ags-copyright
cp $BASEPATH/debian/ags+libraries/startgame $BASEPATH/ags+libraries/
cp $BASEPATH/debian/ags+libraries/README $BASEPATH/ags+libraries/

cd $BASEPATH && tar -cf - ags+libraries/ | xz -9 -c - > ags+libraries_$(date +%Y%m%d).tar.xz 
