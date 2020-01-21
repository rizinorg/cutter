#!/bin/bash

SCRIPTPATH=$(realpath "$(dirname "${BASH_SOURCE[0]}")")

cd "$SCRIPTPATH/.."

if [[ ! -d r2dec-js ]]; then
	git clone --depth 1 https://github.com/wargio/r2dec-js.git || exit 1
fi

cd r2dec-js || exit 1
rm -rf build || exit 1
mkdir build && cd build || exit 1
meson --buildtype=release --libdir=share/radare2/plugins --datadir=share/radare2/plugins "$@" ../p || exit 1
ninja || exit 1
ninja install || exit 1

