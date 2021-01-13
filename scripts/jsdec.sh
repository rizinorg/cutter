#!/bin/bash

set -e

SCRIPTPATH=$(realpath "$(dirname "${BASH_SOURCE[0]}")")

cd "$SCRIPTPATH/.."

if [[ ! -d jsdec ]]; then
	git clone https://github.com/rizinorg/jsdec.git --depth 1 --branch master
fi

cd jsdec
rm -rf build
mkdir build && cd build
meson --buildtype=release --libdir=share/rizin/plugins --datadir=share/rizin/plugins "$@" ../p
ninja
ninja install

