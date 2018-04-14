#!/bin/bash

if ! [[ $# -eq 2 ]]; then
    echo "Usage: $0 [Python prefix] [appdir]"
    exit 1
fi

python_prefix=$1
appdir=$2

echo "Embedding Python from prefix $python_prefix in appdir $appdir"

cp -RT "$python_prefix" "$appdir/usr/" || exit 1
cd "$appdir/usr/" || exit 1

echo "Cleaning up embedded Python"
find . | grep -E "(__pycache__|\.pyc|\.pyo$)" | xargs rm -rf
rm -r lib/python3.6/test lib/python3.6/idlelib lib/python3.6/curses lib/python3.6/lib2to3