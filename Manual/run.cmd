start /WAIT tex2rtf.exe ags.tex ags.html -html -sync
python striphdr.py htmlfiles\actutor.htm ags29.htm
python striphdr.py htmlfiles\actutor2.htm ags30.htm
python procdocs.py
cd htmlfiles
for %%i in (acintro*.htm) do python ..\striphdr.py %%i ..\%%i
cd ..
for %%i in (acintro*.htm) do echo %%i >>ags.hhp
echo images\*.jpg >>ags.hhp
echo images\*.gif >>ags.hhp
hhc ags.hhp
