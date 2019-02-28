#!/bin/bash

if ! [[ $# -eq 1 ]]; then
    echo "Usage: $0 [appdir]"
    exit 1
fi

python_version=python3.6

python_prefix=$(pkg-config --variable=prefix python3)
appdir=$1

echo "Embedding Python from prefix $python_prefix in appdir $appdir"

mkdir -p "$appdir/usr"
cd "$appdir/usr/" || exit 1

cp -RT "$python_prefix" "." || exit 1
echo "Cleaning up embedded Python"
find . | grep -E "(__pycache__|\.pyc|\.pyo$)" | xargs rm -rf
rm -r lib/$python_version/test lib/$python_version/idlelib lib/$python_version/curses lib/$python_version/lib2to3

echo "Checking if PySide2 is available"

pyside_prefix=$(pkg-config --variable=prefix pyside2)
if [ $? -ne 0 ]; then
	echo "PySide2 is not available, ignoring."
	exit 0
fi

echo "PySide is at $pyside_prefix"

if [ "$pyside_prefix" == "$python_prefix" ]; then
	echo "Prefixes are equal, not copying anything from lib"
else
	cp -RT "$pyside_prefix/lib/$python_version" "lib/$python_version" || exit 1
fi
