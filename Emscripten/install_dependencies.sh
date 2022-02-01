#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
pushd "${SCRIPT_DIR}"

mkdir emscripten
pushd emscripten
git clone https://github.com/emscripten-core/emsdk.git
pushd emsdk
./emsdk install 3.1.2
./emsdk activate 3.1.2
source ./emsdk_env.sh
popd
popd

popd

echo "you can now run build.sh to build ags!"