#!/bin/sh

# This script is a work in progress

####   Constants    ####
ERR=0

#### User variables ####
BUILD="$(pwd)/build"
QMAKE_CONF=$*
ROOT_DIR=$(pwd)

find_qmake() {
	qmakepath=$(command -v qmake-qt5)
	if [ -z "$qmakepath" ]; then
		qmakepath=$(command -v qmake)
	fi
	if [ -z "$qmakepath" ]; then
		echo "You need qmake to build Cutter."
		echo "Please make sure qmake is in your PATH environment variable."
		exit 1
	fi
	echo "$qmakepath"
}

find_lrelease() {
	lreleasepath=$(command -v lrelease-qt5)
	if [ -z "$lreleasepath" ]; then
		lreleasepath=$(command -v lrelease)
	fi
	if [ -z "$lreleasepath" ]; then
		echo "You need lrelease to build Cutter."
		echo "Please make sure lrelease is in your PATH environment variable."
		exit 1
	fi
	echo "$lreleasepath"
}

find_gmake() {
	gmakepath=$(command -v gmake)
	if [ -z "$gmakepath" ]; then
		gmakepath=$(command -v make)
	fi

	${gmakepath} --help 2>&1 | grep -q gnu
	if [ $? != 0 ]; then
		echo "You need GNU make to build Cutter."
		echo "Please make sure gmake is in your PATH environment variable."
		exit 1
	fi
	echo "$gmakepath"
}

prepare_breakpad() {
    OS="$(uname -s)"
	if [ -z "$OS" ]; then
		echo "Could not identify OS, OSTYPE var is empty. You can try to disable breakpad to avoid this error."
		exit 1
	fi

	if [ "$OS" = "Linux" ]; then
		. $ROOT_DIR/scripts/prepare_breakpad_linux.sh
		export PKG_CONFIG_PATH="$CUSTOM_BREAKPAD_PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH"
	elif [ "$OS" = "Darwin" ]; then
		. $ROOT_DIR/scripts/prepare_breakpad_macos.sh
	fi
}

# Create translations
$(find_lrelease) ./src/Cutter.pro

# Build
if [ "${QMAKE_CONF#*CUTTER_ENABLE_CRASH_REPORTS=true}" != "$QMAKE_CONF" ]; then
	prepare_breakpad
fi
mkdir -p "$BUILD"
cd "$BUILD" || exit 1
$(find_qmake) ../src/Cutter.pro "$QMAKE_CONF"
$(find_gmake) -j4
ERR=$((ERR+$?))

# Move translations
mkdir -p "$(pwd)/translations"
find "$ROOT_DIR/src/translations" -maxdepth 1  -type f | grep "cutter_..\.qm" | while read -r SRC_FILE; do
    mv "$SRC_FILE" "$(pwd)/translations"
done

# Finish
if [ ${ERR} -gt 0 ]; then
    echo "Something went wrong!"
else
    echo "Build complete."
	printf "This build of Cutter will be installed. Do you agree? [Y/n] "
    read -r answer
    if [ -z "$answer" ] || [ "$answer" = "Y" ] || [ "$answer" = "y" ]; then
		$(find_gmake) install
	else
		echo "Binary available at $BUILD/Cutter"
	fi
fi

cd ..
