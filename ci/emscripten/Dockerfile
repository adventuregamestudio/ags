# The current used version of emsdk
FROM emscripten/emsdk:3.1.28

#fix TZ docker hang
ENV TZ=Europe/London
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# Install required tools
RUN apt-get update && apt-get install -y tzdata libarchive-tools git pkg-config curl build-essential cmake ninja-build bash python3 python3-pip

RUN embuilder.py build --lto libstubs libc libc++ libc++abi sdl2 ogg vorbis
RUN embuilder.py build libstubs libc libc++ libc++abi sdl2 ogg vorbis
