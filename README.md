# Cutter
[![Build Status](https://travis-ci.org/radareorg/cutter.svg?branch=master)](https://travis-ci.org/radareorg/cutter)
[![Build status](https://ci.appveyor.com/api/projects/status/s9rkx1dn3uy4bfdx/branch/master?svg=true)](https://ci.appveyor.com/project/radare/cutter/branch/master)

A Qt and C++ GUI for radare2 reverse engineering framework (originally named Iaito).

## Screenshot

![Screenshot](https://raw.githubusercontent.com/radareorg/cutter/master/docs/screenshot.png)

## Disclaimer

Cutter is not aimed at existing radare2 users. It instead focuses on those whose are not yet radare2 users because of the learning curve, because they don't like CLI applications or because of the difficulty/instability of radare2.

## Installing

### Downloading a release

You can download the latest release [here](https://github.com/radareorg/cutter/releases).

### Building from source

#### Requirements

Cutter is based on Qt so you will need to have it installed.
- Download: [Qt Open Source](https://info.qt.io/download-qt-for-application-development)
- Add Qt 5.9.1: http://doc.qt.io/qtcreator/creator-project-qmake.html
    
#### Building

First you must clone the repository:
```sh
git clone https://github.com/radareorg/cutter
cd cutter
```

##### Building radare2
**/!\ WARNING /!\ This will delete your current radare2 version if any already installed.**
```sh
git submodule init radare2 && git submodule update radare2
cd radare2 && ./sys/install.sh
cd ..
```

##### Building cutter

Cutter can be build with two methods: The preferred one is with qmake, but you can also compile it with cmake. Choose the one you want to use.

- Method 1: Qmake
```sh
mkdir build
cd build
qmake ../src
make
```

- Method 2: CMake
```sh
mkdir build
cd build
cmake ../src
make
```

Then run cutter: `./cutter` or `./build/cutter`

Note: If radare2 is not installed system-wide (`./sys/user.sh` installation for instance) you might want to use  `LD_LIBRARY_PATH=$HOME/bin/prefix/radare2/lib ./cutter` to run cutter.


## Troubleshooting

On Mac, QT5 apps fail to build on QtCreator if you have the libjpeg lib installed with brew. Run this command to workaround the issue:

	sudo mv /usr/local/lib/libjpeg.dylib /usr/local/lib/libjpeg.dylib.not-found
	
If you encounter the `Project ERROR: r_core development package not found` try this command instead `PKG_CONFIG_PATH=$HOME/bin/prefix/radare2/lib/pkgconfig qmake`

## Platforms

Cutter is developed on OS X, Linux and Windows. The first release for users will include installers for all three platforms.

## Keyboard shortcuts

| Shortcut | Function |
| --- | --- |
| Global shortcuts: ||
| . | Focus console input |
| G & S | Focus search bar |
| F5 | Refresh contents |
| Disassembly view: ||
| Esc | Seek to previous position |
| Space | Switch to disassembly graph view |
| Ctrl/Cmd+C | Copy |
| ; | Add comment |
| N | Rename current function/flag |
| Shift+N | Rename flag/function used here |
| X | Show Xrefs |
| Disassembly graph view: ||
| Esc | Seek to previous position |
| Space | Switch to disassembly view |
| + | Zoom in |
| - | Zoom out |
| = | Reset zoom |
| J | Next instruction |
| K | Previous instruction |


## Help

Right now the best place to obtain help from *cutter* developers and community is joining this telegram group:

- https://t.me/r2cutter
- #cutter on irc.freenode.net
