start /WAIT tex2rtf.exe ags.tex ags.html -html -sync
striphdr htmlfiles\actutor.htm ags29.htm
striphdr htmlfiles\actutor2.htm ags30.htm
procdocs
cd htmlfiles
for %%i in (acintro*.htm) do ..\striphdr %%i ..\%%i
cd ..
for %%i in (acintro*.htm) do echo %%i >>ags.hhp
echo images\*.jpg >>ags.hhp
echo images\*.gif >>ags.hhp
hhc ags.hhp
pause
