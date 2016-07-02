#!/bin/bash

set -e

rm -rf tmp
git clone --progress https://git.xiph.org/theora.git tmp

pushd tmp
NAME=$(git log -1 --pretty=format:'libtheora-%ad-g%h' --date=format:'%Y%m%d')
popd
mv tmp $NAME

tar cjf $NAME.tar.bz2 --exclude '.git*' $NAME
