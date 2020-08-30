#!/bin/bash

cd $(dirname "${BASH_SOURCE[0]}")/..
mkdir -p cutter-deps && cd cutter-deps

LINUX_FILE="cutter-deps-linux.tar.gz"
LINUX_MD5=31fd19443a3405d6b6097cbbd4c11fba
LINUX_URL=https://github.com/radareorg/cutter-deps/releases/download/v12/cutter-deps-linux.tar.gz

MACOS_FILE="cutter-deps-macos.tar.gz"
MACOS_MD5=e75041c04fc806437723a399028402af
MACOS_URL=https://github.com/radareorg/cutter-deps/releases/download/v12/cutter-deps-macos.tar.gz

WIN_FILE="cutter-deps-win.tar.gz"
WIN_MD5=7c755404140f2e9945bfc13d2e645bb1
WIN_URL=https://github.com/radareorg/cutter-deps/releases/download/v12/cutter-deps-win.tar.gz

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

