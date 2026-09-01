#!/bin/bash

set -e

# change to directory where script is located
cd "$( dirname "${BASH_SOURCE[0]}" )"

# NOTE: this entire script is linux specific
# FIX-ME: we can probably put the flags here and other things to make this script work on macOS and maybe MinGW in MSYS2?
# For now this guards and exits if things are wrong
case "$(uname -s)" in
    Linux)
        ;;
    *)
        echo "Only Linux for now..." >&2
        exit 1
        ;;
esac

# TODO: make this a parameter to this script?
NPROC=4

# NOTE: these config flags are LINUX specific
# We could have them in a platform switch clause if we want to also include macOS and MinGW someday
PREFIX="${PREFIX:-/usr/local}"
SDL2_CMAKE_DIR="$PREFIX/lib/cmake/SDL2"
SDL_CONFIGURE_FLAGS=(
	--enable-shared
	--enable-loadso
	--enable-pulseaudio-shared
	--enable-sndio-shared
	--enable-x11-shared
	--enable-oss=no
	--enable-libsamplerate-shared
	--enable-video-wayland=no
	--enable-directfb-shared
	--enable-rpath=no
)

autotools_build()
{
    local dir="$1"
    shift

    (
        cd "$dir"
        ./autogen.sh
        ./configure "$@"
        make -j"$NPROC"
        make install
    )
}

# The actual build starts here
# The order matters because vorbis depends on libogg
autotools_build ogg --prefix="$PREFIX"
autotools_build vorbis --prefix="$PREFIX"
autotools_build theora --disable-encode --disable-examples --disable-oggtest --prefix="$PREFIX"

(
    cd SDL
    ./configure --prefix="$PREFIX" "${SDL_CONFIGURE_FLAGS[@]}"
    make -j"$NPROC"
    make install
)

# SDL_sound only builds with CMake
(
    cd SDL_sound
    mkdir -p build
    cd build
    cmake -DSDL2_DIR="$SDL2_CMAKE_DIR" -DSDLSOUND_DECODER_MIDI=1 -DCMAKE_INSTALL_PREFIX="$PREFIX" ..
    cmake --build . --parallel "$NPROC"
    make install
)
