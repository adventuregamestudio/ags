FROM ghcr.io/cirruslabs/android-sdk:34-ndk
# CirrusCI Android NDK image uses Ubuntu 22.04
# cirrus ANDROID_NDK_VERSION=26.2.11394342
# cirrus ANDROID_BUILD_TOOLS_VERSION=34.0.0

# we use an older NDK that still supports platform 19 as minSdk
# otherwise we get undefined behavior build warnings
ENV AGS_ANDROID_NDK_VERSION 25.2.9519653
RUN yes | sdkmanager "ndk;$AGS_ANDROID_NDK_VERSION"

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

