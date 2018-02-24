@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION

FOR %%i IN (python.exe) DO (IF NOT DEFINED PYTHON SET PYTHON=%%~dp$PATH:i)

IF NOT DEFINED PYTHON SET PYTHON=C:\Program Files\Python36
IF NOT DEFINED NINJA_URL SET NINJA_URL=https://github.com/ninja-build/ninja/releases/download/v1.8.2/ninja-win.zip
IF NOT DEFINED VSVARSALLPATH SET VSVARSALLPATH=C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat

IF "%1" == "32" (
    CALL "%VSVARSALLPATH%" x86
) ELSE IF "%1" == "64" (
    CALL "%VSVARSALLPATH%" x64
) ELSE (
    ECHO Usage: %0 {32^|64}
    EXIT /B 1
)
SET BITS=%1

SET "PATH=%CD%;%PYTHON%;%PATH%"

git submodule update --init

ECHO Downloading meson and ninja
python -m pip install meson
IF !ERRORLEVEL! NEQ 0 EXIT /B 1
IF NOT EXIST ninja.exe (
	powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; wget %NINJA_URL% -OutFile ninja.zip" && powershell -Command Expand-Archive .\ninja.zip -DestinationPath .\ && DEL ninja.zip
	IF !ERRORLEVEL! NEQ 0 EXIT /B 1
)

ECHO Building radare2 (%BITS%)
CD radare2
git clean -xfd
RMDIR /S /Q ..\dist%BITS%
python sys\meson.py --release --install=..\dist%BITS% --shared --copylib
IF !ERRORLEVEL! NEQ 0 EXIT /B 1
COPY /Y build\r_userconf.h ..\dist%BITS%\include\libr\
COPY /Y build\r_version.h ..\dist%BITS%\include\libr\
COPY /Y build\shlr\liblibr2sdb.a ..\dist%BITS%\r_sdb.lib
CD ..
COPY /Y dist%BITS%\*.lib cutter_win32\radare2\lib%BITS%\

ECHO Copying relevant files in cutter_win32
XCOPY /S /Y dist%BITS%\include\libr cutter_win32\radare2\include\libr\
