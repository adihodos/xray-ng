@echo off
rem set path=%PATH%;c:\opt\dev\tools\apitrace-msvc\x64\bin
pushd c:\MinGW
echo %cd%
call set_distro_paths.bat
popd
set CC=gcc
set CXX=g++
set TBB_ROOT=c:\opt\dev\sdks\intel-tbb
set GLFW_ROOT=c:\opt\dev\sdks\glfw
set STLSOFT_ROOT=c:\opt\dev\sdks\stlsoft-1.9.124
set ASSIMP_ROOT=c:\opt\dev\sdks\assimp-mingw-x64
set FMT_ROOT=c:\opt\dev\sdks\fmt