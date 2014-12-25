@echo off
set deploypath=..\..\..\..\..\..\Deploy
set src=..\..\..\..\..\pyilmbase-1.0.0\PyIex

if not exist %deploypath% mkdir %deploypath%

set intdir=%1%
if %intdir%=="" set intdir=Release

set instpath=%deploypath%\bin\%intdir%
if not exist %instpath% mkdir %instpath%
copy ..\%intdir%\iex.pyd %instpath%


