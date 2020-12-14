ARG FROM_DEBIAN=debian:latest
FROM $FROM_DEBIAN

# Take default debconf options
ENV DEBIAN_FRONTEND noninteractive

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
  git \
  libaldmb1-dev \
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
  libxxf86vm-dev

# Build and install CMake
ARG CMAKE_VERSION=3.14.5
RUN curl -fLsS "https://github.com/Kitware/CMake/releases/download/v$CMAKE_VERSION/cmake-$CMAKE_VERSION.tar.gz" | tar -f - -xvzC /tmp && \
  cd /tmp/cmake-$CMAKE_VERSION && \
  ./configure --system-curl && make && make install && \
  rm -r /tmp/cmake-$CMAKE_VERSION

# Build and install SDL2
ARG SDL2_VERSION=2.0.12
RUN curl -fLsS "https://www.libsdl.org/release/SDL2-$SDL2_VERSION.tar.gz" | tar -f - -xvzC /tmp && \
  cd /tmp/SDL2-$SDL2_VERSION && \
  ./configure && make && make install && \
  rm -r /tmp/SDL2-$SDL2_VERSION

# Build and install SDL_sound
ARG SDL2_SOUND_VERSION=997e90562b35
RUN cd /tmp && \
  curl -fLsS "https://hg.icculus.org/icculus/SDL_sound/archive/$SDL2_SOUND_VERSION.tar.gz" --output SDL_sound.tar.gz && \
  tar -xvzf SDL_sound.tar.gz && \
  mv SDL_sound-$SDL2_SOUND_VERSION SDL_sound &&  \
  cd /tmp/SDL_sound  && \
  mkdir /tmp/SDL_sound/build && \
  cd /tmp/SDL_sound/build && \
  cmake ..  && make && make install && \
  rm -r /tmp/SDL_sound
