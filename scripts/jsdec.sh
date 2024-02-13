#!/bin/bash

set -e

SCRIPTPATH=$(realpath "$(dirname "${BASH_SOURCE[0]}")")

cd "$SCRIPTPATH/.."

if [ ! -d jsdec ]; then
	git clone https://github.com/rizinorg/jsdec.git --depth 1 --branch "v0.7.0"
fi

cd jsdec
if [ -d build ]; then
	rm -rf build
fi
meson --buildtype=release "$@" build
ninja -C build
ninja -C build install

