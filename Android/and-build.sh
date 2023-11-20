#!/bin/bash
set -e
#   Exit immediately if a simple command exits with a non-zero status, unless
# the command that fails is part of an until or  while loop, part of an
# if statement, part of a && or || list, or if the command's return status
# is being inverted using !.

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
TAR_CMD="bsdtar"
command -v bsdtar >/dev/null 2>&1 || { TAR_CMD="tar" ; }

function prepare_package {
  set -e
  echo "Do this only before any build, or after a full clean."
  echo "doing preparations..."
  pushd "${SCRIPT_DIR}"
  [[ -d package_debug ]] && rm -rf package_debug
  [[ -d package_release ]] && rm -rf package_release
  mkdir package_debug
  mkdir package_release
  cp -r library package_debug/library
  cp -r misc package_debug/misc
  cp -r mygame package_debug/mygame
  cp -r gradle package_debug/gradle
  cp -r plugins package_debug/plugins
  
  cp -r library package_release/library
  cp -r misc package_release/misc
  cp -r mygame package_release/mygame
  cp -r gradle package_release/gradle
  cp -r plugins package_release/plugins
  
  mkdir package_release/mygame/gradle
  mkdir package_release/mygame/gradle/wrapper
  mkdir package_debug/mygame/gradle
  mkdir package_debug/mygame/gradle/wrapper
  
  cp agsplayer/gradle/wrapper/gradle-wrapper.jar package_release/mygame/gradle/wrapper/
  cp agsplayer/gradle/wrapper/gradle-wrapper.properties package_release/mygame/gradle/wrapper/
    
  cp agsplayer/gradle/wrapper/gradle-wrapper.jar package_debug/mygame/gradle/wrapper/
  cp agsplayer/gradle/wrapper/gradle-wrapper.properties package_debug/mygame/gradle/wrapper/
  
  popd
  echo "done!"
}

function build_debug_library_and_runtime {
  set -e
  echo "building debug library and runtime..."
  pushd "${SCRIPT_DIR}"
  pushd agsplayer 
  ./gradlew assembleDebug --console=plain 
  popd
  popd
  echo "done!"
}

function build_release_library_and_runtime {
  set -e
  echo "building release library and runtime..."
  pushd "${SCRIPT_DIR}"
  pushd agsplayer 
  ./gradlew assembleRelease --console=plain 
  popd
  popd
  echo "done!"
}

function rename_apks {
  set -e
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

function create_proj_archive {
  set -e
  echo "creating libs archive..."
  pushd "${SCRIPT_DIR}"
  version=$(awk -F"[ \"]+" '{ if ($1=="#define" && $2=="ACI_VERSION_STR") { print $3; exit } }' ../Common/core/def_version.h)
  cp -rf agsplayer/app/build/intermediates/stripped_native_libs/debug/out/lib/* package_debug/library/runtime/libs
  cp -rf agsplayer/app/build/intermediates/stripped_native_libs/release/out/lib/* package_release/library/runtime/libs
  ${TAR_CMD} -f ../AGS-${version}-android-proj-debug.zip -acv --strip-components 1 package_debug
  ${TAR_CMD} -f ../AGS-${version}-android-proj-release.zip -acv --strip-components 1 package_release
  popd
  echo "done!"
}

function usage
{
   echo "android build script."
   echo
   echo "Syntax: and-build.sh [options]"
   echo "options:"
   echo "  prepare"
   echo "  build_debug"
   echo "  build_release"
   echo "  archive_apks"
   echo "  archive_project"
   echo "  -h --help  prints this message."
   echo
}

if [[ $# -eq 0 ]]; then
    usage
    exit
fi

while : ; do
  case "$1" in
    prepare)
       prepare_package
       shift 1 ;;
    build_debug)
       build_debug_library_and_runtime
       shift 1 ;;
    build_release)
       build_release_library_and_runtime
       shift 1 ;;
    archive_apks)
       rename_apks
       shift 1 ;;
    archive_project)
       create_proj_archive
       shift 1 ;;
    -h|--help)
       usage
       break ;;
    *)
       break ;;
  esac
done
