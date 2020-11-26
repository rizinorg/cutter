#!/bin/bash

set -eu

scripts/rz-ghidra.sh \
	-DCMAKE_C_COMPILER=cl \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_PREFIX_PATH="$APPVEYOR_BUILD_FOLDER/rz_dist;$APPVEYOR_BUILD_FOLDER/rz_dist/include/librz;$APPVEYOR_BUILD_FOLDER/rz_dist/include/librz/sdb" \
	-DCMAKE_INSTALL_PREFIX="$APPVEYOR_BUILD_FOLDER/rz_dist" \
	-DRIZIN_INSTALL_PLUGDIR="$APPVEYOR_BUILD_FOLDER/rz_dist/lib/plugins" \
	-DBUILD_SLEIGH_PLUGIN=OFF
