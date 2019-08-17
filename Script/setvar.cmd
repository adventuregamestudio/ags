@echo off

for /f "tokens=1-3" %%a in (%~dp0..\Common\core\def_version.h) do ^
if "%%a" == "#define" if "%%b" == "%1" ^
set %1=%%~c
