#!/bin/bash

mkdir python && cd python

wget "https://www.python.org/ftp/python/3.6.4/Python-3.6.4.tar.xz" || exit 1
tar -xf Python-3.6.4.tar.xz || exit 1

export CUSTOM_PYTHON_PREFIX="`pwd`/prefix"

cd Python-3.6.4 || exit 1
echo "Building Python to install to prefix $CUSTOM_PYTHON_PREFIX"

./configure --enable-shared --prefix=$CUSTOM_PYTHON_PREFIX || exit 1
make -j4 || exit 1
make install > /dev/null || exit 1

PYTHONHOME=$CUSTOM_PYTHON_PREFIX \
       LD_LIBRARY_PATH=$CUSTOM_PYTHON_PREFIX/lib \
       "$CUSTOM_PYTHON_PREFIX/bin/pip3" install jupyter || exit 1

cd ../..
