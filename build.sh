#!/bin/sh

# This script is a work in progress

####   Constants    ####
BUILDR2=1
ERR=0

#### User variables ####
BUILD="build"
QMAKE_CONF=""
ROOT_DIR=`pwd`
#QMAKE_CONF="CUTTER_ENABLE_JUPYTER=false CUTTER_ENABLE_QTWEBENGINE=false"

# Create translations
lrelease ./src/Cutter.pro

# Checking for radare2
r2 -v >/dev/null 2>&1
if [ $? = 0 ]; then
	R2COMMIT=$(r2 -v | tail -n1 | sed "s,commit: \\(.*\\) build.*,\\1,")
	SUBMODULE=$(git submodule | awk '{print $1}')
	if [ "$R2COMMIT" = "$SUBMODULE" ]; then
		BUILDR2=0
	fi
fi

# Check if either qmake or qmake-qt5 is available
qmakepath=$(which qmake-qt5)
if [ -z "$qmakepath" ]; then
	qmakepath=$(which qmake)
fi
if [ -z "$qmakepath" ]; then
	echo "You need qmake to build Cutter."
	echo "Please make sure qmake is in your PATH environment variable."
	exit 1
fi

# Build radare2
if [ $BUILDR2 -eq 1 ]; then
	answer="Y"
	printf "A (new?) version of radare2 will be installed. Do you agree? [Y/n] "
	read answer
	if [ -z "$answer" -o "$answer" = "Y" -o "$answer" = "y" ]; then
		git submodule init && git submodule update
		cd radare2 || exit 1
		./sys/install.sh
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
"$qmakepath" ../src/Cutter.pro $QMAKE_CONF
make -j4
ERR=$((ERR+$?))

# Move translations
mkdir -p "`pwd`/translations"
find "$ROOT_DIR/src/translations" -maxdepth 1  -type f | grep "cutter_..\.qm" | while read SRC_FILE; do
    mv $SRC_FILE "`pwd`/translations"
done

cd ..

# Done
if [ ${ERR} -gt 0 ]; then
	echo "Something went wrong!"
else
	echo "Build complete. Binary available at: $BUILD/Cutter"
fi

