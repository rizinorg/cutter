# Iaitō [![Build Status](https://travis-ci.org/hteso/iaito.svg?branch=master)](https://travis-ci.org/hteso/iaito)

> The GUI that ~~radare2~~ humans deserve

A Qt and C++ GUI for radare2 reverse engineering framework

## Screenshot

![Screenshot](https://raw.githubusercontent.com/hteso/iaito/master/Screenshots/Screenshot.png)

## Disclaimer

Iaitō is not aimed at existing radare2 users, it is focused on those whose are not yet radare2 users because of the learning curve, they don't like CLI applications or the difficulty/instability of radare2.

**IMPORTANT:** the current status is **highly unstable**, it is an alpha version aimed for developers. Users please wait for the first stable release with installers.

## The code sucks

Yes, the code sucks. Hopefully we will be able to remove this statement from the README one day, but I had never coded Qt nor C++ until I started Iaitō, so obviously the code is ugly and not well designed.

## Requirements

- **Radare2**: Make sure that, when cloning the project, you use `git clone --recurse-submodules` or run `git submodule update --init` to clone the correct radare2 version. Then execute the following command in the radare2 folder:
```
sys/install.sh
```

- QtCreator and Qt: Right now Iaitō uses Qt 5.6, you will need the latest QtCreator and Qt 5.6 added during the installation:
    - Download: https://www.qt.io/ide/
    - Add Qt 5.6: http://doc.qt.io/qtcreator/creator-project-qmake.html

## Platforms

Iaitō is developed and tested in OS X, Linux and [Windows](https://github.com/hteso/iaito/wiki/Compiling-on-Windows). The first release for users will include installers for all three platforms.

## Documentation

Proper documentation and website will be created before the first release.

Roadmap information for developers can be found [here](https://github.com/hteso/iaito/wiki/Roadmap).

## Help

Right now the best place to obtain help from Iaitō developers and community is joining this telegram group:

- https://t.me/iaito
