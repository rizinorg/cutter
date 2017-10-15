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

cd radare2

echo Building radare2 (x86)
git clean -xfd
copy ..\ninja.exe .\
copy ..\meson.py .\
rmdir /s /q ..\dist32
call "%VSVARSALLPATH%" x86
call meson.bat --release --shared
if not %ERRORLEVEL%==0 exit
call sys\meson_install.bat --with-static ..\dist32
copy /Y build\r_userconf.h ..\dist32\include\libr\
copy /Y build\r_version.h ..\dist32\include\libr\
copy /Y build\shlr\liblibr2sdb.a ..\dist32\r_sdb.lib

echo Building radare2 (x64)
git clean -xfd
copy ..\ninja.exe .\
copy ..\meson.py .\
rmdir /s /q ..\dist64
call "%VSVARSALLPATH%" x64
call meson.bat --release --shared
if not %ERRORLEVEL%==0 exit
call sys\meson_install.bat --with-static ..\dist64
copy /Y build\shlr\liblibr2sdb.a ..\dist64\r_sdb.lib

cd ..

echo Copying relevant files in cutter_win32
xcopy /s /Y dist32\include\libr cutter_win32\radare2\include\libr\
copy /Y dist32\*.lib cutter_win32\radare2\lib32\
copy /Y dist64\*.lib cutter_win32\radare2\lib64\

del ninja.exe
del meson.py

set PATH=%OLDPATH%
set OLDPATH=