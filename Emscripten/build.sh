#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
pushd "${SCRIPT_DIR}"

command -v emcc >/dev/null 2>&1 || { echo >&2 "Make sure emsdk is installed and activated!  emcc not found. Aborting."; exit 1; }

BUILD_CMD="ninja"
command -v ninja >/dev/null 2>&1 || { BUILD_CMD="make" ; }

mkdir build-release
pushd build-release

if [ ${BUILD_CMD} == "ninja" ]; then
  emcmake cmake -G "Ninja" ../.. -DCMAKE_BUILD_TYPE=Release
else 
  emcmake cmake ../.. -DCMAKE_BUILD_TYPE=Release
fi

if ! ${BUILD_CMD}; then
  exit 1
fi
popd
popd
