#!/bin/bash
set -e

SCRIPTPATH=$(realpath "$(dirname "${BASH_SOURCE[0]}")")

if [[ $# -lt 1 || $# -gt 2 ]]; then
	echo "Usage: $0 <install-prefix> [extra-cmake-opts]" >&2
	exit 1
fi
if [[ -z "$1" ]]; then
	echo "error: install prefix is empty" >&2
	exit 1
fi
INSTALL_PREFIX="$1"
EXTRA_CMAKE_OPTS="$2"

# keep in sync with rz-frida release
FRIDA_VERSION="17.17.0"

export GIT_TERMINAL_PROMPT=0

# compare versions
ver_newer() {
	awk -v a="$1" -v b="$2" '
		function ver(s) { split(s, v, "."); return v[1] * 1000000 + v[2] * 1000 + v[3] }
		BEGIN { exit !(ver(a) > ver(b)) }
	'
}

cd "$SCRIPTPATH/.."

if [[ ! -f rz-frida/meson.build ]]; then
	git clone https://github.com/rizinorg/rz-frida.git --depth 1 rz-frida
fi

cd rz-frida

if [[ ! -f frida-core-devkit/frida-core.h ]]; then
	case "$(uname -s)" in
		Linux) FRIDA_PLATFORM="linux-x86_64" ;;
		Darwin) FRIDA_PLATFORM="macos-$(uname -m)" ;;
		*) echo "unsupported platform: $(uname -s)" >&2; exit 1 ;;
	esac
	curl -sSL --retry 3 --connect-timeout 30 --max-time 900 -o frida-core-devkit.tar.xz \
		"https://github.com/frida/frida/releases/download/${FRIDA_VERSION}/frida-core-devkit-${FRIDA_VERSION}-${FRIDA_PLATFORM}.tar.xz"
	mkdir -p frida-core-devkit
	tar xf frida-core-devkit.tar.xz -C frida-core-devkit
fi

if [[ "$(uname -s)" == "Darwin" ]]; then
	# frida devkit is built against a specific minimum macOS, link at exactly
	# that floor
	FRIDA_MIN_OS=$(otool -l frida-core-devkit/libfrida-core.a 2>/dev/null \
		| awk '/^ *minos / {print $2}' | sort -V | tail -1)
	if [[ -z "$FRIDA_MIN_OS" ]] || ! ver_newer "$FRIDA_MIN_OS" "10.15"; then
		export MACOSX_DEPLOYMENT_TARGET="10.15"
	else
		export MACOSX_DEPLOYMENT_TARGET="$FRIDA_MIN_OS"
	fi
fi

# remove leftovers when this script reruns (like retried make package)
rm -rf build
meson setup build --buildtype=release --pkg-config-path="$INSTALL_PREFIX/lib/pkgconfig" --prefix="$INSTALL_PREFIX" \
	-Dfrida_core=enabled \
	-Dfrida_include_dir="$PWD/frida-core-devkit" \
	-Dfrida_library="$PWD/frida-core-devkit/libfrida-core.a"
ninja -C build install

FRIDA_PLUGIN_INSTALLED=""
for cand in \
	"$SCRIPTPATH/../build/Rizin-prefix/lib/rizin/plugins/librz_frida.dylib" \
	"$SCRIPTPATH/../build/Rizin-prefix/lib/rizin/plugins/librz_frida.so" \
	"$INSTALL_PREFIX/lib/rizin/plugins/librz_frida.dylib" \
	"$INSTALL_PREFIX/lib/rizin/plugins/librz_frida.so"; do
	if [[ -f "$cand" ]]; then
		FRIDA_PLUGIN_INSTALLED="$cand"
		break
	fi
done
if [[ -n "$FRIDA_PLUGIN_INSTALLED" ]]; then
	if [[ "$(uname -s)" == "Darwin" ]]; then
		install_name_tool -id "@rpath/$(basename "$FRIDA_PLUGIN_INSTALLED")" "$FRIDA_PLUGIN_INSTALLED"
		while read -r rzlib; do
			install_name_tool -change "$rzlib" "@rpath/$(basename "$rzlib")" "$FRIDA_PLUGIN_INSTALLED"
		done < <(otool -L "$FRIDA_PLUGIN_INSTALLED" | awk '$1 ~ /Rizin-prefix\/lib\/lib/ {print $1}')
	fi
	FRIDA_PLUGIN_DEST="$INSTALL_PREFIX/lib/rizin/plugins/$(basename "$FRIDA_PLUGIN_INSTALLED")"
	if [[ "$FRIDA_PLUGIN_INSTALLED" != "$FRIDA_PLUGIN_DEST" ]]; then
		mkdir -p "$INSTALL_PREFIX/lib/rizin/plugins"
		cp -f "$FRIDA_PLUGIN_INSTALLED" "$FRIDA_PLUGIN_DEST"
	fi
fi

cd plugin/cutter
mkdir -p build && cd build
export PKG_CONFIG_PATH="$INSTALL_PREFIX/lib/pkgconfig"
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
	-DCMAKE_PREFIX_PATH="$INSTALL_PREFIX;$SCRIPTPATH/../cutter-deps/qt" $EXTRA_CMAKE_OPTS ..
ninja install
