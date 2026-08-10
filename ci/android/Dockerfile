FROM ghcr.io/cirruslabs/android-sdk:35-ndk
# CirrusCI Android NDK image uses Ubuntu 24.04
# ANDROID_NDK_VERSION=27.2.12479018
# ANDROID_BUILD_TOOLS_VERSION=35.0.0

# we use an older NDK that still supports platform 19 as minSdk
# otherwise we get undefined behavior build warnings
ENV AGS_ANDROID_NDK_VERSION 25.2.9519653
RUN yes | sdkmanager "ndk;$AGS_ANDROID_NDK_VERSION"

ENV AGS_ANDROID_BUILD_TOOLS_VERSION 34.0.0
RUN yes | sdkmanager "build-tools;$AGS_ANDROID_BUILD_TOOLS_VERSION"

ARG APT_CONF_LOCAL=99local
RUN mkdir -p /etc/apt/apt.conf.d && \
  printf 'APT::Get::Assume-Yes "true";\n\
APT::Get::Install-Recommends "false";\n\
APT::Get::Install-Suggests "false";\n' > /etc/apt/apt.conf.d/$APT_CONF_LOCAL

# Upgrade existing packages
RUN apt-get update && apt-get upgrade

# Get cmake and ninja (this gets cmake 3.27.4 and ninja 1.11)
RUN apt-get install \
                cmake \
                ninja-build \
                libarchive-tools

