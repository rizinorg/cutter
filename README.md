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

Cutter is available for all platforms (Linux, OS X, Windows).
You can download the latest release [here](https://github.com/radareorg/cutter/releases).
 *  Linux: use the [AppImage](https://github.com/radareorg/cutter/releases/download/v1.6/Cutter-v1.6-x86_64.AppImage) file. Then just make it executable and run it:
     * `chmod +x Cutter-v1.6-x86_64.AppImage`
     * `./Cutter-v1.6-x86_64.AppImage`

### Building from source

#### Requirements

Cutter is based on Qt so you will need to have it installed.
- Download: [Qt Open Source](https://www.qt.io/download-qt-for-application-development)
- Add Qt 5.9.1: http://doc.qt.io/qtcreator/creator-project-qmake.html

#### Building

First you must clone the repository:
```sh
git clone --recurse-submodules https://github.com/radareorg/cutter
cd cutter
```

Building on linux:
```sh
./build.sh
```
##### Important note:
When using the default `build.sh` script you might encounter a `ModuleNotFoundError` upon starting Cutter.
This can be resolved by either

1. disabling the optional jupyter support during building by modifying `build.sh` as follows:

   * Uncomment `#QMAKE_CONF="CUTTER_ENABLE_JUPYTER=false CUTTER_ENABLE_QTWEBENGINE=false"`
   * Comment out the prior empty `QMAKE_CONF=""`

2. or alternatively by installing the two python dependencies manually afterwards via:
```
pip3 install notebook jupyter_client
```
____


Building on Windows:
```
prepare_r2.bat
build.bat
```

If any of those do not work, check the more detailed version [here](https://github.com/radareorg/cutter/blob/master/docs/Compiling.md).

Check this [page](https://github.com/radareorg/cutter/blob/master/docs/Common-errors.md) for common issues.

### Docker

To deploy *cutter* using a pre-built `Dockerfile`, it's possible to use the [provided configuration](docker). The corresponding `README.md` file also contains instructions on how to get started using the docker image with minimal effort.

### Global shortcuts
| Shortcut   | Function            |
| ---------- | ------------------- |
| .          | Focus console input |
| G/S        | Focus search bar    |
| Ctrl/Cmd+R | Refresh contents    |

### Disassembly view shortcuts
| Shortcut   | Function                         |
| ---------- | -------------------------------- |
| Esc        | Seek to previous position        |
| Space      | Switch to disassembly graph view |
| Ctrl/Cmd+C | Copy                             |
| ;          | Add comment                      |
| N          | Rename current function/flag     |
| Shift+N    | Rename flag/function used here   |
| X          | Show Xrefs                       |

### Graph view shortcuts
| Shortcut            | Function                   |
| ------------------- | -------------------------- |
| Esc                 | Seek to previous position  |
| Space               | Switch to disassembly view |
| Ctrl/Cmd+MouseWheel | Zoom                       |
| +                   | Zoom in                    |
| -                   | Zoom out                   |
| =                   | Reset zoom                 |
| J                   | Next instruction           |
| K                   | Previous instruction       |

### Debug shortcuts
| Shortcut        | Function       |
| --------------- | -------------- |
| F9              | Start debug    |
| F7              | Step into      |
| F8              | Step over      |
| Ctrl/Cmd+F8     | Step out       |
| F5              | Continue       |
| F2/(Ctrl/Cmd)+B | Add breakpoint |

## Help

Right now the best place to obtain help from *cutter* developers and community is joining this telegram group:

- https://t.me/r2cutter
- #cutter on irc.freenode.net
- [@r2gui](https://twitter.com/r2gui) on Twitter
