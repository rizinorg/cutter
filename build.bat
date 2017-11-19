@echo off

echo Setting path
if "%OLDPATH%"=="" set OLDPATH=%PATH%
if "%QT32PATH%"=="" set QT32PATH=C:\Qt\5.9.2\msvc2015
if "%QT64PATH%"=="" set QT64PATH=C:\Qt\5.9.2\msvc2015_64
if "%VSVARSALLPATH%"=="" set VSVARSALLPATH=C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat

if "%1"=="32" (
    set "PATH=%QT32PATH%\bin;%PATH%"
    call "%VSVARSALLPATH%" x86
    set MSBUILDPLATFORM=Win32
) else if "%1"=="64" (
    set "PATH=%QT64PATH%\bin;%PATH%"
    call "%VSVARSALLPATH%" x64
    set MSBUILDPLATFORM=x64
) else (
    echo Usage: build.bat 32/64
    goto restorepath
)

echo Preparing directory
rmdir /s /q build%1
mkdir build%1
cd build%1

echo Building cutter
qmake ..\src\cutter.pro -config release -tp vc
if not %ERRORLEVEL%==0 exit
msbuild /m cutter.vcxproj /p:Configuration=Release;Platform=%MSBUILDPLATFORM%
if not %ERRORLEVEL%==0 exit

echo Deploying cutter
mkdir cutter%1
move release\cutter.exe cutter%1\cutter.exe
xcopy /s ..\dist%1 cutter%1\
windeployqt cutter%1\cutter.exe
cd ..

:restorepath
echo Restoring path
set PATH=%OLDPATH%
set OLDPATH=
