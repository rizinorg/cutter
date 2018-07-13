#!/bin/bash

mkdir -p python && cd python || exit 1

export CUSTOM_PYTHON_PREFIX="`pwd`/prefix"

wget "https://www.python.org/ftp/python/3.6.4/Python-3.6.4.tar.xz" || exit 1
tar -xf Python-3.6.4.tar.xz || exit 1

cd Python-3.6.4 || exit 1
echo "Building Python to install to prefix $CUSTOM_PYTHON_PREFIX"

./configure --enable-shared --prefix=$CUSTOM_PYTHON_PREFIX || exit 1
make -j4 || exit 1
make install > /dev/null || exit 1

cd ..

echo "Patching libs in $CUSTOM_PYTHON_PREFIX/lib/python3.6/lib-dynload to have the correct rpath"
wget https://nixos.org/releases/patchelf/patchelf-0.9/patchelf-0.9.tar.bz2 || exit 1
tar -xf patchelf-0.9.tar.bz2 || exit 1
cd patchelf-0.9 || exit 1
./configure || exit 1
make || exit 1
cd ..

for lib in "$CUSTOM_PYTHON_PREFIX/lib/python3.6/lib-dynload"/*.so; do
	echo "  patching $lib"
    patchelf-0.9/src/patchelf --set-rpath '$ORIGIN/../..' "$lib" || exit 1
done

PYTHONHOME=$CUSTOM_PYTHON_PREFIX \
       LD_LIBRARY_PATH=$CUSTOM_PYTHON_PREFIX/lib \
       "$CUSTOM_PYTHON_PREFIX/bin/pip3" install jupyter || exit 1

cd ..
