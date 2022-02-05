#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
TAR_CMD="bsdtar"
command -v bsdtar >/dev/null 2>&1 || { TAR_CMD="tar" ; }

function build_debug_library_and_runtime {
  echo "building debug library and runtime..."
  pushd "${SCRIPT_DIR}"
  pushd agsplayer && ./gradlew assembleDebug --console=plain && popd
  popd
  echo "done!"
}

function build_release_library_and_runtime {
  echo "building release library and runtime..."
  pushd "${SCRIPT_DIR}"
  pushd agsplayer && ./gradlew assembleRelease --console=plain && popd
  popd
  echo "done!"
}

function rename_apks {
  echo "renaming apks..."
  pushd "${SCRIPT_DIR}"
  version=$(awk -F"[ \"]+" '{ if ($1=="#define" && $2=="ACI_VERSION_STR") { print $3; exit } }' ../Common/core/def_version.h)
  pushd agsplayer/app/build/outputs/apk/debug/
  for apk in $(find -maxdepth 1 -name "*.apk" -type f); do
    mv -v $apk AGS-${version}-${apk#*-}
  done
  popd
  mv agsplayer/app/build/outputs/apk/debug/*.apk ..
  pushd agsplayer/app/build/outputs/apk/release/
  for apk in $(find -maxdepth 1 -name "*.apk" -type f); do
    mv -v $apk AGS-${version}-${apk#*-}
  done
  popd
  mv agsplayer/app/build/outputs/apk/release/*.apk ..
  popd
  echo "done!"
}

function create_libs_archive {
  echo "creating libs archive..."
  pushd "${SCRIPT_DIR}"
  version=$(awk -F"[ \"]+" '{ if ($1=="#define" && $2=="ACI_VERSION_STR") { print $3; exit } }' ../Common/core/def_version.h)
  ${TAR_CMD} -f ../AGS-${version}-android-libs-debug.zip -acv --strip-components 8 agsplayer/app/build/intermediates/stripped_native_libs/debug/out/lib
  ${TAR_CMD} -f ../AGS-${version}-android-libs-release.zip -acv --strip-components 8 agsplayer/app/build/intermediates/stripped_native_libs/release/out/lib
  popd
  echo "done!"
}

function usage
{
   echo "android build script."
   echo
   echo "Syntax: and-build.sh [options]"
   echo "options:"
   echo "build_debug"
   echo "build_release"
   echo "archive_apks"
   echo "archive_libs"
   echo "-h --help  prints this message."
   echo
}

if [[ $# -eq 0 ]]; then
    usage
    exit
fi

while : ; do
  case "$1" in 
    build_debug)
       build_debug_library_and_runtime
       shift 1 ;;
    build_release)
       build_release_library_and_runtime
       shift 1 ;;
    archive_apks)
       rename_apks
       shift 1 ;;    
    archive_libs)
       create_libs_archive
       shift 1 ;;
    -h|--help)
       usage
       break ;;
    *)
       break ;;
  esac
done