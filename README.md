<img width="150" height="150" align="left" style="float: left; margin: 0 10px 0 0;" alt="Cutter logo" src="https://raw.githubusercontent.com/radareorg/cutter/master/src/img/cutter.svg?sanitize=true">

# Cutter

Cutter is a free and open-source reverse engineering framework powered by [radare2](https://github.com/radareorg/radare2) . Its goal is making an advanced, customizable and FOSS reverse-engineering platform while keeping the user experience at mind. Cutter is created by reverse engineers for reverse engineers.  

[![Build Status](https://travis-ci.com/radareorg/cutter.svg?branch=master)](https://travis-ci.com/radareorg/cutter)
[![Build status](https://ci.appveyor.com/api/projects/status/s9rkx1dn3uy4bfdx/branch/master?svg=true)](https://ci.appveyor.com/project/radareorg/cutter/branch/master)
[![Total alerts](https://img.shields.io/lgtm/alerts/g/radareorg/cutter.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/radareorg/cutter/alerts/)

![Screenshot](https://raw.githubusercontent.com/radareorg/cutter/master/docs/source/images/screenshot.png)

## Learn more at [https://cutter.re](https://cutter.re).

## Downloading a release

Cutter is available for all platforms (Linux, macOS, Windows).
You can download the latest release [here](https://github.com/radareorg/cutter/releases).
 * macOS: Download the latest `.dmg` file or use [Homebrew Cask](https://github.com/Homebrew/homebrew-cask) `brew cask install cutter`.
 * Windows: Download the latest Zip archive.
 * Linux: Download the latest AppImage file. Then just make it executable and run it:
   * `chmod +x <appimage_file>`
   * `./<appimage_file>`

## Building from sources

To build Cutter on your local machine, please follow this guide: [Building from source](https://cutter.re/docs/building.html)

## Docker

To deploy *cutter* using a pre-built `Dockerfile`, it's possible to use the [provided configuration](docker). The corresponding `README.md` file also contains instructions on how to get started using the docker image with minimal effort.

## Documentation

You can find our documentation in our [website](https://cutter.re/docs/).

## Plugins
Cutter supports both Python and Native C++ plugins. Want to extend Cutter with Plugins? Read the [Plugins](https://cutter.re/docs/plugins) section on our documentation.

### Official and Community Plugins
Our community built many plugins and useful scripts for Cutter such as the native integration of Ghidra decompiler or the plugin to visualize DynamoRIO code coverage. You can find more plugins in the [following list](https://github.com/radareorg/cutter-plugins). Don't hesitate to extend it with your own plugins and scripts for Cutter.

## Help

The best place to obtain help from *Cutter* developers and community is to contact us on:

- **Telegram:** https://t.me/r2cutter
- **IRC:** #cutter on irc.freenode.net
- **Twitter:** [@r2gui](https://twitter.com/r2gui)
