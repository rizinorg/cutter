#!/bin/bash

cd $(dirname "${BASH_SOURCE[0]}")/..
mkdir -p cutter-deps && cd cutter-deps

LINUX_FILE="cutter-deps-linux.tar.gz"
LINUX_MD5=8f9c10fd3cb8557ff7753f6fc602dd58
LINUX_URL=https://github.com/radareorg/cutter-deps/releases/download/v5/cutter-deps-linux.tar.gz

MACOS_FILE="cutter-deps-macos.tar.gz"
MACOS_MD5=a9d7c6156ac4e2d9097f8339d8a99c03
MACOS_URL=https://github.com/radareorg/cutter-deps/releases/download/v5/cutter-deps-macos.tar.gz

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

