#!/bin/bash

SCRIPTPATH=$(realpath "$(dirname "${BASH_SOURCE[0]}")")

cd "$SCRIPTPATH/.."

if [[ ! -d r2ghidra-dec ]]; then
    git clone --depth 1 --branch decompiler-refactoring --recurse-submodules https://github.com/radareorg/r2ghidra-dec.git || exit 1
fi
cd r2ghidra-dec || exit 1

mkdir build && cd build || exit 1
cmake -G Ninja "$@" .. || exit 1
ninja || exit 1
ninja install || exit 1

