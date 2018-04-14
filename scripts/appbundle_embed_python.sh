#!/bin/bash

if ! [[ $# -eq 3 ]]; then
    echo "Usage: $0 [Python.framework] [AppBundle.app] [AppBundle.app/Contents/MacOS/Executable]"
    exit 1
fi

py_framework=$1
appbundle=$2
executable=$3

echo "Embedding $py_framework into $appbundle"

mkdir -p "$appbundle/Contents/Frameworks" || exit 1
cp -a "$py_framework" "$appbundle/Contents/Frameworks/" || exit 1

echo "Making executable $executable point to embedded Framework"
install_name_tool -change `otool -L "$executable" | sed -n "s/^[[:blank:]]*\([^[:blank:]]*Python\) (.*$/\1/p"` @executable_path/../Frameworks/Python.framework/Versions/Current/Python "$executable" 

echo "Cleaning up embedded Python Framework"
cd "$appbundle/Contents/Frameworks/Python.framework" || exit 1
find . | grep -E "(__pycache__|\.pyc|\.pyo$)" | xargs rm -rf || exit 1
rm -r Versions/Current/Resources/* Versions/Current/lib/python3.6/test Versions/Current/lib/python3.6/idlelib Versions/Current/lib/python3.6/curses Versions/Current/lib/python3.6/lib2to3 || exit 1

