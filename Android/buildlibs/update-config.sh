#!/bin/bash

wget -c http://git.savannah.gnu.org/cgit/config.git/plain/config.sub -O ../config.sub
wget -c http://git.savannah.gnu.org/cgit/config.git/plain/config.guess -O ../config.guess

cp ../config.sub $1/config.sub
cp ../config.guess $1/config.guess
