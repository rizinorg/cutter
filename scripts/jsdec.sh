#!/bin/bash

set -e
INSTALL_PREFIX="$1"
EXTRA_CMAKE_OPTS="$2"

SCRIPTPATH=$(realpath "$(dirname "${BASH_SOURCE[0]}")")

cd "$SCRIPTPATH/.."

if [ ! -d jsdec ]; then
	git clone https://github.com/rizinorg/jsdec.git --depth 1 --branch "dev"
fi

cd jsdec
if [ -d build_lib ]; then
	rm -rf build_lib
fi
meson setup --buildtype=release --pkg-config-path="$INSTALL_PREFIX/lib/pkgconfig" -Dbuild_type=cutter build_lib
ninja -C build_lib

mkdir -p build_plugin && cd build_plugin
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DJSDEC_BUILD_DIR="../build_lib" -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" $EXTRA_CMAKE_OPTS ../cutter-plugin
ninja install
