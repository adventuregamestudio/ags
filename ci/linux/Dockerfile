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

# Get curl
RUN apt-get install curl

# Get build dependencies
RUN apt-get install \
  build-essential \
  debhelper \
  git \
  libaldmb1-dev \
  libogg-dev \
  libsdl2-dev \
  libtheora-dev \
  libvorbis-dev \
  pkg-config

# Build newer libogg
ARG LIBOGG_VERSION=1.3.4
RUN curl -fLsS "https://downloads.xiph.org/releases/ogg/libogg-$LIBOGG_VERSION.tar.gz" | tar -f - -xvzC /tmp && \
  cd /tmp/libogg-$LIBOGG_VERSION && \
  ./configure --prefix=/opt && \
  make && \
  make install && \
  rm -r /tmp/libogg-$LIBOGG_VERSION

# Build newer libvorbis
ARG LIBVORBIS_VERSION=1.3.7
RUN curl -fLsS "https://downloads.xiph.org/releases/vorbis/libvorbis-$LIBVORBIS_VERSION.tar.xz" | tar -f - -xvJC /tmp && \
  cd /tmp/libvorbis-$LIBVORBIS_VERSION && \
  ./configure --disable-examples --disable-oggtest --prefix=/opt && \
  make && \
  make install && \
  rm -r /tmp/libvorbis-$LIBVORBIS_VERSION

# Build newer libtheora - note that encoding support is disabled
ARG LIBTHEORA_VERSION=1.1.1
RUN curl -fLsS "https://downloads.xiph.org/releases/theora/libtheora-$LIBTHEORA_VERSION.tar.bz2" | tar -f - -xvjC /tmp && \
  cd /tmp/libtheora-$LIBTHEORA_VERSION && \
  ./configure --disable-encode --disable-examples --disable-oggtest --prefix=/opt && \
  make && \
  make install && \
  rm -r /tmp/libtheora-$LIBTHEORA_VERSION

# Build and install CMake
ARG CMAKE_VERSION=3.14.5
RUN curl -fLsS "https://github.com/Kitware/CMake/releases/download/v$CMAKE_VERSION/cmake-$CMAKE_VERSION.tar.gz" | tar -f - -xvzC /tmp && \
  cd /tmp/cmake-$CMAKE_VERSION && \
  ./configure && make && make install && \
  rm -r /tmp/cmake-$CMAKE_VERSION
