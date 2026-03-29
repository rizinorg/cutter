#!/bin/bash
set -e

SCRIPTPATH=$(realpath "$(dirname "${BASH_SOURCE[0]}")")
INSTALL_PREFIX="$1"
EXTRA_CMAKE_OPTS="$2"

cd "$SCRIPTPATH/.."

if [[ ! -d rz-silhouette ]]; then
	git clone https://github.com/rizinorg/rz-silhouette.git --depth 1 rz-silhouette
fi

cd rz-silhouette

if ! command -v capnp >/dev/null 2>&1; then
	echo "capnp is required to build rz-silhouette. Install the Cap'n Proto compiler and retry." >&2
	exit 1
fi

meson --buildtype=release --pkg-config-path="$INSTALL_PREFIX/lib/pkgconfig" --prefix="$INSTALL_PREFIX" build
ninja -C build install
