#!/bin/bash

set -e

cd $(dirname "${BASH_SOURCE[0]}")/..
mkdir -p cutter-deps && cd cutter-deps

DEPS_BASE_URL=https://github.com/karliss/cutter-deps/releases/download/qt6-test #TODO: replace before merging

if [ "$CUTTER_QT" == "5" ]; then
	DEPS_FILE_linux_x86_64=cutter-deps-linux-x86_64.tar.gz
	DEPS_SHA256_linux_x86_64=0721c85548bbcf31f6911cdb2227e5efb4a20c34262672d4cd2193db166b2f8c
	DEPS_BASE_URL=https://github.com/rizinorg/cutter-deps/releases/download/v15
else
	DEPS_FILE_linux_x86_64=cutter-deps-linux-x86_64.tar.gz
	DEPS_SHA256_linux_x86_64=c9292eda751ec6cef6454e1b702ca4f9942077aad7a2887bb828de5bc11a5e3a
fi
echo $DEPS_SHA256_linux_x86_64

DEPS_FILE_macos_x86_64=cutter-deps-macos-x86_64.tar.gz
DEPS_SHA256_macos_x86_64=fa0a245f1c6cb89284b5dd5af7ac68b340814d6584cf891617f7f2ce8ae30c69

DEPS_FILE_macos_arm64=cutter-deps-macos-arm64.tar.gz
DEPS_SHA256_macos_arm64=ebca9ada787a287161a7bd69b943ac93abd108a5a888242dbf7dd5710c2ab1b2

DEPS_FILE_win_x86_64=cutter-deps-win-x86_64.tar.gz
DEPS_SHA256_win_x86_64=6b730cc4fb17b1c4880e3ce1ce42fb07ac0e98905869c6bdc8d93a9b05b7db2d


ARCH=x86_64
if [ "$OS" == "Windows_NT" ]; then
	PLATFORM=win
else
	UNAME_S="$(uname -s)"
	if [ "$UNAME_S" == "Linux" ]; then
		PLATFORM=linux
	elif [ "$UNAME_S" == "Darwin" ]; then
		PLATFORM=macos
		ARCH=$(uname -m)
	else
		echo "Unsupported Platform: uname -s => $UNAME_S, \$OS => $OS"
		exit 1
	fi
fi

DEPS_FILE=DEPS_FILE_${PLATFORM}_${ARCH}
DEPS_FILE=${!DEPS_FILE}
DEPS_SHA256=DEPS_SHA256_${PLATFORM}_${ARCH}
DEPS_SHA256=${!DEPS_SHA256}
DEPS_URL=${DEPS_BASE_URL}/${DEPS_FILE}

SHA256SUM=sha256sum
if ! command -v ${SHA256SUM} &> /dev/null; then
	SHA256SUM="shasum -a 256"
fi

curl -L "$DEPS_URL" -o "$DEPS_FILE" || exit 1
echo "$DEPS_SHA256  $DEPS_FILE" | ${SHA256SUM} -c - || exit 1

tar -xf "$DEPS_FILE" || exit 1

if [ -f relocate.sh ]; then
	./relocate.sh || exit 1
fi

