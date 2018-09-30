# Cutter
[![Build Status](https://travis-ci.org/radareorg/cutter.svg?branch=master)](https://travis-ci.org/radareorg/cutter)
[![Build status](https://ci.appveyor.com/api/projects/status/s9rkx1dn3uy4bfdx/branch/master?svg=true)](https://ci.appveyor.com/project/radare/cutter/branch/master)

A Qt and C++ GUI for radare2 reverse engineering framework (originally named Iaito).

![Screenshot](https://raw.githubusercontent.com/radareorg/cutter/master/docs/images/screenshot.png)

# Disclaimer

Cutter is not aimed at existing radare2 users. It instead focuses on those whose are not yet radare2 users because of the learning curve, because they don't like CLI applications or because of the difficulty/instability of radare2.

# Downloading a release

Cutter is available for all platforms (Linux, OS X, Windows).
You can download the latest release [here](https://github.com/radareorg/cutter/releases).
 * OSX: Download the latest `.dmg` file.
 * Windows: Download the latest archive.
 * Linux: use the [AppImage](https://github.com/radareorg/cutter/releases/download/v1.7.1/Cutter-v1.7.1-x86_64.AppImage) file. Then just make it executable and run it:
     * `chmod +x Cutter-v1.7.1-x86_64.AppImage`
     * `./Cutter-v1.7.1-x86_64.AppImage`

## Building from sources

To build Cutter on your local machine, please follow this guide: [Building from source](https://radareorg.github.io/cutter/building.html)

## Docker

To deploy *cutter* using a pre-built `Dockerfile`, it's possible to use the [provided configuration](docker). The corresponding `README.md` file also contains instructions on how to get started using the docker image with minimal effort.

# Documentation

You can find our documentation [here](https://radareorg.github.io/cutter/).

# Help

Right now the best place to obtain help from *Cutter* developers and community is to contact us on:

- https://t.me/r2cutter
- #cutter on irc.freenode.net
- [@r2gui](https://twitter.com/r2gui) on Twitter
