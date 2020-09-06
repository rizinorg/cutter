#!/bin/bash

set -e

SCRIPTPATH=$(realpath "$(dirname "${BASH_SOURCE[0]}")")

cd "$SCRIPTPATH/.."

if [[ ! -d r2ghidra ]]; then
    # remove depth, branch and uncomment lines bellow to use specifc commit
    git clone --recurse-submodules https://github.com/radareorg/r2ghidra.git  --depth 1  --branch master || exit 1
    #pushd r2ghidra
    #git checkout  --recurse-submodules 5e845f4b50e8559bd51af03b22b6586e8cc5c35c
    #popd
fi
cd r2ghidra || exit 1

mkdir build && cd build || exit 1
cmake -G Ninja "$@" .. || exit 1
ninja || exit 1
ninja install || exit 1

