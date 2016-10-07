@echo off
rem set path=%PATH%;c:\opt\dev\tools\apitrace-msvc\x64\bin

rem set PATH=%PATH%;C:\Windows\system32;C:\Windows;C:\Windows\System32\Wbem;C:\Windows\System32\WindowsPowerShell\v1.0\
rem set PATH=%PATH%;C:\Python27;c:\opt\dev\tools\cmake\bin;C:\opt\dev\tools\vim-7.4.193-python-2.7-python-3.3-ruby-2.0.0-lua-5.2-windows-x64
rem set PATH=%PATH%;c:\opt\dev\tools\git\cmd;C:\opt\dev\tools\sys_tools;C:\opt\dev\tools\jom;C:\opt\dev\tools\code_compare
rem set PATH=%PATH%;C:\opt\tools\curl-7.45.0-win64-mingw\bin;c:\opt\tools\gnuwin32\bin;c:\opt\dev\tools\apitrace-msvc\x64\bin
set PATH=%PATH%;C:\Program Files\Windows Kits\10\Debuggers\x64
set _NT_SYMBOL_PATH=cache*d:\debugging_symbols;srv*http://msdl.microsoft.com/download/symbols
echo %PATH%
rem pushd c:\MinGW
rem echo %cd%
rem call set_distro_paths.bat
rem popd
set TBB_ROOT=C:\opt\dev\sdks\intel-tbb-msvc
set GLFW_ROOT=C:\opt\dev\sdks\glfw-msvc-x64
set STLSOFT_ROOT=c:\opt\dev\sdks\stlsoft-1.9.124
set ASSIMP_ROOT=C:\opt\dev\sdks\assimp-msvc-x64
set FMT_ROOT=C:\opt\dev\sdks\fmt-msvc-x64
