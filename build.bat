@ECHO off
SETLOCAL ENABLEDELAYEDEXPANSION

IF "%Platform%" == "X64" (
    SET MSBUILDPLATFORM=x64
    SET BITS=64
) ELSE (
    SET MSBUILDPLATFORM=Win32
    SET BITS=32
)

ECHO Preparing directory
RMDIR /S /Q build%BITS%
MKDIR build%BITS%
CD build%BITS%

ECHO Building cutter
qmake "CONFIG+=%*" ..\src\cutter.pro -config release -tp vc
IF !ERRORLEVEL! NEQ 0 EXIT /B 1
msbuild /m cutter.vcxproj /p:Configuration=Release;Platform=%MSBUILDPLATFORM%
IF !ERRORLEVEL! NEQ 0 EXIT /B 1

ECHO Deploying cutter
MKDIR cutter
MOVE release\cutter.exe cutter\cutter.exe
XCOPY /S ..\dist%BITS% cutter\
windeployqt cutter\cutter.exe
