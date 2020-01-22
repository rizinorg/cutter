#!/bin/bash

set -e

SCRIPTPATH=$(realpath "$(dirname "${BASH_SOURCE[0]}")")

cd "$SCRIPTPATH/.."

if [[ ! -d r2dec-js ]]; then
	git clone --depth 1 https://github.com/wargio/r2dec-js.git
fi

cd r2dec-js
rm -rf build
mkdir build && cd build
meson --buildtype=release --libdir=share/radare2/plugins --datadir=share/radare2/plugins "$@" ../p
ninja
ninja install

