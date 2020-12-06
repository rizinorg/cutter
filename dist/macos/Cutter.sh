#!/bin/bash

echo "Cutter Launch Script for macOS 🥞"

HERE=$(dirname "$0")
PREFIX=$HERE/../Resources
export DYLD_LIBRARY_PATH="$PREFIX/lib"
export DYLD_FRAMEWORK_PATH="$PREFIX/lib"
"$HERE/Cutter.bin" "$@"
