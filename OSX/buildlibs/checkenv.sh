#!/bin/bash

set -e

if [[ $('xcode-select' --print-path) != "/Applications/Xcode.app/Contents/Developer" ]]; then
    echo "xcode-select path must be set to \"/Applications/Xcode.app/Contents/Developer\""
    echo "try \"sudo xcode-select --reset\""
    exit 1
fi

for bin in curl pkg-config autoconf automake aclocal glibtoolize cmake xcodebuild; do
 path_to_executable=$(which $bin)
 if [ ! -x "$path_to_executable" ] ; then
    echo "error: require $bin"
    echo "try \"brew install curl pkg-config autoconf automake libtool\""
    exit 1
 fi
done

exit 0