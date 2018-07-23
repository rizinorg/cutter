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
SET "R2DIST=r2_dist_%PLATFORM%"

ECHO Building radare2 (%PLATFORM%)
CD radare2
git clean -xfd
RMDIR /S /Q ..\%R2DIST%
python sys\meson.py --release --shared --install=..\%R2DIST% --options r2_datdir=radare2/share
IF !ERRORLEVEL! NEQ 0 EXIT /B 1
COPY /Y build\shlr\libr2sdb.a ..\%R2DIST%\lib\r_sdb.lib
