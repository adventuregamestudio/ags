# base will already have Chocolatey installed
FROM ghcr.io/ericoporto/min-ags-dev-env:2.0.16

# if no temp folder exists by default, create it
RUN IF exist %TEMP%\nul ( echo %TEMP% ) ELSE ( mkdir %TEMP% )

# Windows 10.0.10240 SDK (but only install the .NET 4.6 SDK)
RUN pushd %TEMP% && \
  curl -fLO https://download.microsoft.com/download/E/1/F/E1F1E61E-F3C6-4420-A916-FB7C47FBC89E/standalonesdk/sdksetup.exe && \
  start /wait sdksetup /ceip off /features OptionID.NetFxSoftwareDevelopmentKit /quiet /norestart && \
  popd && \
  mkdir empty && \
  robocopy empty %TEMP% /MIR > nul & \
  rd /s /q empty

ARG NUGET_VERSION=6.12.1
ARG INNO_SETUP_VERSION=6.2.2
RUN choco install nuget.commandline --version %NUGET_VERSION% -y && \
  choco install innosetup --version %INNO_SETUP_VERSION% -y && \
  mkdir empty && \
  robocopy empty %TEMP% /MIR > nul & \
  rd /s /q empty

RUN pushd %TEMP% && \
  mkdir Lib\Xiph && \
  pushd Lib\Xiph && \
  nuget install ericoporto.xiph-for-ags -Version 2022.12.23 && \
  popd && \
  popd && \
  mkdir Lib\Xiph && \
  mkdir Lib\Xiph\x86 && \
  pushd Lib\Xiph\x86 && \
  copy %TEMP%\Lib\Xiph\ericoporto.xiph-for-ags.2022.12.23\native\lib\x86\libogg_static.lib libogg_static.lib && \
  copy %TEMP%\Lib\Xiph\ericoporto.xiph-for-ags.2022.12.23\native\lib\x86\libtheora_static.lib libtheora_static.lib && \
  copy %TEMP%\Lib\Xiph\ericoporto.xiph-for-ags.2022.12.23\native\lib\x86\libvorbis_static.lib libvorbis_static.lib && \
  copy %TEMP%\Lib\Xiph\ericoporto.xiph-for-ags.2022.12.23\native\lib\x86\libvorbisfile_static.lib libvorbisfile_static.lib && \
  popd && \
  mkdir Lib\Xiph\x64 && \
  pushd Lib\Xiph\x64 && \
  copy %TEMP%\Lib\Xiph\ericoporto.xiph-for-ags.2022.12.23\native\lib\x64\libogg_static.lib libogg_static.lib && \
  copy %TEMP%\Lib\Xiph\ericoporto.xiph-for-ags.2022.12.23\native\lib\x64\libtheora_static.lib libtheora_static.lib && \
  copy %TEMP%\Lib\Xiph\ericoporto.xiph-for-ags.2022.12.23\native\lib\x64\libvorbis_static.lib libvorbis_static.lib && \
  copy %TEMP%\Lib\Xiph\ericoporto.xiph-for-ags.2022.12.23\native\lib\x64\libvorbisfile_static.lib libvorbisfile_static.lib && \
  popd && \
  rd /s /q %TEMP%\Lib

ARG IRRKLANG_VERSION=1.6.0
RUN mkdir Lib\irrKlang && \
  mkdir Lib\irrKlang\x86 && \
  mkdir Lib\irrKlang\x64 && \
  pushd %TEMP% && \
  curl -fLOJ https://www.ambiera.at/downloads/irrKlang-32bit-%IRRKLANG_VERSION%.zip && \
  curl -fLOJ https://www.ambiera.at/downloads/irrKlang-64bit-%IRRKLANG_VERSION%.zip && \
  popd && \
  tar -f %TEMP%\irrKlang-32bit-%IRRKLANG_VERSION%.zip -xvzC %TEMP% irrKlang-%IRRKLANG_VERSION%/bin/dotnet-4/*.dll && \
  move %TEMP%\irrKlang-%IRRKLANG_VERSION%\bin\dotnet-4\*.dll Lib\irrKlang\x86\ && \
  rd /s /q %TEMP%\irrKlang-%IRRKLANG_VERSION% && \
  tar -f %TEMP%\irrKlang-64bit-%IRRKLANG_VERSION%.zip -xvzC %TEMP% irrKlang-64bit-%IRRKLANG_VERSION%/bin/dotnet-4-64/*.dll && \
  move %TEMP%\irrKlang-64bit-%IRRKLANG_VERSION%\bin\dotnet-4-64\*.dll Lib\irrKlang\x64\ && \
  rd /s /q %TEMP%\irrKlang-64bit-%IRRKLANG_VERSION%

# from
# https://github.com/microsoft/winget-pkgs/blob/master/manifests/m/Microsoft/VCRedist/2015%2B/x86/14.29.30133.0/Microsoft.VCRedist.2015%2B.x86.installer.yaml
# https://github.com/microsoft/winget-pkgs/blob/master/manifests/m/Microsoft/VCRedist/2015%2B/x64/14.29.30133.0/Microsoft.VCRedist.2015%2B.x64.installer.yaml
RUN mkdir Redist && \
  cd Redist && \
  curl -fLJ https://download.visualstudio.microsoft.com/download/pr/221ed2ae-1269-497b-a962-e113045001fa/1ACD8D5EA1CDC3EB2EB4C87BE3AB28722D0825C15449E5C9CEEF95D897DE52FA/VC_redist.x86.exe -o vc_redist.x86.exe && \
  curl -fLJ https://download.visualstudio.microsoft.com/download/pr/7239cdc3-bd73-4f27-9943-22de059a6267/003063723B2131DA23F40E2063FB79867BAE275F7B5C099DBD1792E25845872B/VC_redist.x64.exe -o vc_redist.x64.exe

ARG SDL_VERSION=release-2.28.5
ARG SDL_VERSION_NUMBER=2.28.5
RUN mkdir Lib\SDL2 && \
  curl -fLOJ "https://github.com/libsdl-org/SDL/releases/download/%SDL_VERSION%/SDL2-devel-%SDL_VERSION_NUMBER%-VC.zip" && \
  tar -f SDL2-devel-%SDL_VERSION_NUMBER%-VC.zip -xvzC Lib\SDL2\ --strip-components 1  && \
  del /f SDL2-devel-%SDL_VERSION_NUMBER%-VC.zip  && \
  echo set^(SDL2_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/include"^) > "Lib\SDL2\sdl2-config.cmake" && \
  echo # Support both 32 and 64 bit builds >> "Lib\SDL2\sdl2-config.cmake" && \
  echo if ^(${CMAKE_SIZEOF_VOID_P} MATCHES 8^) >> "Lib\SDL2\sdl2-config.cmake" && \
  echo   set^(SDL2_LIBRARIES "${CMAKE_CURRENT_LIST_DIR}/lib/x64/SDL2.lib;${CMAKE_CURRENT_LIST_DIR}/lib/x64/SDL2main.lib"^)  >> "Lib\SDL2\sdl2-config.cmake" && \
  echo else ^(^)  >> "Lib\SDL2\sdl2-config.cmake" && \
  echo   set^(SDL2_LIBRARIES "${CMAKE_CURRENT_LIST_DIR}/lib/x86/SDL2.lib;${CMAKE_CURRENT_LIST_DIR}/lib/x86/SDL2main.lib"^) >> "Lib\SDL2\sdl2-config.cmake" && \
  echo endif ^(^)  >> "Lib\SDL2\sdl2-config.cmake" && \
  echo string^(STRIP "${SDL2_LIBRARIES}" SDL2_LIBRARIES^) >> "Lib\SDL2\sdl2-config.cmake" 
 
ARG SDL2_SOUND_VERSION=c5639414c1bb24fb4eef5861c13adb42a4aab950
RUN mkdir Lib\SDL_sound && \
  mkdir Lib\SDL_sound\build_x86  && \
  mkdir Lib\SDL_sound\build_x64  && \
  curl -fLSs "https://github.com/icculus/SDL_sound/archive/%SDL2_SOUND_VERSION%.tar.gz" | tar -f - -xvzC Lib\SDL_sound --strip-components 1 && \
  set SDL2_DIR=%cd%\Lib\SDL2 && \
  call "C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/Tools/VsDevCmd.bat" && \
  cmake -S Lib\SDL_sound -B Lib\SDL_sound\build_x86 -G "Visual Studio 17 2022" -T"v142" -A"Win32" -DCMAKE_PREFIX_PATH=Lib\SDL2  -DSDLSOUND_DECODER_MIDI=1 && \
  pushd Lib\SDL_sound\build_x86 && \
  msbuild SDL_sound.sln /p:PlatformToolset=v142 /p:Configuration=Release /p:Platform=Win32 /maxcpucount /nologo && \
  popd && \
  call "C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/Tools/VsDevCmd.bat" && \
  cmake -S Lib\SDL_sound -B Lib\SDL_sound\build_x64 -G "Visual Studio 17 2022" -T"v142" -A"x64" -DCMAKE_PREFIX_PATH=Lib\SDL2  -DSDLSOUND_DECODER_MIDI=1 && \
  pushd Lib\SDL_sound\build_x64 && \
  msbuild SDL_sound.sln /p:PlatformToolset=v142 /p:Configuration=Release /p:Platform=x64 /maxcpucount /nologo && \
  popd && \  
  mkdir Lib\SDL_sound\lib && \
  mkdir Lib\SDL_sound\lib\x86 && \
  mkdir Lib\SDL_sound\lib\x64 && \
  copy Lib\SDL_sound\build_x86\Release\SDL2_sound-static.lib Lib\SDL_sound\lib\x86\SDL2_sound-static.lib && \
  copy Lib\SDL_sound\build_x64\Release\SDL2_sound-static.lib Lib\SDL_sound\lib\x64\SDL2_sound-static.lib

