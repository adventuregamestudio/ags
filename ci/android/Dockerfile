FROM cirrusci/android-sdk:33-ndk
# CirrusCI Android NDK image uses Ubuntu 22.04
# ANDROID_NDK_VERSION=25.1.8937393
# ANDROID_BUILD_TOOLS_VERSION=33.0.1

ARG APT_CONF_LOCAL=99local
RUN mkdir -p /etc/apt/apt.conf.d && \
  printf 'APT::Get::Assume-Yes "true";\n\
APT::Get::Install-Recommends "false";\n\
APT::Get::Install-Suggests "false";\n' > /etc/apt/apt.conf.d/$APT_CONF_LOCAL

# Upgrade existing packages
RUN apt-get update && apt-get upgrade

# Get cmake and ninja (this gets cmake 3.22.1 and ninja 1.10)
RUN apt-get install \
                cmake \
                ninja-build \
                libarchive-tools

