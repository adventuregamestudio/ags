FROM cirrusci/android-sdk:30-ndk
# CirrusCI Android NDK image uses Ubuntu 20.04
# ANDROID_NDK_VERSION=21.3.6528147
# ANDROID_BUILD_TOOLS_VERSION=30.0.2

ARG APT_CONF_LOCAL=99local
RUN mkdir -p /etc/apt/apt.conf.d && \
  printf 'APT::Get::Assume-Yes "true";\n\
APT::Get::Install-Recommends "false";\n\
APT::Get::Install-Suggests "false";\n' > /etc/apt/apt.conf.d/$APT_CONF_LOCAL

# Upgrade existing packages
RUN apt-get update && apt-get upgrade

# Get cmake and ninja (this gets cmake 3.16.3 and ninja 1.10)
RUN apt-get install \
                cmake \
                ninja-build \
                libarchive-tools

