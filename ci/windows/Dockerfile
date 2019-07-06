# base will already have Chocolatey installed
FROM cirrusci/windowsservercore:2019

ARG VISUALCPP_BUILD_TOOLS_VERSION=14.0.25420.1
RUN cinst visualcpp-build-tools --version %VISUALCPP_BUILD_TOOLS_VERSION% -y

ARG INNO_SETUP_VERSION=6.0.2
RUN cinst innosetup --version %INNO_SETUP_VERSION% -y
