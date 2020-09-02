#!/bin/bash

set -e

SCRIPTPATH=$(realpath "$(dirname "${BASH_SOURCE[0]}")")

cd "$SCRIPTPATH/.."

if [[ ! -d r2ghidra-dec ]]; then
    git clone --recurse-submodules https://github.com/radareorg/r2ghidra-dec.git || exit 1
    pushd r2ghidra-dec
    git checkout  --recurse-submodules 8e576eeadc211de4ac8d8c759cc368fa48cdfa99
    popd
fi
cd r2ghidra-dec || exit 1

mkdir build && cd build || exit 1
cmake -G Ninja "$@" .. || exit 1
ninja || exit 1
ninja install || exit 1

