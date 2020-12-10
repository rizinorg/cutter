<img width="150" height="150" align="left" style="float: left; margin: 0 10px 0 0;" alt="Cutter logo" src="https://raw.githubusercontent.com/rizinorg/cutter/master/src/img/cutter.svg?sanitize=true">

# Cutter

Cutter is a free and open-source reverse engineering framework powered by [rizin](https://github.com/rizinorg/rizin) . Its goal is making an advanced, customizable and FOSS reverse-engineering platform while keeping the user experience in mind. Cutter is created by reverse engineers for reverse engineers.  

[![Cutter CI](https://github.com/rizinorg/cutter/workflows/Cutter%20CI/badge.svg)](https://github.com/rizinorg/cutter/actions?query=workflow%3A%22Cutter+CI%22)
[![Build status](https://ci.appveyor.com/api/projects/status/tn7kttv55b8wf799/branch/master?svg=true)](https://ci.appveyor.com/project/rizinorg/cutter/branch/master)
[![Total alerts](https://img.shields.io/lgtm/alerts/g/rizinorg/cutter.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/rizinorg/cutter/alerts/)

![Screenshot](https://raw.githubusercontent.com/rizinorg/cutter/master/docs/source/images/screenshot.png)

## Learn more at [https://cutter.re](https://cutter.re).

## Downloading a release

Cutter is available for all platforms (Linux, macOS, Windows).
You can download the latest release [here](https://github.com/rizinorg/cutter/releases).
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
Our community has built many plugins and useful scripts for Cutter such as the native integration of Ghidra decompiler or the plugin to visualize DynamoRIO code coverage. You can find more plugins in the [following list](https://github.com/rizinorg/cutter-plugins). Don't hesitate to extend it with your own plugins and scripts for Cutter.

## Help

The best place to obtain help from *Cutter* developers and community is to contact us on:

- **Telegram:** https://t.me/cutter_re
- **Mattermost:** https://im.rizin.re
- **IRC:** #cutter on irc.freenode.net
- **Twitter:** [@cutter_re](https://twitter.com/cutter_re)
