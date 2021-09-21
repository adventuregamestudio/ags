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
  autoconf \
  dh-autoreconf \
  git \
  libaldmb1-dev \
  libogg-dev \
  libsdl2-dev \
  libtheora-dev \
  libvorbis-dev \
  pkg-config

# Build newer libogg
ARG LIBOGG_VERSION=1.3.4
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

# Build and install CMake
ARG CMAKE_VERSION=3.14.5
RUN curl -fLsS "https://github.com/Kitware/CMake/releases/download/v$CMAKE_VERSION/cmake-$CMAKE_VERSION.tar.gz" | tar -f - -xvzC /tmp && \
  cd /tmp/cmake-$CMAKE_VERSION && \
  ./configure && make && make install && \
  rm -r /tmp/cmake-$CMAKE_VERSION
