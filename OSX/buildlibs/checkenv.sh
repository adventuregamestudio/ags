#!/bin/sh

if [ "$(xcode-select --print-path)" != "/Applications/Xcode.app/Contents/Developer" ]; then
    echo "xcode-select path must be set to '/Applications/Xcode.app/Contents/Developer'"
    echo "try 'sudo xcode-select --reset'"
    exit 1
fi

for bin in curl pkg-config autoconf automake aclocal libtoolize cmake xcodebuild; do
    if ! `which -s "$bin"`; then
        if [ "$bin" = "libtoolize" ] && `which -s "g$bin"`; then
            continue
        else
            echo "error: require $bin"
            echo "try 'brew install curl pkg-config autoconf automake libtool'"
            exit 1
        fi
    fi
done

exit 0
