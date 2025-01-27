#!/bin/bash

set -e

cd $(dirname "${BASH_SOURCE[0]}")/..
mkdir -p cutter-deps && cd cutter-deps

DEPS_BASE_URL=https://github.com/rizinorg/cutter-deps/releases/download/v16

if [ "$CUTTER_QT" == "5" ]; then
	DEPS_FILE_linux_x86_64=cutter-deps-q5-linux-x86_64.tar.gz
	DEPS_SHA256_linux_x86_64=ab3099fe699db100f2d00e1b70cdf77dec6b8fdd9cd1709c96e123a15fb62571
	DEPS_BASE_URL=https://github.com/rizinorg/cutter-deps/releases/download/qt5-v17
else
	DEPS_FILE_linux_x86_64=cutter-deps-linux-x86_64.tar.gz
	DEPS_SHA256_linux_x86_64=f63c5af2d9872bc6538a94c839d6ef6645c7630c42cff30f1d9da8eefd9eb040
fi
echo $DEPS_SHA256_linux_x86_64

DEPS_FILE_macos_x86_64=cutter-deps-macos-x86_64.tar.gz
DEPS_SHA256_macos_x86_64=bcdc214e34dc3fd720327ad42e03fe3ec996ca28a9987e99898f149a65299a8c

DEPS_FILE_macos_arm64=cutter-deps-macos-arm64.tar.gz
DEPS_SHA256_macos_arm64=aa3f5ae91b93c5176d6bd4313af0888a2b6dcdaa2ef1750dd7e2f98156882e0f

DEPS_FILE_win_x86_64=cutter-deps-win-x86_64.tar.gz
DEPS_SHA256_win_x86_64=710e40cf8329205d09535cc56a9fb155a56ff1a1ca112145864382fb3d4e8160


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

