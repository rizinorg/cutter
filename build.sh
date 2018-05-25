#!/bin/sh

# This script is a work in progress

####   Constants    ####
BUILDR2=1
ERR=0

#### User variables ####
BUILD="build"
QMAKE_CONF=""
#QMAKE_CONF="CUTTER_ENABLE_JUPYTER=false CUTTER_ENABLE_QTWEBENGINE=false"

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
mkdir "$BUILD"
cd "$BUILD" || exit 1
"$qmakepath" "$QMAKE_CONF" ../src/Cutter.pro
make -j4
ERR=$((ERR+$?))
cd ..

# Done
if [ ${ERR} -gt 0 ]; then
	echo "Something went wrong!"
else
	echo "Build complete. Binary available at: $BUILD/Cutter"
fi

