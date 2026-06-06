@echo off

set compiler_flags=-std=c++23 -pedantic -Wall -Wextra -Werror
set disabled_warnings=-Wno-unused-function -Wno-unused-parameter -Wno-unused-variable
set compiler_flags_debug=-O0 -g3
set compiler_flags_release=-O2 -s

if not exist out (
   mkdir out
)

clang++ %compiler_flags% %disabled_warnings% %compiler_flags_debug% src\main.cpp -o out\w32-pong.exe
