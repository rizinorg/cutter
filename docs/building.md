# Building

First you must get the source of Cutter by cloning the repository:

```
git clone --recurse-submodules https://github.com/radareorg/cutter
```

The "official" way to build Cutter is by using qmake, but there are two alternatives -- cmake and meson.

In any case, there are obviously some requirements:
* Radare2 installed from submodule
* Qt 5.9 or above
* Python3.6

**Before compiling, note that we also provide binaries available for windows/linux/MacOS X [here](https://github.com/radareorg/cutter/releases).**


---


# Building options


Note that there are two major building options available:
* `CUTTER_ENABLE_JUPYTER` is used to compile Cutter with bundled Python and Jupyter module
* `CUTTER_ENABLE_QTWEBENGINE` is used to compile Cutter with bundled QtWebEngine (to ease jupyter console usage)

---

# Building with Qmake

#### Compiling on Linux / macOS

The easy way is to simply run `./build.sh` from the root directory, and let the magic happen. The script will use qmake to build Cutter.

If you want to manually use qmake, follow this steps:
```sh
mkdir build; cd build
qmake ../src/Cutter.pro
make
cd ..
```

#### Additional steps for macOS

On macOS you will also have to copy the launcher bash script:
```sh
mv Cutter.app/Contents/MacOS/Cutter Cutter.app/Contents/MacOS/Cutter.bin
cp ../src/macos/Cutter Cutter.app/Contents/MacOS/Cutter && chmod +x Cutter.app/Contents/MacOS/Cutter
```

---

# Building with Cmake

### Requirements
* CMake >= 3.1

### Building on Linux

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

---

# Building on Windows

Alternatively, on Windows you can run something like this (depending on your Cmake installation)
```batch
set CMAKE_PREFIX_PATH=c:\Qt\qt-5.6.2-msvc2013-x86\5.6\msvc2013\lib\cmake
cd src
mkdir build
cd build
cmake-gui ..
```

Click `Configure` and select `Visual Studio 14 2015` from the list. After configuration is done, click `Generate` and you can open `Cutter.sln` to compile the code as usual.

## Troubleshooting

> Cmake: qt development package not found

Depending on how Qt installed (Distribution packages or using the Qt installer application), CMake may not be able to find it by itself if it is not in a common place. If that is the case, double check that the correct Qt version is installed. Locate its prefix (a directory containing bin/, lib/, include/, etc.) and specify it to CMake using `CMAKE_PREFIX_PATH` in the above process, e.g.:
```
rm CMakeCache.txt # the cache may be polluted with unwanted libraries found before
cmake -DCMAKE_PREFIX_PATH=/opt/Qt/5.9.1/gcc_64 ..
```

> `ModuleNotFoundError` upon starting Cutter.

This can be resolved by either:
1. Disabling the optional jupyter support during building by modifying `build.sh` as follows:

   * Uncomment `#QMAKE_CONF="CUTTER_ENABLE_JUPYTER=false CUTTER_ENABLE_QTWEBENGINE=false"`
   * Comment out the prior empty `QMAKE_CONF=""`

2. Or alternatively by installing the two python dependencies manually afterwards via:
```
pip3 install notebook jupyter_client
```

## Building with Meson (Windows)

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
SET "PATH=C:\Program Files\Python36;%PATH%" ``` 
Install Meson:
```batch
python -m pip install meson
```

To compile Cutter run:
```batch
CALL prepare_r2.bat
CALL build.bat
```

