# Compiling on Windows

## Cloning the project

Make sure that when cloning the project you use `git clone --recurse-submodules` or run `git submodule init` and `git submodule update` to clone the [iaito_win32](https://github.com/mrexodia/iaito_win32) submodule.

## Setting up Qt 5.6.2

It is advised to use [Qt 5.6.2](https://download.qt.io/archive/qt/5.6/5.6.2) to compile on Windows. Install one of the packages (MinGW or MSVC) before you start. If you want to use a different Qt version, make sure to change the commands below accordingly.

## Building Iaito

There are two main methods to build Iaito. Choose the one you prefer.

#### Building with Qt Creator

See [Adding Kits](http://doc.qt.io/qtcreator/creator-targets.html) for documentation on how to setup Qt kits (this works for both MinGW and MSVC). Once you have set up a kit simply open `Iaito.pro`, select a kit and compile.

#### Building with CMake

In the project root, run:

```batch
set CMAKE_PREFIX_PATH=c:\Qt\qt-5.6.2-msvc2013-x86\5.6\msvc2013\lib\cmake
mkdir build-cmake
cd build-cmake
cmake-gui ../src
```

Click `Configure` and select `Visual Studio 12 2013` from the list. After configuration is done, click `Generate` and you can open `Iaito.sln` to compile the code as usual.

## Deploying/Running Iaito

**These steps are required to get iaito.exe to run.**

You can use the following commands to deploy a standalone version of Iaito for your friends (assuming you have copied `iaito.exe` to a new empty directory and opened a terminal there).

```batch
set PATH=%PATH%;c:\Qt\qt-5.6.2-msvc2013-x86\5.6\msvc2013\bin
windeployqt iaito.exe
```

After this you will need to add the following files next to `iaito.exe` (they can usually be found in your system directories, OpenSSL binaries can be found [here](https://slproweb.com/products/Win32OpenSSL.html)):

```
libeay32.dll
ssleay32.dll
MSVCP120.dll
MSVCR120.dll
```

After that you have to extract the [recommended radare2 version](https://github.com/mrexodia/iaito_win32/releases/latest) (`radare2-XXXXXX.zip`) in this directory so that you have `iaito.exe` and `libr_core.dll` next to each other.

Starting `iaito.exe` should open the GUI. Typing `?d call` in the command bar should show you:

> calls a subroutine, push eip into the stack (esp)

![windows screenshot](https://i.imgur.com/BPKSSZY.png)