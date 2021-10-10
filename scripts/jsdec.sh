#!/bin/bash

set -e

SCRIPTPATH=$(realpath "$(dirname "${BASH_SOURCE[0]}")")

cd "$SCRIPTPATH/.."

if [[ ! -d jsdec ]]; then
	git clone https://github.com/rizinorg/jsdec.git --depth 1 --branch v0.3.1
fi

cd jsdec
rm -rf build
mkdir build && cd build
meson --buildtype=release -Drizin_plugdir=share/rizin/plugins -Djsc_folder="../" --libdir=share/rizin/plugins --datadir=share/rizin/plugins "$@" ../p
ninja
ninja install

