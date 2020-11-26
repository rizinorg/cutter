@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION

IF "%VisualStudioVersion%" == "14.0" ( IF NOT DEFINED Platform SET "Platform=X86" )
FOR /F %%i IN ('powershell -c "\"%Platform%\".toLower()"') DO SET PLATFORM=%%i
powershell -c "if ('%PLATFORM%' -notin ('x86', 'x64')) {Exit 1}"
IF !ERRORLEVEL! NEQ 0 (
    ECHO Unknown platform: %PLATFORM%
    EXIT /B 1
)

SET "PATH=%CD%;%PATH%"
SET "RZDIST=rz_dist"

ECHO Building Rizin (%PLATFORM%)
CD rizin
git clean -xfd
RMDIR /S /Q ..\%RZDIST%
meson.exe rz_builddir --buildtype=release --prefix=%CD%\..\%RZDIST% || EXIT /B 1
ninja -C rz_builddir install || EXIT /B 1
IF !ERRORLEVEL! NEQ 0 EXIT /B 1
