#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
pushd "${SCRIPT_DIR}"
mkdir emscripten
pushd emscripten
git clone https://github.com/emscripten-core/emsdk.git
pushd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
popd
popd
mkdir build-release
pushd build-release
emcmake cmake ../.. -DCMAKE_BUILD_TYPE=Release
if ! make; then
  exit 1
fi
popd
popd
