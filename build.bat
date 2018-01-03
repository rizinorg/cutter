@ECHO off
SETLOCAL ENABLEDELAYEDEXPANSION

IF NOT DEFINED QT32PATH SET QT32PATH=C:\Qt\5.9.2\msvc2015
IF NOT DEFINED QT64PATH SET QT64PATH=C:\Qt\5.9.2\msvc2015_64
IF NOT DEFINED VSVARSALLPATH SET VSVARSALLPATH=C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat

if "%1" == "32" (
    SET "PATH=%QT32PATH%\bin;%PATH%"
    CALL "%VSVARSALLPATH%" x86
    SET MSBUILDPLATFORM=Win32
) else if "%1" == "64" (
    SET "PATH=%QT64PATH%\bin;%PATH%"
    CALL "%VSVARSALLPATH%" x64
    SET MSBUILDPLATFORM=x64
) else (
    ECHO Usage: %0 {32^|64}
    EXIT /B
)

ECHO Preparing directory
RMDIR /S /Q build%1
MKDIR build%1
CD build%1

ECHO Building cutter
qmake ..\src\cutter.pro -config release -tp vc
IF !ERRORLEVEL! NEQ 0 EXIT /B
msbuild /m cutter.vcxproj /p:Configuration=Release;Platform=%MSBUILDPLATFORM%
IF !ERRORLEVEL! NEQ 0 EXIT /B

ECHO Deploying cutter
MKDIR cutter%1
MOVE release\cutter.exe cutter%1\cutter.exe
XCOPY /S ..\dist%1 cutter%1\
windeployqt cutter%1\cutter.exe
CD ..
