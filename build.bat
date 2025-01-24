@echo off

subst W: %CD%

.\init.bat

W:
mkdir build
cd build
cl ..\src\main.cpp user32.lib gdi32.lib
cd ..