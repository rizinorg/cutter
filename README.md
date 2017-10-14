# Cutter
[![Build Status](https://travis-ci.org/radareorg/cutter.svg?branch=master)](https://travis-ci.org/radareorg/cutter)
[![Build status](https://ci.appveyor.com/api/projects/status/s9rkx1dn3uy4bfdx/branch/master?svg=true)](https://ci.appveyor.com/project/radare/cutter/branch/master)

A Qt and C++ GUI for radare2 reverse engineering framework (originally Iaito)

## Screenshot

![Screenshot](https://raw.githubusercontent.com/radareorg/cutter/master/docs/screenshot.png)

## Disclaimer

Cutter is not aimed at existing radare2 users. It instead focuses on those whose are not yet radare2 users because of the learning curve, because they don't like CLI applications or because of the difficulty/instability of radare2.

**IMPORTANT:** the current status is **highly unstable**, it is an alpha version aimed for developers. Users please wait for the first stable release with installers.

## Requirements

**You do not have to do this if you are using a [release](https://github.com/radareorg/cutter/releases) since r2 is included in those!**

- **Radare2**: Make sure that, when cloning the project, you use `git clone --recurse-submodules` or run `git submodule update --init` to clone the correct radare2 version. Then execute the following command in the radare2 folder:
```
sys/install.sh
```

- QtCreator and Qt: Right now *cutter* uses Qt 5.9.1, you will need the latest QtCreator and Qt added during the installation:
    - Download: [Qt Open Source](https://info.qt.io/download-qt-for-application-development)
    - Add Qt 5.9.1: http://doc.qt.io/qtcreator/creator-project-qmake.html

## Troubleshoting

On Mac, QT5 apps fail to build on QtCreator if you have the libjpeg lib installed with brew. Run this command to workaround the issue:

	sudo mv /usr/local/lib/libjpeg.dylib /usr/local/lib/libjpeg.dylib.not-found

## Platforms

Cutter is developed on OS X, Linux and Windows. The first release for users will include installers for all three platforms.

## Documentation

Proper documentation and website will be created before the first release.

## Help

Right now the best place to obtain help from *cutter* developers and community is joining this telegram group:

- https://t.me/iaito
- #cutter on irc.freenode.net
