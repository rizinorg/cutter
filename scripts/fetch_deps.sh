#!/bin/bash

cd $(dirname "${BASH_SOURCE[0]}")/..
mkdir -p cutter-deps && cd cutter-deps

FILE=cutter-deps.tar.gz
MD5=e150049c5587fbaa581fa7e7266f3f3c
URL=https://github.com/thestr4ng3r/cutter-deps/releases/download/untagged-8ab99b98f962294e705b/cutter-deps.tar.gz

curl -L "$URL" -o "$FILE" || exit 1
echo "$MD5 $FILE" | md5sum -c - || exit 1
tar -xf "$FILE" || exit 1
./relocate.sh || exit 1

