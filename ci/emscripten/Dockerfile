# This Dockerfile is heavily based on the official EMSDK Dockerfile 
# For more information, see https://github.com/emscripten-core/emsdk/tree/main/docker
FROM ubuntu:noble

# The current used version of emsdk
ARG EMSCRIPTEN_VERSION=3.1.74
ENV EMSDK=/emsdk

#fix TZ docker hang
ENV TZ=America/San_Francisco
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# Install required tools
RUN echo "## Update and install packages" \
    && apt-get -qq -y update \
    # Somewhere in here apt sets up tzdata which asks for your time zone and blocks
    # waiting for the answer which you can't give as docker build doesn't read from
    # the terninal. The env vars set here avoid the interactive prompt and set the TZ.
    && DEBIAN_FRONTEND="noninteractive" TZ="America/San_Francisco" apt-get -qq install -y --no-install-recommends \
        tzdata \
        sudo \
        binutils \
        libxml2 \
        ca-certificates \
        python3 \
        python3-pip \
        wget \
        curl \
        zip \
        unzip \
        file \
        git \
        git-lfs \
        ssh-client \
        build-essential \
        make \
        ant \
        libidn12 \
        cmake \
        openjdk-11-jre-headless \
        libarchive-tools \
        pkg-config \
        ninja-build \
        bash \
    # Standard Cleanup on Debian images
    && apt-get -y clean \
    && apt-get -y autoclean \
    && apt-get -y autoremove \
    && rm -rf /var/lib/apt/lists/* \
    && rm -rf /var/cache/debconf/*-old \
    && rm -rf /usr/share/doc/* \
    && rm -rf /usr/share/man/?? \
    && rm -rf /usr/share/man/??_* \
    && echo "## Done"

# ------------------------------------------------------------------------------

# Install Emscripten 
RUN echo "## Install Emscripten" \
    && git clone https://github.com/emscripten-core/emsdk.git ${EMSDK} \
    && cd ${EMSDK} \
    && ./emsdk install ${EMSCRIPTEN_VERSION} \
    && echo "## Done"

# This generates configuration that contains all valid paths according to installed SDK
# TODO(sbc): We should be able to use just emcc -v here but it doesn't
# currently create the sanity file.
RUN cd ${EMSDK} \
    && echo "## Generate standard configuration" \
    && ./emsdk activate ${EMSCRIPTEN_VERSION} \
    && chmod 777 ${EMSDK}/upstream/emscripten \
    && chmod -R 777 ${EMSDK}/upstream/emscripten/cache \
    && echo "int main() { return 0; }" > hello.c \
    && ${EMSDK}/upstream/emscripten/emcc -c hello.c \
    && cat ${EMSDK}/upstream/emscripten/cache/sanity.txt \
    && echo "## Done"

# Cleanup Emscripten installation and strip some symbols
RUN echo "## Aggressive optimization: Remove debug symbols" \
    && cd ${EMSDK} && . ./emsdk_env.sh \
    # Remove debugging symbols from embedded node (extra 7MB)
    && strip -s `which node` \
    # Tests consume ~80MB disc space
    && rm -fr ${EMSDK}/upstream/emscripten/tests \
    # strip out symbols from clang (~extra 50MB disc space)
    && find ${EMSDK}/upstream/bin -type f -exec strip -s {} + || true \
    && echo "## Done"

# These fallback environment variables are intended for situations where the
# entrypoint is not utilized (as in a derived image) or overridden (e.g. when
# using `--entrypoint /bin/bash` in CLI).
# This corresponds to the env variables set during: `source ./emsdk_env.sh`
ENV EMSDK=/emsdk \
    PATH="/emsdk:/emsdk/upstream/emscripten:/emsdk/node/20.18.0_64bit/bin:${PATH}"

# ------------------------------------------------------------------------------

RUN embuilder.py --lto build libstubs libc libc++ libc++abi sdl2 ogg vorbis
RUN embuilder.py build libstubs libc libc++ libc++abi sdl2 ogg vorbis

# ------------------------------------------------------------------------------
# Use commonly used /src as working directory
WORKDIR /src