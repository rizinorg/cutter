#!/bin/bash

set -e

cd $(dirname "${BASH_SOURCE[0]}")/..
mkdir -p cutter-deps && cd cutter-deps

DEPS_BASE_URL=https://github.com/rizinorg/cutter-deps/releases/download/v18

if [ "$CUTTER_QT" == "5" ]; then
	DEPS_FILE_linux_x86_64=cutter-deps-q5-linux-x86_64.tar.gz
	DEPS_SHA256_linux_x86_64=ab3099fe699db100f2d00e1b70cdf77dec6b8fdd9cd1709c96e123a15fb62571
	DEPS_BASE_URL=https://github.com/rizinorg/cutter-deps/releases/download/qt5-v17
else
	DEPS_FILE_linux_x86_64=cutter-deps-linux-x86_64.tar.gz
	DEPS_SHA256_linux_x86_64=204683c44e4824df7cb9925f1939544f081e64b7a701fe95fd03f67125a165b4
fi
echo $DEPS_SHA256_linux_x86_64

DEPS_FILE_macos_x86_64=cutter-deps-macos-x86_64.tar.gz
DEPS_SHA256_macos_x86_64=a46c2dee12d4160410237da848075fc87c41c3850021f583d3b469a59c386650

DEPS_FILE_macos_arm64=cutter-deps-macos-arm64.tar.gz
DEPS_SHA256_macos_arm64=1ca381b562d632a370e05a85b0fe60a591a261bba0f48d97a0e40b4307c2577a

DEPS_FILE_win_x86_64=cutter-deps-win-x86_64.tar.gz
DEPS_SHA256_win_x86_64=95a3cf676178b8c58442d22ee8b21ce2530bfdae86f514901310490a8f28867a


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

