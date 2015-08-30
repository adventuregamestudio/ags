@echo off

tex2rtf.exe ags.tex ags.html -html -sync
if %errorlevel% neq -1 goto error

python procdocs.py || goto error

python striphdr.py htmlfiles\actutor.htm ags29.htm || goto error
python striphdr.py htmlfiles\actutor2.htm ags30.htm || goto error
pushd htmlfiles
for %%i in (acintro*.htm) do (
    python ..\striphdr.py %%i ..\%%i || goto error
)
popd
for %%i in (acintro*.htm) do (
    echo %%i >>ags.hhp
)

echo images\*.jpg >>ags.hhp
echo images\*.gif >>ags.hhp

hhc ags.hhp
if %errorlevel% neq 1 goto error

goto :EOF

:error
echo Failed with error level %errorlevel%
exit /b 1
