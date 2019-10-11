@echo off
setlocal

set MANUAL_FILENAME=ags-help.chm
set MANUAL_REPOSITORY=adventuregamestudio/ags-manual
set MANUAL_TAG_URL=https://adventuregamestudio.github.io/ags-manual/tag.txt

for /f %%a in ('curl -fLSs %MANUAL_TAG_URL%') do set tag=%%a
if defined tag curl -fLOJ "https://github.com/%MANUAL_REPOSITORY%/releases/download/%tag%/%MANUAL_FILENAME%"

endlocal
