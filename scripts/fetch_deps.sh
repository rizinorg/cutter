#!/bin/bash

FILE=cutter-deps.tar.gz
MD5=6b25169d8a4228df0516892875934166
URL=https://github.com/thestr4ng3r/cutter-deps/releases/download/untagged-53327f1ed48017700884/cutter-deps.tar.gz

curl -L "$URL" -o "$FILE" || exit 1
echo "$MD5 $FILE" | md5sum -c - || exit 1
tar -xf "$FILE" || exit 1
./relocate.sh || exit 1

