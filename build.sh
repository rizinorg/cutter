#!/bin/sh

# This script is a work in progress

####   Constants    ####
ERR=0

#### User variables ####
BUILD="build"
QMAKE_CONF=""
ROOT_DIR=`pwd`
#QMAKE_CONF="CUTTER_ENABLE_JUPYTER=false CUTTER_ENABLE_QTWEBENGINE=false"

# Create translations
lrelease ./src/Cutter.pro

check_r2() {
	r2 -v >/dev/null 2>&1
	if [ $? = 0 ]; then
		R2COMMIT=$(r2 -v | tail -n1 | sed "s,commit: \\(.*\\) build.*,\\1,")
		SUBMODULE=$(git submodule | grep "radare2" | awk '{print $1}')
		if [ "$R2COMMIT" = "$SUBMODULE" ]; then
			return 0
		fi
	fi
	return 1
}

find_qmake() {
	qmakepath=$(which qmake-qt5)
	if [ -z "$qmakepath" ]; then
		qmakepath=$(which qmake)
	fi
	if [ -z "$qmakepath" ]; then
		echo "You need qmake to build Cutter."
		echo "Please make sure qmake is in your PATH environment variable."
		exit 1
	fi
	echo "$qmakepath"
}

find_gmake() {
	gmakepath=$(which gmake)
	if [ -z "$gmakepath" ]; then
		gmakepath=$(which make)
	fi

	${gmakepath} --help 2>&1 | grep -q gnu
	if [ $? != 0 ]; then
		echo "You need GNU make to build Cutter."
		echo "Please make sure gmake is in your PATH environment variable."
		exit 1
	fi
	echo "$gmakepath"
}

# Build radare2
check_r2
if [ $? -eq 1 ]; then
    printf "A (new?) version of radare2 will be installed. Do you agree? [Y/n] "
    read answer
    if [ -z "$answer" -o "$answer" = "Y" -o "$answer" = "y" ]; then
        R2PREFIX=${1:-"/usr"}
        git submodule init && git submodule update
        cd radare2 || exit 1
        ./sys/install.sh "$R2PREFIX"
        cd ..
    else
        echo "Sorry but this script won't work otherwise. Read the README."
        exit 1
    fi
else
    echo "Correct radare2 version found, skipping..."
fi

# Build
mkdir -p "$BUILD"
cd "$BUILD" || exit 1
$(find_qmake) ../src/Cutter.pro $QMAKE_CONF
$(find_gmake) -j4
ERR=$((ERR+$?))

# Move translations
mkdir -p "`pwd`/translations"
find "$ROOT_DIR/src/translations" -maxdepth 1  -type f | grep "cutter_..\.qm" | while read SRC_FILE; do
    mv $SRC_FILE "`pwd`/translations"
done

# Finish
if [ ${ERR} -gt 0 ]; then
    echo "Something went wrong!"
else
    echo "Build complete."
	printf "This build of Cutter will be installed. Do you agree? [Y/n] "
    read answer
    if [ -z "$answer" -o "$answer" = "Y" -o "$answer" = "y" ]; then
		$(find_gmake) install
	else
		echo "Binary available at $BUILD/Cutter"
	fi
fi

cd ..
