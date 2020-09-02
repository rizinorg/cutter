#!/bin/bash

set -e

SCRIPTPATH=$(realpath "$(dirname "${BASH_SOURCE[0]}")")

cd "$SCRIPTPATH/.."

if [[ ! -d r2dec-js ]]; then
	git clone https://github.com/wargio/r2dec-js.git
	cd r2dec-js
	git checkout b5a0d15c7bcc488f268ffb0931b7ced2919f6c9d
	cd ..
fi

cd r2dec-js
rm -rf build
mkdir build && cd build
meson --buildtype=release --libdir=share/radare2/plugins --datadir=share/radare2/plugins "$@" ../p
ninja
ninja install

