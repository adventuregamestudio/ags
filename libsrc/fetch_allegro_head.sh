#!/bin/bash

set -e

rm -rf tmp
git clone --recursive https://github.com/liballeg/allegro5.git tmp

pushd tmp
git checkout 4.4
NAME=$(git log -1 --pretty=format:'allegro-%ad-g%h' --date=format:'%Y%m%d')
popd
mv tmp $NAME

tar cjf $NAME.tar.bz2 --exclude '.git*' $NAME
