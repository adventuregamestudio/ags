ARG FROM_DEBIAN=debian:latest
ARG FROM_PLATFORM=$BUILDPLATFORM
FROM --platform=$FROM_PLATFORM $FROM_DEBIAN

# Take default debconf options
ENV DEBIAN_FRONTEND noninteractive

# Debian Jessie has been moved to archive
RUN echo "deb [trusted=yes] http://archive.debian.org/debian jessie main non-free contrib" > /etc/apt/sources.list && \
    echo 'deb-src [trusted=yes] http://archive.debian.org/debian/ jessie main non-free contrib'  >> /etc/apt/sources.list && \
    echo 'deb [trusted=yes] http://archive.debian.org/debian-security/ jessie/updates main non-free contrib'  >> /etc/apt/sources.list

# Configure apt defaults
ARG APT_CONF_LOCAL=99local
RUN mkdir -p /etc/apt/apt.conf.d && \
  printf 'APT::Get::Assume-Yes "true";\n\
APT::Get::Install-Recommends "false";\n\
APT::Get::Install-Suggests "false";\n' > /etc/apt/apt.conf.d/$APT_CONF_LOCAL

# Upgrade existing packages
RUN apt-get update && apt-get upgrade

# Get curl and ssl for cmake fetches
RUN apt-get install curl libcurl4-openssl-dev libssl-dev

# Get build dependencies
RUN apt-get install \
  build-essential \
  debhelper \
  autoconf \
  dh-autoreconf \
  git \
  libogg-dev \
  libtheora-dev \
  libvorbis-dev \
  pkg-config

# Get SDL2 build dependencies
RUN apt-get install \
  libasound2-dev \
  libdbus-1-dev \
  libegl1-mesa-dev \
  libgl1-mesa-dev \
  libgles2-mesa-dev \
  libglu1-mesa-dev \
  libibus-1.0-dev \
  libpulse-dev \
  libsndio-dev \
  libudev-dev \
  libwayland-dev \
  libx11-dev \
  libxcursor-dev \
  libxext-dev \
  libxi-dev \
  libxinerama-dev \
  libxkbcommon-dev \
  libxrandr-dev \
  libxss-dev \
  libxt-dev \
  libxv-dev \
  libxxf86vm-dev \
  libwayland-dev \
  libxkbcommon-dev 

# Build and install CMake
ARG CMAKE_VERSION=3.24.1
RUN curl -fLsS "https://github.com/Kitware/CMake/releases/download/v$CMAKE_VERSION/cmake-$CMAKE_VERSION.tar.gz" | tar -f - -xvzC /tmp && \
  cd /tmp/cmake-$CMAKE_VERSION && \
  ./configure --system-curl && make -j$(getconf _NPROCESSORS_ONLN) && make install && \
  rm -r /tmp/cmake-$CMAKE_VERSION

# Build newer libogg
ARG LIBOGG_VERSION=1.3.5
RUN curl -fLsS "https://github.com/xiph/ogg/releases/download/v${LIBOGG_VERSION}/libogg-${LIBOGG_VERSION}.tar.xz" --output /tmp/libogg-${LIBOGG_VERSION}.tar.xz && \
  tar --file=/tmp/libogg-${LIBOGG_VERSION}.tar.xz -xvJC /tmp && \
  cd /tmp/libogg-${LIBOGG_VERSION} && \
  ./configure --prefix=/opt && \
  make && \
  make install && \
  rm -r /tmp/libogg-${LIBOGG_VERSION} && \
  rm /tmp/libogg-${LIBOGG_VERSION}.tar.xz

# Build newer libvorbis
ARG LIBVORBIS_VERSION=1.3.7
RUN curl -fLsS "https://github.com/xiph/vorbis/releases/download/v${LIBVORBIS_VERSION}/libvorbis-${LIBVORBIS_VERSION}.tar.gz" --output /tmp/libvorbis-${LIBVORBIS_VERSION}.tar.gz && \
  tar --file=/tmp/libvorbis-${LIBVORBIS_VERSION}.tar.gz -xvzC /tmp && \
  cd /tmp/libvorbis-${LIBVORBIS_VERSION} && \
  ./configure --disable-examples --disable-oggtest --prefix=/opt && \
  make && \
  make install && \
  rm -r /tmp/libvorbis-${LIBVORBIS_VERSION} && \
  rm /tmp/libvorbis-${LIBVORBIS_VERSION}.tar.gz 

# Build and install SDL2
ARG SDL2_VERSION=2.28.2
RUN curl -fLsS "https://github.com/libsdl-org/SDL/archive/refs/tags/release-${SDL2_VERSION}.tar.gz" | tar -f - -xvzC /tmp && \
  cd /tmp/SDL-release-${SDL2_VERSION} && \
  ./configure --enable-shared --enable-loadso --enable-pulseaudio-shared --enable-sndio-shared --enable-x11-shared --enable-oss=no --enable-libsamplerate-shared --enable-video-wayland=no --enable-directfb-shared --enable-rpath=no && \
  make -j$(getconf _NPROCESSORS_ONLN) && make install && \
  ldconfig -n /usr/local/lib && \
  mkdir -p /usr/local/share/doc/libSDL2-2.0/ && cp /tmp/SDL-release-${SDL2_VERSION}/LICENSE.txt /usr/local/share/doc/libSDL2-2.0/copyright && \
  rm -r /tmp/SDL-release-${SDL2_VERSION}
  
# Build newer libtheora - note that encoding support is disabled AND it's directory uses GH repo name instead
ARG LIBTHEORA_VERSION=1.1.1
RUN curl -fLsS "https://github.com/xiph/theora/archive/refs/tags/v${LIBTHEORA_VERSION}.tar.gz" --output /tmp/libtheora-${LIBTHEORA_VERSION}.tar.gz && \
  tar --file=/tmp/libtheora-${LIBTHEORA_VERSION}.tar.gz -xvzC /tmp && \
  cd /tmp/theora-${LIBTHEORA_VERSION} && \
  ./autogen.sh --disable-encode --disable-examples --disable-oggtest --prefix=/opt && \
  make && \
  make install && \
  rm -r /tmp/theora-${LIBTHEORA_VERSION} && \
  rm /tmp/libtheora-${LIBTHEORA_VERSION}.tar.gz 

# Build and install SDL_sound
ARG SDL2_SOUND_VERSION=1507be95c3605e4fd6a48ea4c527e4aa711a1566
RUN cd /tmp && \
  curl -fLsS "https://github.com/icculus/SDL_sound/archive/$SDL2_SOUND_VERSION.tar.gz" --output SDL_sound.tar.gz && \
  tar -xvzf SDL_sound.tar.gz && \
  mv SDL_sound-$SDL2_SOUND_VERSION SDL_sound &&  \
  cd /tmp/SDL_sound  && \
  mkdir /tmp/SDL_sound/build && \
  cd /tmp/SDL_sound/build && \
  cmake -DSDL2_DIR=/usr/local/lib/cmake/SDL2  -DSDLSOUND_DECODER_MIDI=1 ..  && make -j$(getconf _NPROCESSORS_ONLN) && make install && \
  rm /usr/local/lib/libSDL2_sound.*so* && \
  rm -r /tmp/SDL_sound

RUN ldconfig -v