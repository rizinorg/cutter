#!/bin/bash

set -eu

git clone --recurse-submodules https://github.com/radareorg/r2ghidra-dec.git $APPVEYOR_BUILD_FOLDER/r2ghidra-dec
pushd $APPVEYOR_BUILD_FOLDER/r2ghidra-dec
git checkout --recurse-submodules 8e576eeadc211de4ac8d8c759cc368fa48cdfa99
popd

scripts/r2ghidra.sh \
	-DCMAKE_C_COMPILER=cl \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_PREFIX_PATH="$APPVEYOR_BUILD_FOLDER/r2_dist;$APPVEYOR_BUILD_FOLDER/r2_dist/include/libr;$APPVEYOR_BUILD_FOLDER/r2_dist/include/libr/sdb;$QT64PATH" \
	-DCMAKE_INSTALL_PREFIX="$APPVEYOR_BUILD_FOLDER/r2_dist" \
	-DRADARE2_INSTALL_PLUGDIR="$APPVEYOR_BUILD_FOLDER/r2_dist/lib/plugins"