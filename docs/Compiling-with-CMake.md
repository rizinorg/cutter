# Compiling with CMake

The "official" way to build Cutter is by using qmake, but as an alternative, a [CMakeLists.txt](https://github.com/radareorg/cutter/blob/master/src/CMakeLists.txt) is provided, so CMake can be used as well.

This page provides a guide to compile on **Linux** or **macOS**, the process for Windows is described in [Compiling-on-Windows.md](Compiling-on-Windows.md).

## Requirements
* CMake >= 3.1
* Radare2 installed from submodule, see [README.md](https://github.com/radareorg/cutter#requirements)
* Qt 5.9.1 including the components Core Widgets Gui WebEngine WebEngineWidgets

## Building

The root for CMake is in src/. In-source builds are **not allowed**, so you **must** run CMake from a separate directory:
```
cd src
mkdir build
cd build
cmake ..
```

If all went well, you should now have a working Makefile in your build directory:
```
make
```

## Troubleshooting

Depending on how Qt installed (Distribution packages or using the Qt installer application), CMake may not be able to find it by itself if it is not in a common place. You may encounter an error that looks like this:
```
CMake Error at /usr/lib/x86_64-linux-gnu/cmake/Qt5/Qt5Config.cmake:26 (find_package):
  Could not find a package configuration file provided by "Qt5WebEngine" with
  any of the following names:

    Qt5WebEngineConfig.cmake
    qt5webengine-config.cmake

  Add the installation prefix of "Qt5WebEngine" to CMAKE_PREFIX_PATH or set
  "Qt5WebEngine_DIR" to a directory containing one of the above files.  If
  "Qt5WebEngine" provides a separate development package or SDK, be sure it
  has been installed.
```

If that is the case, double check that Qt 5.6 is installed correctly and contains all required components. Locate its prefix (a directory containing bin/, lib/, include/, etc.) and specify it to CMake using `CMAKE_PREFIX_PATH` in the above process, e.g.:
```
rm CMakeCache.txt # the cache may be polluted with unwanted libraries found before
cmake -DCMAKE_PREFIX_PATH=/opt/Qt/5.6/gcc_64 ..
```
