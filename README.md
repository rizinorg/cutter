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
- Download: [Qt Open Source](https://www.qt.io/download-qt-for-application-development)
- Add Qt 5.9.1: http://doc.qt.io/qtcreator/creator-project-qmake.html
    
#### Building

First you must clone the repository:
```sh
git clone https://github.com/radareorg/cutter
cd cutter
```

Building on linux:
```sh
./build.sh
```

Building on Windows:
```batch
set ARCH=x64
set PYTHON=C:\Python36-x64
prepare_r2.bat
build.bat
```

If any of those do not work, check the more detailed version [here](https://github.com/radareorg/cutter/blob/master/docs/Compiling.md).

Check this [page](https://github.com/radareorg/cutter/blob/master/docs/Common-errors.md) for common issues.

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
- [@r2gui](https://twitter.com/r2gui) on Twitter
