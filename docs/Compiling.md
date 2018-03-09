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
```
mkdir build; cd build
qmake ../src/Cutter.pro
make
cd ..
```

### Compiling on Windows

The easy way to compile on Windows is to run:

```
set PYTHON=C:\Python36-x64
set ARCH=x64
prepare_r2.bat
build.bat
```

## It doesn't work!

Check this [page](https://github.com/radareorg/cutter/blob/master/docs/Common-errors.md) for common issues.
