#!/bin/sh
set -e

# This script uses Debian 8 (jessie) chroots to build ags
# on Debian or Ubuntu.
# That is because Debian 8 uses glibc 2.19, while
# most other major Linux distributions (including Ubuntu)
# use higher versions. Building ags
# on a distribution with glibc > 2.19 will likely result in
# something that does not work on Debian 8.

# Preliminaries for running this script:
# This script is intended for use on Debian or Ubuntu
# Install ubuntu-dev-scripts and pbuilder:

# sudo apt-get install ubuntu-dev-tools pbuilder

# Create the (chroot) environments that will be used for building ags:

# pbuilder-dist jessie i386 --security-only create
# pbuilder-dist jessie amd64 --security-only create

# The only other thing you should take care of is that your ags
# source tree is git clean. That means: check 'git status', commit any
# changes and maybe run 'git clean -dfx'.

# The chroots can later be updated, which becomes necessary if
# this script fails because some Debian package cannot be downloaded:

# pbuilder-dist jessie i386 --security-only update
# pbuilder-dist jessie amd64 --security-only update

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

# Modifying the hook script itself, because I don't know a better way
# to pass parameters to it.
sed -i -r "4s/.*/BINDMOUNT=$(echo $BASEPATH | sed -e 's/[\/&]/\\&/g')\/ags+libraries/" $BASEPATH/debian/ags+libraries/hooks/B00_copy_libs.sh
sed -i -r "5s/.*/BIT=32/" $BASEPATH/debian/ags+libraries/hooks/B00_copy_libs.sh

# Build the source package:
cd $BASEPATH
CHANGELOG_VERSION=$(head -1 debian/changelog | sed 's/.*(\(.*\)).*/\1/')
VERSION=$(grep '#define ACI_VERSION_STR' Common/core/def_version.h | sed 's/.*"\(.*\)".*/\1/')
sed -i -- "s/$CHANGELOG_VERSION/$VERSION/" debian/changelog
debian/rules get-orig-source
debuild -us -uc -S

# Build ags binary package in i386 chroot, also use a hook script to copy libraries and licenses
# from the chroot to a folder that is mounted into the chroot via --bindmounts.
DEB_BUILD_OPTIONS="rpath=$ORIGIN/lib32" pbuilder-dist jessie i386 build \
  --buildresult $BASEPATH/ags+libraries \
  --hookdir $BASEPATH/debian/ags+libraries/hooks \
  --bindmounts "$BASEPATH/ags+libraries" \
  $BASEPATH/../ags_$VERSION.dsc

# Get the ags binary out of the binary Debian package and clean up.
cd $BASEPATH/ags+libraries
ar p $BASEPATH/ags+libraries/ags_${VERSION}_i386.deb data.tar.xz | unxz | tar x
sudo cp $BASEPATH/ags+libraries/usr/bin/ags $BASEPATH/ags+libraries/data/ags32
rm -rf $BASEPATH/ags+libraries/ags_* $BASEPATH/ags+libraries/ags-dbg_* $BASEPATH/ags+libraries/usr

# Repeat for amd64.
sed -i -r "5s/.*/BIT=64/" $BASEPATH/debian/ags+libraries/hooks/B00_copy_libs.sh
DEB_BUILD_OPTIONS="rpath=$ORIGIN/lib64" pbuilder-dist jessie amd64 build \
  --buildresult $BASEPATH/ags+libraries \
  --hookdir $BASEPATH/debian/ags+libraries/hooks \
  --bindmounts "$BASEPATH/ags+libraries" \
  $BASEPATH/../ags_$VERSION.dsc

cd $BASEPATH/ags+libraries
ar p $BASEPATH/ags+libraries/ags_${VERSION}_amd64.deb data.tar.xz | unxz | tar x
sudo cp $BASEPATH/ags+libraries/usr/bin/ags $BASEPATH/ags+libraries/data/ags64
rm -rf $BASEPATH/ags+libraries/ags_* $BASEPATH/ags+libraries/ags-dbg_* $BASEPATH/ags+libraries/usr last_operation.log

# Clean up hook script.
sed -i -r "4s/.*/BINDMOUNT=/" $BASEPATH/debian/ags+libraries/hooks/B00_copy_libs.sh
sed -i -r "5s/.*/BIT=32/" $BASEPATH/debian/ags+libraries/hooks/B00_copy_libs.sh

# Copy other files that we want to include.
sudo cp $BASEPATH/debian/copyright $BASEPATH/ags+libraries/data/licenses/ags-copyright
cp $BASEPATH/debian/ags+libraries/startgame $BASEPATH/ags+libraries/
cp $BASEPATH/debian/ags+libraries/README $BASEPATH/ags+libraries/

# Compress
cd $BASEPATH && tar -cf - ags+libraries/ | xz -9 -c - > ags+libraries_$(date +%Y%m%d).tar.xz 

# Delete the ags+libraries folder, because some of the subfolders and files are owned by root.
# Extracting the .tar.xz yields the folder with proper owner/permissions.
sudo rm -rf $BASEPATH/ags+libraries/

# Restore changelog version to prevent unnecessary commits
sed -i -- "s/$VERSION/$CHANGELOG_VERSION/" $BASEPATH/debian/changelog
