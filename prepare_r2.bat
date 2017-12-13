@echo off

if "%OLDPATH%"=="" set OLDPATH=%PATH%
if "%PYTHON%"=="" set PYTHON=C:\Python36-x64
if "%NINJA_URL%"=="" set NINJA_URL=https://github.com/ninja-build/ninja/releases/download/v1.8.2/ninja-win.zip
if "%VSVARSALLPATH%"=="" set VSVARSALLPATH=C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat

set "PYTHONHOME=%PYTHON%"
set "PATH=%PYTHON%;%PATH%"

git submodule update --init

echo Downloading meson and ninja
python -m pip install meson && COPY %PYTHON%\Scripts\meson.py meson.py
if defined NINJA_URL ( powershell -Command wget %NINJA_URL% -OutFile ninja.zip && unzip -o ninja.zip -d .\ && del ninja.zip )


IF NOT "%BITS%"=="32" (
	set VARSALL=x64
	set BI=64
	call :BUILD
)
IF NOT "%BITS%"=="64" (
	set VARSALL=x86
	set BI=32
	call :BUILD
)

GOTO :END

:BUILD
echo Building radare2 (%VARSALL%)
cd radare2
git clean -xfd
copy ..\ninja.exe .\
copy ..\meson.py .\
rmdir /s /q ..\dist%BI%
call "%VSVARSALLPATH%" %VARSALL%
%PYTHON%\python.exe sys\meson.py --release --shared --prefix="%CD%"
if not %ERRORLEVEL%==0 exit
call sys\meson_install.bat --with-static ..\dist%BI%
copy /Y build\r_userconf.h ..\dist%BI%\include\libr\
copy /Y build\r_version.h ..\dist%BI%\include\libr\
copy /Y build\shlr\liblibr2sdb.a ..\dist%BI%\r_sdb.lib
cd ..
copy /Y dist%BI%\*.lib cutter_win32\radare2\lib%BI%\
EXIT /B 0

:END
echo Copying relevant files in cutter_win32
IF "%BITS%"=="64" (
	xcopy /s /Y dist64\include\libr cutter_win32\radare2\include\libr\
) ELSE (
	xcopy /s /Y dist32\include\libr cutter_win32\radare2\include\libr\
)
del ninja.exe
del meson.py

set PATH=%OLDPATH%
set OLDPATH=
