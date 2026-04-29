@echo off

REM Script to quickly setup dev environment in Windows
REM This script will retrieve Xiph and SDL2 binaries but it will have to build
REM SDL_sound because no binary release for it exists.
REM It also uses CMake, VS, curl, nuget. I think those exists in a regular CI.
REM Windows comes with bsdtar by default, so this script assumes it exists.

set XIPH_VERSION=2022.12.23
set SDL_VERSION=release-2.30.11
set SDL_VERSION_NUMBER=2.30.11
set SDL2_SOUND_VERSION=474dbf755a1b67ebe7a55467b4f65e033f268aff
set VCREDIST_X86_URL=https://download.visualstudio.microsoft.com/download/pr/5319f718-2a84-4aff-86be-8dbdefd92ca1/DD1A8BE03398367745A87A5E35BEBDAB00FDAD080CF42AF0C3F20802D08C25D4/VC_redist.x86.exe
set VCREDIST_X64_URL=https://download.visualstudio.microsoft.com/download/pr/c7dac50a-e3e8-40f6-bbb2-9cc4e3dfcabe/1821577409C35B2B9505AC833E246376CC68A8262972100444010B57226F0940/VC_redist.x64.exe

set INSTALL_ROOT=C:\Lib

REM --- First check -----------------------------------------------------------
where nuget >nul 2>&1
if errorlevel 1 (
    echo ERROR: nuget not found in PATH
    echo Get nuget from https://www.nuget.org/downloads
    echo And put in a directory in your PATH
    exit /b 1
)

where cmake >nul 2>&1
if errorlevel 1 (
    echo ERROR: cmake not found in PATH
    echo Install cmake from https://cmake.org/download/
    echo And make sure it is in your PATH
    exit /b 1
)

goto :main

REM --- Helpers ---------------------------------------------------------------
:recreate_dir
REM %1 = directory
if exist "%~1" (
    echo Removing "%~1"
    rmdir /s /q "%~1"
)
echo Creating "%~1"
mkdir "%~1"
exit /b

:download
REM %1 = URL, %2 = output file
curl.exe -L --fail --silent --show-error --retry 3 --retry-delay 5 --retry-connrefused --max-time 300 -o "%~2" "%~1"
if errorlevel 1 (
    echo Download failed: %~1
    if exist "%~2" del /f /q "%~2"
    exit /b 1
)
exit /b


:main

REM --- VcRedist begin --------------------------------------------------------
REM Note: VcRedist is only required for the installer.

call :recreate_dir "%INSTALL_ROOT%\VcRedist"


call :download "%VCREDIST_X86_URL%" "%INSTALL_ROOT%\VcRedist\vc_redist.x86.exe" || exit /b 1
call :download "%VCREDIST_X64_URL%" "%INSTALL_ROOT%\VcRedist\vc_redist.x64.exe" || exit /b 1

echo Downloaded Vc Redistributables.
echo Done.
REM --- VcRedist end ----------------------------------------------------------

REM --- irrKlang begin --------------------------------------------------------
REM Note: irrKlang is only required for the AGS Editor.
REM There are also no newer releases of it, so no need to worry about versions.
echo Downloading irrKlang...
set IRRKLANG_ZIP=%TEMP%\irrKlang.zip
call :download https://github.com/ericoporto/irrKlang-for-ags/releases/download/1.6.0/irrKlang.zip "%IRRKLANG_ZIP%" || exit /b 1

call :recreate_dir "%INSTALL_ROOT%\irrKlang"
tar -xf "%IRRKLANG_ZIP%" -C "%INSTALL_ROOT%\irrKlang"
del "%IRRKLANG_ZIP%"

echo Downloaded and extracted irrKlang.
echo Done.
REM --- irrKlang end ----------------------------------------------------------

REM --- Xiph begin ------------------------------------------------------------
echo Downloading Xiph libs...
set XIPH_TMP=%TEMP%\nuget-xiph
call :recreate_dir "%XIPH_TMP%"

nuget install ericoporto.xiph-for-ags -Version %XIPH_VERSION% -OutputDirectory "%XIPH_TMP%" -NonInteractive

set XIPH_SRC=%XIPH_TMP%\ericoporto.xiph-for-ags.%XIPH_VERSION%\native\lib

for %%A in (x86 x64) do (
    call :recreate_dir "%INSTALL_ROOT%\Xiph\%%A"
    copy "%XIPH_SRC%\%%A\*.lib" "%INSTALL_ROOT%\Xiph\%%A\" >nul
    echo Copied Xiph %%A
)

rmdir /s /q "%XIPH_TMP%"
echo Downloaded and extracted Xiph from NuGET.
echo Done.
REM --- Xiph end --------------------------------------------------------------

REM --- SDL2 begin ------------------------------------------------------------
echo Downloading SDL2...
set SDL2_ZIP=%TEMP%\SDL2.zip
call :download https://github.com/libsdl-org/SDL/releases/download/%SDL_VERSION%/SDL2-devel-%SDL_VERSION_NUMBER%-VC.zip "%SDL2_ZIP%" || exit /b 1

call :recreate_dir "%INSTALL_ROOT%\SDL2"
tar -xf "%SDL2_ZIP%" -C "%INSTALL_ROOT%\SDL2" --strip-components=1
del "%SDL2_ZIP%"

echo Downloaded and extracted SDL2 VC build.
echo Done.
REM --- SDL2 end --------------------------------------------------------------

REM --- SDL_sound begin -------------------------------------------------------
echo Downloading SDL_sound...
set SDLSOUND_TAR=%TEMP%\SDL_sound.tar.gz
call :download https://github.com/icculus/SDL_sound/archive/%SDL2_SOUND_VERSION%.tar.gz "%SDLSOUND_TAR%" || exit /b 1

set SDLSOUND_SRC=%INSTALL_ROOT%\SDL_sound
call :recreate_dir "%SDLSOUND_SRC%"
tar -xf "%SDLSOUND_TAR%" -C "%SDLSOUND_SRC%" --strip-components=1
del "%SDLSOUND_TAR%"

echo Downloaded and extracted SDL_sound.

echo Building SDL_sound...
for %%A in (x86 x64) do (

    echo Building SDL_sound %%A...

    if "%%A"=="x86" (
        cmake -S "%SDLSOUND_SRC%" -B "%SDLSOUND_SRC%\build_%%A" ^
            -G "Visual Studio 17 2022" -T v142 -A Win32 ^
            -DCMAKE_PREFIX_PATH="%INSTALL_ROOT%\SDL2" ^
            -DSDLSOUND_DECODER_MIDI=1
    ) else (
        cmake -S "%SDLSOUND_SRC%" -B "%SDLSOUND_SRC%\build_%%A" ^
            -G "Visual Studio 17 2022" -T v142 -A x64 ^
            -DCMAKE_PREFIX_PATH="%INSTALL_ROOT%\SDL2" ^
            -DSDLSOUND_DECODER_MIDI=1
    )

    cmake --build "%SDLSOUND_SRC%\build_%%A" --config Release --parallel 2

    call :recreate_dir "%SDLSOUND_SRC%\lib\%%A"
    copy "%SDLSOUND_SRC%\build_%%A\Release\SDL2_sound-static.lib" "%SDLSOUND_SRC%\lib\%%A\" >nul
)

echo Built SDL_sound.
echo Done.
REM --- SDL_sound end -------------------------------------------------------

echo All dependencies installed to %INSTALL_ROOT%
echo irrKlang:  %INSTALL_ROOT%\irrKlang
echo Xiph:      %INSTALL_ROOT%\Xiph
echo SDL2:      %INSTALL_ROOT%\SDL2
echo SDL_sound: %INSTALL_ROOT%\SDL_sound
echo VcRedist:  %INSTALL_ROOT%\VcRedist
