#!/bin/bash
set -e

#    This script has the command line stuff to pick the resulting binary files
# from the web build and packaging them like the Editor expects it from the web
# bundle.
#    This script takes one argument, which is the directory where the build
# artifacts from the web engine are expected to be.

BUILD_DIR="${1:-build}"
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
ROOT_DIR="${SCRIPT_DIR}/.."
TAR_CMD="bsdtar"
command -v bsdtar >/dev/null 2>&1 || { TAR_CMD="tar" ; }

if [ ! -d "${BUILD_DIR}" ]; then
  echo "Error: build directory '${BUILD_DIR}' does not exist."
  exit 1
fi

if [ ! -f "${BUILD_DIR}/ags.wasm" ]; then
  echo "Error: can't find ags.wasm in '${BUILD_DIR}'."
  exit 1
fi

if [ ! -f "${BUILD_DIR}/ags.js" ]; then
  echo "Error: can't find ags.js in '${BUILD_DIR}."
  exit 1
fi

VERSION=$(awk -F'[ "]+' '
  $1=="#define" && $2=="ACI_VERSION_STR" {
    print $3;
    exit
  }' "${ROOT_DIR}/Common/ac/def_version.h")
ARCHIVE_NAME="ags_${VERSION}_web.tar.gz"

TMP_DIR="$(mktemp -d)"

pushd "${SCRIPT_DIR}"
cp my_game_files.js "${TMP_DIR}/"
pushd "${BUILD_DIR}"
mv ags.js ags.wasm ags.html "${TMP_DIR}/"
popd && popd
mv "${TMP_DIR}/ags.html" "${TMP_DIR}/index.html"

${TAR_CMD} -f "${ARCHIVE_NAME}" -cvzC "${TMP_DIR}" \
  index.html ags.js ags.wasm my_game_files.js

rm -rf "${TMP_DIR}"

mv --verbose "${ARCHIVE_NAME}" "${ROOT_DIR}/${ARCHIVE_NAME}"

echo "Packaged web build as: '${ARCHIVE_NAME}' at '${ROOT_DIR}'"
