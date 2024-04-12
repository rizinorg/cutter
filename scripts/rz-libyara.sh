#!/bin/bash
set -e

SCRIPTPATH=$(realpath "$(dirname "${BASH_SOURCE[0]}")")
INSTALL_PREFIX="$1"
EXTRA_CMAKE_OPTS="$2"

cd "$SCRIPTPATH/.."

if [[ ! -d rz_libyara ]]; then
    git clone https://github.com/rizinorg/rz-libyara.git --depth 1 --branch main rz_libyara
    git -C rz_libyara submodule init
    git -C rz_libyara submodule update
fi

cd rz_libyara

meson --buildtype=release --pkg-config-path="$INSTALL_PREFIX/lib/pkgconfig" --prefix="$INSTALL_PREFIX" -Denable_openssl=false -Duse_sys_yara=disabled build
ninja -C build install

cd cutter-plugin
mkdir build && cd build
cmake -G Ninja -DRIZIN_INSTALL_PLUGDIR="../build" -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" $EXTRA_CMAKE_OPTS ..
ninja
ninja install
