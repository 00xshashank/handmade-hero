@echo off

subst W: %CD%

where cl >nul 2>&1 || call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x86

W:
mkdir build
cd build
cl ..\src\main.cpp /link user32.lib gdi32.lib
cd ..