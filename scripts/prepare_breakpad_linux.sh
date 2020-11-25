#!/bin/sh

set -euo pipefail

git clone https://github.com/google/breakpad.git
cd breakpad
git clone https://chromium.googlesource.com/linux-syscall-support src/third_party/lss
CFLAGS=-w CXXFLAGS=-w ./configure --disable-tools --prefix=`pwd`/prefix && make -j4 && make install || exit 1

export CUSTOM_BREAKPAD_PREFIX="`pwd`/prefix"
cd ..
