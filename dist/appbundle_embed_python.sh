#!/bin/bash

set -euo pipefail
if ! [[ $# -eq 3 ]]; then
    echo "Usage: $0 [Python.framework] [AppBundle.app] [AppBundle.app/Contents/MacOS/Executable]"
    exit 1
fi

python_version=python3.12

py_framework=$1
appbundle=$2
executable=$3

echo "Embedding $py_framework into $appbundle | $executable"

mkdir -p "$appbundle/Contents/Frameworks"
if [ ! -d "$appbundle/Contents/Frameworks/Python.framework" ]
then
    cp -a -n "$py_framework" "$appbundle/Contents/Frameworks/"
    echo "Cleaning up embedded Python Framework"
    cd "$appbundle/Contents/Frameworks/Python.framework"
    find . | grep -E "(__pycache__|\.pyc|\.pyo$)" | xargs rm -rf
    rm -r Versions/Current/Resources/Python.app "Versions/Current/lib/$python_version/test" "Versions/Current/lib/$python_version/idlelib" "Versions/Current/lib/$python_version/curses" "Versions/Current/lib/$python_version/lib2to3" || echo "Couldn't remove something"
else
    echo "Python.framework already exists, skipping copying"
    cd "$appbundle/Contents/Frameworks/Python.framework"
fi

echo "Making executable $executable point to embedded Framework"
install_name_tool -change `otool -L "$executable" | sed -n "s/^[[:blank:]]*\([^[:blank:]]*Python\) (.*$/\1/p"` @executable_path/../Frameworks/Python.framework/Versions/Current/Python "$executable" 

echo "Checking if PySide is available"

pyside_prefix=$(pkg-config --variable=prefix pyside6)
if [ $? -ne 0 ]; then
	echo "PySide is not available, ignoring."
	exit 0
fi

echo "PySide is at $pyside_prefix"

if [ ! -d "Versions/Current/lib/$python_version/site-packages/PySide6" ]
then
    cp -va "$pyside_prefix/lib/$python_version/" "Versions/Current/lib/$python_version"
    cd .. # $appbundle/Contents/Frameworks
    cp -va "$pyside_prefix/lib/"*.dylib .
else
    echo "site-packages/Pyside6 exists, skipping copying"
fi
