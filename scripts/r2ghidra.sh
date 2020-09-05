#!/bin/bash

set -e

SCRIPTPATH=$(realpath "$(dirname "${BASH_SOURCE[0]}")")

cd "$SCRIPTPATH/.."

if [[ ! -d r2ghidra-dec ]]; then
    git clone --recurse-submodules https://github.com/karliss/r2ghidra-dec.git || exit 1
    pushd r2ghidra-dec
    git checkout  --recurse-submodules 5e845f4b50e8559bd51af03b22b6586e8cc5c35c
    popd
fi
cd r2ghidra-dec || exit 1

mkdir build && cd build || exit 1
cmake -G Ninja "$@" .. || exit 1
ninja || exit 1
ninja install || exit 1

