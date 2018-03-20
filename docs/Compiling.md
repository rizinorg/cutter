# Compiling Cutter

Cutter supports different build systems:

* Building with qmake
* [Building with cmake](https://github.com/radareorg/cutter/blob/master/docs/Compiling-with-CMake.md)
* Building with meson (see ./meson.py)

Each time in the section below, the prefered method will be explained. For other methods check associated documentation.

## Requirements

* Qt (version differs from time to time but >= 5.6.1 should be fine)
* Radare2 (version changes, see `git submodule` to check the exact version)
* Python 3.6

### Compiling on Linux / OsX

The easy way is to simply run `./build.sh` from the root directory, and let the magic happen. The script will use qmake to build Cutter.

If you want to manually use qmake, follow this steps:
```sh
mkdir build; cd build
qmake ../src/Cutter.pro
make
cd ..
```

### Compiling on Windows

Additional requirements:

* Visual Studio 2015 or Visual Studio 2017
* Ninja build system
* Meson build system

Download and unpack [Ninja](https://github.com/ninja-build/ninja/releases) to the Cutter source root directory.

Environment settings (example for x64 version):
```batch
:: Export MSVC variables
CALL "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64
:: Add qmake to PATH
SET "PATH=C:\Qt\5.10.1\msvc2015_64\bin;%PATH%"
:: Add Python to PATH
SET "PATH=C:\Program Files\Python36;%PATH%"
```

Install Meson:
```batch
python -m pip install meson
```
To compile Cutter run:
```batch
CALL prepare_r2.bat
CALL build.bat
```

Pass `CUTTER_ENABLE_JUPYTER=false` argument to `build.bat` if you want to disable Jupyter support. Use `CUTTER_ENABLE_QTWEBENGINE=false` argument to disable QtWebEngine support for Jupyter.

## It doesn't work!

Check this [page](https://github.com/radareorg/cutter/blob/master/docs/Common-errors.md) for common issues.
