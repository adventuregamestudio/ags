# base will already have Chocolatey installed
FROM cirrusci/windowsservercore:2019

# no temp folder exists by default
RUN mkdir %TEMP%

ARG VISUALCPP_BUILD_TOOLS_VERSION=14.0.25420.1
RUN cinst visualcpp-build-tools --version %VISUALCPP_BUILD_TOOLS_VERSION% -y

# Windows 8.0 SDK (but only install the .NET 4.5 SDK)
RUN cd %TEMP% && \
  curl -fLO http://download.microsoft.com/download/F/1/3/F1300C9C-A120-4341-90DF-8A52509B23AC/standalonesdk/sdksetup.exe && \
  start /wait sdksetup /ceip off /features OptionID.NetFxSoftwareDevelopmentKit /quiet /norestart

ARG NUGET_VERSION=5.1.0
RUN cinst nuget.commandline --version %NUGET_VERSION% -y

ARG INNO_SETUP_VERSION=6.0.2
RUN cinst innosetup --version %INNO_SETUP_VERSION% -y

ARG CMAKE_VERSION=3.14.5
RUN cinst cmake --version %CMAKE_VERSION% --installargs ADD_CMAKE_TO_PATH=System -y

RUN pushd %TEMP% && \
  git clone https://github.com/Microsoft/vcpkg.git && \
  pushd vcpkg && \
  if not exist vcpkg.exe call bootstrap-vcpkg.bat && \
  vcpkg install libogg:x86-windows-static libtheora:x86-windows-static libvorbis:x86-windows-static && \
  popd && \
  popd && \
  mkdir Lib\Xiph && \
  copy %TEMP%\vcpkg\installed\x86-windows-static\lib\ogg.lib Lib\Xiph\libogg_static.lib && \
  copy %TEMP%\vcpkg\installed\x86-windows-static\lib\theora.lib Lib\Xiph\libtheora_static.lib && \
  copy %TEMP%\vcpkg\installed\x86-windows-static\lib\vorbis.lib Lib\Xiph\libvorbis_static.lib && \
  copy %TEMP%\vcpkg\installed\x86-windows-static\lib\vorbisfile.lib Lib\Xiph\libvorbisfile_static.lib

ARG IRRKLANG_VERSION=1.6.0
RUN curl -fLSs http://www.ambiera.at/downloads/irrKlang-32bit-%IRRKLANG_VERSION%.zip | tar -f - -xvzC %TEMP% irrKlang-%IRRKLANG_VERSION%/bin/dotnet-4/*.dll && \
  mkdir Lib\irrKlang && \
  move %TEMP%\irrKlang-%IRRKLANG_VERSION%\bin\dotnet-4\*.dll Lib\irrKlang\ && \
  rd /s /q %TEMP%\irrKlang-%IRRKLANG_VERSION%

RUN curl -fLSs https://download.microsoft.com/download/3/3/f/33f1af6e-c61b-4f14-a0de-3e9096ed4b3a/dxsdk_aug2007.exe | tar -f - -xvzC %TEMP% && \
  tar -f %TEMP%\dxsdk_aug2007.exe -xvzC %TEMP% Lib/x86/*.lib && \
  mkdir Lib\DirectX && \
  move %TEMP%\Lib\x86\*.lib Lib\DirectX\ && \
  del %TEMP%\dxsdk_aug2007.exe && \
  rd /s /q %TEMP%\Lib

RUN mkdir Redist && \
  cd Redist && \
  curl -fLOJ https://download.microsoft.com/download/6/A/A/6AA4EDFF-645B-48C5-81CC-ED5963AEAD48/vc_redist.x86.exe

ARG ALLEGRO_REPO=adventuregamestudio/lib-allegro
ARG ALLEGRO_RELEASES=v4.4.3.1-agspatch-3
RUN for %a in (%ALLEGRO_RELEASES%) do \
  mkdir Source\Allegro\%a && \
  curl -fLSs "https://github.com/%ALLEGRO_REPO%/archive/%a.tar.gz" | tar -f - -xvzC Source\Allegro\%a --strip-components 1 && \
  mkdir Lib\Allegro\%a && \
  pushd Lib\Allegro\%a && \
  curl -fLOJ https://github.com/%ALLEGRO_REPO%/releases/download/%a/alleg-debug-static-mt.lib & \
  curl -fLOJ https://github.com/%ALLEGRO_REPO%/releases/download/%a/alleg-debug-static.lib & \
  curl -fLOJ https://github.com/%ALLEGRO_REPO%/releases/download/%a/alleg-static-mt.lib & \
  curl -fLOJ https://github.com/%ALLEGRO_REPO%/releases/download/%a/alleg-static.lib & \
  popd

# delete all temp Files
RUN mkdir empty && \
  robocopy empty %TEMP% /MIR > nul & \
  rd /s /q empty
