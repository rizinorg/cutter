# Compiling with CMake

The "official" way to build Cutter is by using qmake, but as an alternative, a [CMakeLists.txt](https://github.com/radareorg/cutter/blob/master/src/CMakeLists.txt) is provided, so CMake can be used as well.

## Requirements
* CMake >= 3.1
* Radare2 installed from submodule, see [README.md](https://github.com/radareorg/cutter#requirements)
* Qt 5.9.1

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

### On windows

Alternatively, on Windows you can run something like this (depending on your Cmake installation)
```batch
set CMAKE_PREFIX_PATH=c:\Qt\qt-5.6.2-msvc2013-x86\5.6\msvc2013\lib\cmake
cd src
mkdir build
cd build
cmake-gui ..
```

Click `Configure` and select `Visual Studio 12 2013` from the list. After configuration is done, click `Generate` and you can open `Cutter.sln` to compile the code as usual.

## Troubleshooting

Depending on how Qt installed (Distribution packages or using the Qt installer application), CMake may not be able to find it by itself if it is not in a common place. If that is the case, double check that the correct Qt version is installed. Locate its prefix (a directory containing bin/, lib/, include/, etc.) and specify it to CMake using `CMAKE_PREFIX_PATH` in the above process, e.g.:
```
rm CMakeCache.txt # the cache may be polluted with unwanted libraries found before
cmake -DCMAKE_PREFIX_PATH=/opt/Qt/5.9.1/gcc_64 ..
```
