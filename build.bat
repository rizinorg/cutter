@ECHO off
SETLOCAL ENABLEDELAYEDEXPANSION

IF NOT DEFINED QT32PATH SET QT32PATH=C:\Qt\5.9.2\msvc2015
IF NOT DEFINED QT64PATH SET QT64PATH=C:\Qt\5.9.2\msvc2015_64
IF NOT DEFINED VSVARSALLPATH SET VSVARSALLPATH=C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat

IF "%1" == "32" (
    SET "PATH=%QT32PATH%\bin;%PATH%"
    CALL "%VSVARSALLPATH%" x86
    SET MSBUILDPLATFORM=Win32
) ELSE IF "%1" == "64" (
    SET "PATH=%QT64PATH%\bin;%PATH%"
    CALL "%VSVARSALLPATH%" x64
    SET MSBUILDPLATFORM=x64
) ELSE (
    ECHO Usage: %0 {32^|64}
    EXIT /B 1
)
SET BITS=%1

ECHO Preparing directory
RMDIR /S /Q build%BITS%
MKDIR build%BITS%
CD build%BITS%

ECHO Building cutter
qmake ..\src\cutter.pro -config release -tp vc
IF !ERRORLEVEL! NEQ 0 EXIT /B 1
msbuild /m cutter.vcxproj /p:Configuration=Release;Platform=%MSBUILDPLATFORM%
IF !ERRORLEVEL! NEQ 0 EXIT /B 1

ECHO Deploying cutter
MKDIR cutter%BITS%
MOVE release\cutter.exe cutter%BITS%\cutter.exe
XCOPY /S ..\dist%BITS% cutter%BITS%\
windeployqt cutter%BITS%\cutter.exe
CD ..
