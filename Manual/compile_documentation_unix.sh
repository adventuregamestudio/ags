#!/bin/sh

if test -z $(which wx-config)
  then
    echo "No wx-config found, install wxWidgets development files, on Debian/Ubuntu: libwxbase2.8-dev or libwxgtk2.8-dev"
    exit 2
fi

echo "Building tex2rtf..."
make --directory=tex2rtf/src --file=makefile.unx

echo "Executing tex2rtf..."
tex2rtf/src/tex2rtf ags.tex ags.htm -html -sync

echo "Replacing some strings in html files..."
sed -i 's/ILBRK/<br>/g' *.htm
sed -i 's/GTSS/>/g' *.htm
sed -i 's/LTSS/</g' *.htm

echo "Copying results to ./html..."
mkdir -p html
cp -r *.htm htmlfiles/*.htm *.gif images html
