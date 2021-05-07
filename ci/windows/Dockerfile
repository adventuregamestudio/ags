# base will already have Chocolatey installed
FROM cirrusci/windowsservercore:2019

# if no temp folder exists by default, create it
RUN IF exist %TEMP%\nul ( echo %TEMP% ) ELSE ( mkdir %TEMP% )

ARG VISUALCPP_BUILD_TOOLS_VERSION=14.0.25420.1
RUN cinst visualcpp-build-tools --version %VISUALCPP_BUILD_TOOLS_VERSION% -y && \
  mkdir empty && \
  robocopy empty %TEMP% /MIR > nul & \
  rd /s /q empty

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
ARG CMAKE_VERSION=3.19.1
RUN cinst nuget.commandline --version %NUGET_VERSION% -y && \
  cinst innosetup --version %INNO_SETUP_VERSION% -y && \
  cinst cmake --version %CMAKE_VERSION% --installargs ADD_CMAKE_TO_PATH=System -y && \
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

RUN mkdir Redist && \
  cd Redist && \
  curl -fLOJ https://download.microsoft.com/download/6/A/A/6AA4EDFF-645B-48C5-81CC-ED5963AEAD48/vc_redist.x86.exe

ARG SDL_VERSION=2.0.12
RUN mkdir Lib\SDL2 && \
  curl -fLOJ "https://www.libsdl.org/release/SDL2-devel-%SDL_VERSION%-VC.zip" && \
  tar -f SDL2-devel-%SDL_VERSION%-VC.zip -xvzC Lib\SDL2\ --strip-components 1  && \
  del /f SDL2-devel-%SDL_VERSION%-VC.zip  && \
  echo set^(SDL2_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/include"^) > "Lib\SDL2\sdl2-config.cmake" && \
  echo # Support both 32 and 64 bit builds >> "Lib\SDL2\sdl2-config.cmake" && \
  echo if ^(${CMAKE_SIZEOF_VOID_P} MATCHES 8^) >> "Lib\SDL2\sdl2-config.cmake" && \
  echo   set^(SDL2_LIBRARIES "${CMAKE_CURRENT_LIST_DIR}/lib/x64/SDL2.lib;${CMAKE_CURRENT_LIST_DIR}/lib/x64/SDL2main.lib"^)  >> "Lib\SDL2\sdl2-config.cmake" && \
  echo else ^(^)  >> "Lib\SDL2\sdl2-config.cmake" && \
  echo   set^(SDL2_LIBRARIES "${CMAKE_CURRENT_LIST_DIR}/lib/x86/SDL2.lib;${CMAKE_CURRENT_LIST_DIR}/lib/x86/SDL2main.lib"^) >> "Lib\SDL2\sdl2-config.cmake" && \
  echo endif ^(^)  >> "Lib\SDL2\sdl2-config.cmake" && \
  echo string^(STRIP "${SDL2_LIBRARIES}" SDL2_LIBRARIES^) >> "Lib\SDL2\sdl2-config.cmake" 
 
ARG SDL_SOUND_VERSION=997e90562b35 
RUN mkdir Lib\SDL_sound && \
  echo "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86 ^&^& pushd Lib\SDL_sound\build ^&^& msbuild SDL_sound.sln /p:PlatformToolset=v140 /p:Configuration=Release /p:Platform=Win32 /maxcpucount /nologo ^&^& popd > sdlsoundvcbuild.bat && \
  mkdir Lib\SDL_sound\build  && \	
  curl -fLSs "https://hg.icculus.org/icculus/SDL_sound/archive/%SDL_SOUND_VERSION%.tar.gz" | tar -f - -xvzC Lib\SDL_sound --strip-components 1 && \
  set SDL2_DIR=%cd%\Lib\SDL2 && \
  cmake -DCMAKE_SYSTEM_VERSION=8.1 -S Lib\SDL_sound -B Lib\SDL_sound\build -G "Visual Studio 14 2015" -T"v140" -A"Win32" -DCMAKE_PREFIX_PATH=Lib\SDL2 && \
  sdlsoundvcbuild.bat && \
  copy Lib\SDL_sound\build\Release\SDL2_sound-static.lib Lib\SDL_sound\build\Release\SDL_sound.lib 

