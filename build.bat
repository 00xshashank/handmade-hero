@echo off

subst W: %CD%

C:
call ".\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

W:
mkdir build
cd build
cl ..\src\main.cpp user32.lib gdi32.lib
cd ..