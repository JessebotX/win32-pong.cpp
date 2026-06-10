@echo off

if not exist out (
   cmake -B out -G Ninja
)

cmake --build out
