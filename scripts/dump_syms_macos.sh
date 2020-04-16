#!/bin/bash

if [ ! $# -eq 2 ]; then
	echo "usage: $0 [Cutter.app] [dst]"
	exit 1
fi

appbundle="$1"
dst="$2"
files="$appbundle/Contents/MacOS/Cutter.bin $appbundle/Contents/Frameworks/Python.framework/Python $(find "$appbundle" -type f -name '*.dylib')"

mkdir -p "$dst" || exit 1 

for file in $files; do
	echo "Dumping syms from $file"
	dump_syms "$file" > "$dst/tmp.sym" || echo "Failed to dump syms from $file"
	infoline=($(head -n1 "$dst/tmp.sym"))
	id=${infoline[3]}
	name=${infoline[4]}
	dir="$dst/$name/$id"
	mkdir -p "$dir"
	mv "$dst/tmp.sym" "$dir/$name.sym"
done

echo "Done"

