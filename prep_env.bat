@echo off
rem set path=%PATH%;c:\opt\dev\tools\apitrace-msvc\x64\bin

set PATH=c:\llvm-mingw-x64\bin;C:\mingw-w64\x86_64-6.2.0-posix-seh-rt_v5-rev0\mingw64\bin;
set PATH=%PATH%;C:\Windows\system32;C:\Windows;C:\Windows\System32\Wbem;C:\Windows\System32\WindowsPowerShell\v1.0\
set PATH=%PATH%;C:\Python27;c:\opt\dev\tools\cmake\bin;C:\opt\dev\tools\vim-7.4.193-python-2.7-python-3.3-ruby-2.0.0-lua-5.2-windows-x64
set PATH=%PATH%;c:\opt\dev\tools\git\cmd;C:\opt\dev\tools\sys_tools;C:\opt\dev\tools\jom;C:\opt\dev\tools\code_compare
set PATH=%PATH%;C:\opt\tools\curl-7.45.0-win64-mingw\bin;c:\opt\tools\gnuwin32\bin;c:\opt\dev\tools\apitrace-msvc\x64\bin
echo %PATH%
rem pushd c:\MinGW
rem echo %cd%
rem call set_distro_paths.bat
rem popd
set CC=gcc
set CXX=g++
set TBB_ROOT=c:\opt\dev\sdks\intel-tbb
set GLFW_ROOT=c:\opt\dev\sdks\glfw
set STLSOFT_ROOT=c:\opt\dev\sdks\stlsoft-1.9.124
set ASSIMP_ROOT=c:\opt\dev\sdks\assimp-mingw-x64
set FMT_ROOT=c:\opt\dev\sdks\fmt
