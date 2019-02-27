#!/bin/bash

cd $(dirname "${BASH_SOURCE[0]}")/..
mkdir -p cutter-deps && cd cutter-deps

FILE=cutter-deps.tar.gz
MD5=0805fa6a1626ce787f952b300e2b321d
URL=https://github.com/radareorg/cutter-deps/releases/download/v2/cutter-deps.tar.gz

curl -L "$URL" -o "$FILE" || exit 1
echo "$MD5 $FILE" | md5sum -c - || exit 1
tar -xf "$FILE" || exit 1
./relocate.sh || exit 1

