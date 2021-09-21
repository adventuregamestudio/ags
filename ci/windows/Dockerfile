# base will already have Chocolatey installed
FROM ericoporto/min-ags-dev-env:1.0.0

# if no temp folder exists by default, create it
RUN IF exist %TEMP%\nul ( echo %TEMP% ) ELSE ( mkdir %TEMP% )

# Windows 8.0 SDK (but only install the .NET 4.5 SDK)
RUN pushd %TEMP% && \
  curl -fLO http://download.microsoft.com/download/F/1/3/F1300C9C-A120-4341-90DF-8A52509B23AC/standalonesdk/sdksetup.exe && \
  start /wait sdksetup /ceip off /features OptionID.NetFxSoftwareDevelopmentKit /quiet /norestart && \
  popd && \
  mkdir empty && \
  robocopy empty %TEMP% /MIR > nul & \
  rd /s /q empty

ARG NUGET_VERSION=5.7.0
ARG INNO_SETUP_VERSION=6.0.5
RUN cinst nuget.commandline --version %NUGET_VERSION% -y && \
  cinst innosetup --version %INNO_SETUP_VERSION% -y && \
  mkdir empty && \
  robocopy empty %TEMP% /MIR > nul & \
  rd /s /q empty

RUN pushd %TEMP% && \
  mkdir Lib\Xiph && \
  pushd Lib\Xiph && \
  nuget install ericoporto.xiph-for-ags -Version 2020.12.12.1 && \
  popd && \
  popd && \
  mkdir Lib\Xiph && \
  pushd Lib\Xiph && \
  copy %TEMP%\Lib\Xiph\ericoporto.xiph-for-ags.2020.12.12.1\native\lib\libogg_static.lib libogg_static.lib && \
  copy %TEMP%\Lib\Xiph\ericoporto.xiph-for-ags.2020.12.12.1\native\lib\libtheora_static.lib libtheora_static.lib && \
  copy %TEMP%\Lib\Xiph\ericoporto.xiph-for-ags.2020.12.12.1\native\lib\libvorbis_static.lib libvorbis_static.lib && \
  copy %TEMP%\Lib\Xiph\ericoporto.xiph-for-ags.2020.12.12.1\native\lib\libvorbisfile_static.lib libvorbisfile_static.lib && \
  popd && \
  rd /s /q %TEMP%\Lib

ARG IRRKLANG_VERSION=1.6.0
RUN curl -fLSs http://www.ambiera.at/downloads/irrKlang-32bit-%IRRKLANG_VERSION%.zip | tar -f - -xvzC %TEMP% irrKlang-%IRRKLANG_VERSION%/bin/dotnet-4/*.dll && \
  mkdir Lib\irrKlang && \
  move %TEMP%\irrKlang-%IRRKLANG_VERSION%\bin\dotnet-4\*.dll Lib\irrKlang\ && \
  rd /s /q %TEMP%\irrKlang-%IRRKLANG_VERSION%

RUN curl -fLSs https://archive.org/download/dxsdk_aug2007/dxsdk_aug2007.exe | tar -f - -xvzC %TEMP% && \
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
