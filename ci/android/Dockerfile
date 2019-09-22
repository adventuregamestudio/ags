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

# Get packages
RUN apt-get install \
  autoconf \
  bsdtar \
  build-essential \
  cmake \
  curl \
  git \
  jq \
  libncurses5 \
  libtool \
  pkg-config \
  python

# Get Android NDK
ARG NDK_VERSION=r16b
RUN curl -fLOJ "https://dl.google.com/android/repository/android-ndk-${NDK_VERSION}-linux-x86_64.zip" && \
  bsdtar -f android-ndk-${NDK_VERSION}-linux-x86_64.zip -xvzC /opt && \
  rm android-ndk-${NDK_VERSION}-linux-x86_64.zip

# Make NDK toolchains
RUN export NDK_HOME=/opt/android-ndk-${NDK_VERSION} && \
  for arch in arm mips x86; do \
    echo Building toolchain for $arch; \
    $NDK_HOME/build/tools/make_standalone_toolchain.py --arch $arch --api 14 --install-dir $NDK_HOME/platforms/android-14/$arch; \
  done

# Get Android SDK manager
ARG SDK_TOOLS_VERSION=4333796
RUN curl -flOJ "https://dl.google.com/android/repository/sdk-tools-linux-${SDK_TOOLS_VERSION}.zip" && \
  mkdir /opt/android-sdk && \
  bsdtar -f sdk-tools-linux-${SDK_TOOLS_VERSION}.zip -xvzC /opt/android-sdk && \
  rm sdk-tools-linux-${SDK_TOOLS_VERSION}.zip

# Get OpenJDK 8
ARG OPENJDK_VERSION=openjdk8
RUN curl -fLSs "$(curl -fLSs "https://api.adoptopenjdk.net/v2/info/releases/${OPENJDK_VERSION}?os=linux&arch=x64&release=latest&openjdk_impl=hotspot&type=jdk" | \
  jq -r .binaries[0].binary_link)" | tar -f - -xvzC /opt

# Install SDK packages
ARG BUILD_TOOLS_VERSION=29.0.2
ARG PLATFORM_VERSION=android-16
RUN echo y | JAVA_HOME=$(echo /opt/jdk*) /opt/android-sdk/tools/bin/sdkmanager \
  "build-tools;${BUILD_TOOLS_VERSION}" \
  "platforms;${PLATFORM_VERSION}" \
  platform-tools

# Merge Ant support back in
ARG BUILD_TOOLS_ANT_VERSION=25.2.5
RUN curl -fLOJ "https://dl.google.com/android/repository/tools_r${BUILD_TOOLS_ANT_VERSION}-linux.zip" && \
  bsdtar -f tools_r${BUILD_TOOLS_ANT_VERSION}-linux.zip -xvzkC /opt/android-sdk && \
  rm tools_r${BUILD_TOOLS_ANT_VERSION}-linux.zip

# Get Ant
ARG ANT_VERSION=1.10.7
RUN curl -fLSs "https://www.apache.org/dist/ant/binaries/apache-ant-${ANT_VERSION}-bin.tar.gz" | \
  tar -f - -xvzC /opt

# Build libraries
ARG AGS_ORGANIZATION=adventuregamestudio
ARG AGS_BRANCH=master
RUN cd /tmp && \
  git clone -b "${AGS_BRANCH}" "https://github.com/${AGS_ORGANIZATION}/ags.git" && \
  cd ags/Android/buildlibs && \
  NDK_HOME=/opt/android-ndk-${NDK_VERSION} ./buildall.sh && \
  mv ../nativelibs /opt/ && \
  rm -r /tmp/ags
