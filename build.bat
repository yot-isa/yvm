@echo off
rem launch this from msvc-enabled console

set CFLAGS=/std:c11 /O2 /FC /W4 /WX /wd4996 /nologo /Fe.\build\bin\ /Fo.\build\bin\
set LIBS=

mkdir build
mkdir build\bin

cl.exe %CFLAGS% .\src\main.c
