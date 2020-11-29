#!/bin/bash

cd $(dirname "${BASH_SOURCE[0]}")/..
mkdir -p cutter-deps && cd cutter-deps

LINUX_FILE="cutter-deps-linux.tar.gz"
LINUX_MD5=63fb136f9c2b1da0b881e460030bff7f
LINUX_URL=https://github.com/rizinorg/cutter-deps/releases/download/test-v13/cutter-deps-linux.tar.gz

MACOS_FILE="cutter-deps-macos.tar.gz"
MACOS_MD5=44e490ea45985cfedb5f7882acd36fa2
MACOS_URL=https://github.com/rizinorg/cutter-deps/releases/download/test-v13/cutter-deps-macos.tar.gz

WIN_FILE="cutter-deps-win.tar.gz"
WIN_MD5=f7bfcf69da0c9bbe96f7c34dc149d3b9
WIN_URL=https://github.com/rizinorg/cutter-deps/releases/download/test-v13/cutter-deps-win.tar.gz

if [ "$OS" == "Windows_NT" ]; then
	FILE="${WIN_FILE}"
	MD5="${WIN_MD5}"
	URL="${WIN_URL}"
else
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
		echo "Unsupported Platform: uname -s => $UNAME_S, \$OS => $OS"
		exit 1
	fi
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

if [ -f relocate.sh ]; then
	./relocate.sh || exit 1
fi

