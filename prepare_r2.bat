@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION

IF "%Platform%" == "X64" (
    SET BITS=64
) ELSE (
    SET BITS=32
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
