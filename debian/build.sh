#!/bin/sh
set -e

# This build script is meant to be run by the docker image in the CI.
# This is why there are some environment variable here that exist by magic.
# The code from here was pulled from the Cirrus CI yaml file.
#
# FIX-ME: make this a more proper script at some point that take command line
# parameters and adjust the GitHub Actions line that uses it.


echo "------------ begin debian/build.sh -----------------"
echo "Running build script in:"
uname -a
echo "Building for architecture:"
dpkg --print-architecture
echo "----------------------------------------------------"

ARCH=$(dpkg --print-architecture)
VERSION=$(awk -F'[ "]+' '$1=="#define" && $2=="ACI_VERSION_STR" { print $3; exit }' Common/ac/def_version.h)
sed -i -s "s/ags \(.*\)(.*)/ags \($VERSION\)\1/" debian/changelog
if [ -n "$RPATH_PREFIX" ]; then
  case $ARCH in
    amd64)
      bit=64
      ;;
    i386)
      bit=32
      ;;
    *)
      echo "Unknown architecture"
      exit 1
      ;;
  esac
  DEB_BUILD_OPTIONS="rpath=$RPATH_PREFIX$bit" DEB_LDFLAGS_MAINT_APPEND=-Wl,--disable-new-dtags fakeroot debian/rules binary
  sed -E "/^BI(NDMOUNT|T)=/d" debian/ags+libraries/hooks/B00_copy_libs.sh | BIT=$bit BINDMOUNT=$(pwd) sh
  ar -p ../ags_${VERSION}_$ARCH.deb data.tar.xz | unxz | tar -f - -xvC data --transform "s/.*ags/ags$bit/" ./usr/bin/ags
  cd data && \
  (cd lib$bit && find . \
    \( \
    -name "libogg.so.*" -o \
    -name "libtheora.so.*" -o \
    -name "libvorbisfile.so.*" -o \
    -name "libvorbis.so.*" \
    \) \
    -exec cp -L -v "/opt/lib/{}" "{}" \;) && \
  tar -cvzf ../data_$ARCH.tar.gz *
else
  fakeroot debian/rules binary
  mv ../ags_${VERSION}_$ARCH.deb .
fi

echo "------------ end of debian/build.sh ----------------"
