#!/bin/bash

cd $(dirname "${BASH_SOURCE[0]}")/..
mkdir -p cutter-deps && cd cutter-deps

LINUX_FILE="cutter-deps-linux.tar.gz"
LINUX_MD5=b52ab4f06cb9e4b92372022c37920bf1
LINUX_URL=https://github.com/radareorg/cutter-deps/releases/download/v6/cutter-deps-linux.tar.gz

MACOS_FILE="cutter-deps-macos.tar.gz"
MACOS_MD5=85b663ea73b80663a532dfafe998905d
MACOS_URL=https://github.com/radareorg/cutter-deps/releases/download/v6/cutter-deps-macos.tar.gz

UNAME_S="$(uname -s)"
if [ "$UNAME_S" == "Linux" ]; then
	FILE="${LINUX_FILE}"
	MD5="${LINUX_MD5}"
	URL="${LINUX_URL}"
elif [ "$UNAME_S" == "Darwin" ]; then
	FILE="${MACOS_FILE}"
	MD5="${MACOS_MD5}"
	URL="${MACOS_URL}"
else
	echo "Unsupported Platform: $UNAME_S"
	exit 1
fi

curl -L "$URL" -o "$FILE" || exit 1

if [ "$UNAME_S" == "Darwin" ]; then
	if [ "$(md5 -r "$FILE")" != "$MD5 $FILE" ]; then \
		echo "MD5 mismatch for file $FILE"; \
		exit 1; \
	else \
	        echo "$FILE OK"; \
	fi
else
	echo "$MD5 $FILE" | md5sum -c - || exit 1
fi

tar -xf "$FILE" || exit 1
./relocate.sh || exit 1

