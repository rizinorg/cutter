#!/bin/bash
set -e

SCRIPTPATH=$(realpath "$(dirname "${BASH_SOURCE[0]}")")
INSTALL_PREFIX="$1"
EXTRA_CMAKE_OPTS="$2"

# keep in sync with rz-frida release
FRIDA_VERSION="17.17.0"

cd "$SCRIPTPATH/.."

# url to be updated after rz-frida gets transferred under rizinorg
if [[ ! -d rz-frida ]]; then
	git clone https://github.com/IndAlok/rz-frida.git --depth 1 rz-frida
fi

cd rz-frida

if [[ ! -d frida-core-devkit ]]; then
	case "$(uname -s)" in
		Linux) FRIDA_PLATFORM="linux-x86_64" ;;
		Darwin) FRIDA_PLATFORM="macos-$(uname -m)" ;;
		*) echo "unsupported platform: $(uname -s)" >&2; exit 1 ;;
	esac
	curl -sL -o frida-core-devkit.tar.xz "https://github.com/frida/frida/releases/download/${FRIDA_VERSION}/frida-core-devkit-${FRIDA_VERSION}-${FRIDA_PLATFORM}.tar.xz"
	mkdir -p frida-core-devkit
	tar xf frida-core-devkit.tar.xz -C frida-core-devkit
fi

meson --buildtype=release --pkg-config-path="$INSTALL_PREFIX/lib/pkgconfig" --prefix="$INSTALL_PREFIX" \
	-Dfrida_core=enabled \
	-Dfrida_include_dir="$PWD/frida-core-devkit" \
	-Dfrida_library="$PWD/frida-core-devkit/libfrida-core.a" build
ninja -C build install

cd plugin/cutter
mkdir -p build && cd build
export PKG_CONFIG_PATH="$INSTALL_PREFIX/lib/pkgconfig"
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
	-DCMAKE_PREFIX_PATH="$INSTALL_PREFIX;$SCRIPTPATH/../cutter-deps/qt" $EXTRA_CMAKE_OPTS ..
ninja install
