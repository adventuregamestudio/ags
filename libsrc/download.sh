#!/bin/bash

set -e

# change to directory where script is located
cd "$( dirname "${BASH_SOURCE[0]}" )"

function get {
    local URL=$1
    local FILENAME=$2
    if [[ ! -f $FILENAME ]]; then
        echo
        echo ${FILENAME}: $URL
        echo
        curl -L $URL --output ${FILENAME}
    fi
}

function tarextract {
    local LIBNAME=$1
    if [ -d "$LIBNAME" ]; then
        echo "${LIBNAME}: is already present, delete it to rextract it."
        return;
    fi
    mkdir "$LIBNAME"
    bsdtar -f "$LIBNAME.tar.gz" -xvzC "$LIBNAME" --strip-components 1
}

LIBOGG_VERSION=1.3.5
get https://github.com/xiph/ogg/archive/refs/tags/v${LIBOGG_VERSION}.tar.gz ogg.tar.gz

LIBVORBIS_VERSION=84c023699cdf023a32fa4ded32019f194afcdad0
get https://github.com/xiph/vorbis/archive/${LIBVORBIS_VERSION}.tar.gz vorbis.tar.gz

LIBTHEORA_VERSION=7180717276af1ebc7da15c83162d6c5d6203aabf
get https://github.com/xiph/theora/archive/${LIBTHEORA_VERSION}.tar.gz theora.tar.gz

SDLSOUND_VERSION=1507be95c3605e4fd6a48ea4c527e4aa711a1566
get https://github.com/icculus/SDL_sound/archive/${SDLSOUND_VERSION}.tar.gz SDL_sound.tar.gz

SDL_VERSION=2.28.2
# Framework for macOS
get https://github.com/libsdl-org/SDL/releases/download/release-${SDL_VERSION}/SDL2-${SDL_VERSION}.dmg SDL2-Framework.dmg
# Full code for iOS
get https://github.com/libsdl-org/SDL/archive/refs/tags/release-${SDL_VERSION}.tar.gz SDL.tar.gz

if ! shasum --check sha1sums; then
    echo "Checksum failed, are downloads ok?" >&2
    exit 1
fi

tarextract ogg
tarextract vorbis
tarextract theora
tarextract SDL_sound
tarextract SDL
