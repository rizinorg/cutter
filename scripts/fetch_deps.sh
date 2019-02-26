#!/bin/bash

cd $(dirname "${BASH_SOURCE[0]}")/..
mkdir -p cutter-deps && cd cutter-deps

FILE=cutter-deps.tar.gz
MD5=75f6321311266b5326de1b9e8cb65ef2
URL=https://github.com/thestr4ng3r/cutter-deps/releases/download/untagged-e4ad7bf5c4c756f4862d/cutter-deps.tar.gz

curl -L "$URL" -o "$FILE" || exit 1
echo "$MD5 $FILE" | md5sum -c - || exit 1
tar -xf "$FILE" || exit 1
./relocate.sh || exit 1

