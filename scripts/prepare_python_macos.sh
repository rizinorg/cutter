#!/bin/bash

mkdir python && cd python

wget "https://www.python.org/ftp/python/3.6.4/Python-3.6.4.tar.xz" || exit 1
tar -xf Python-3.6.4.tar.xz || exit 1

export PYTHON_FRAMEWORK_DIR="`pwd`/framework"

cd Python-3.6.4 || exit 1

CPPFLAGS="-I$(brew --prefix openssl)/include" LDFLAGS="-L$(brew --prefix openssl)/lib" ./configure --enable-framework=$PYTHON_FRAMEWORK_DIR || exit 1

# Patch for https://github.com/radareorg/cutter/issues/424
sed -i ".original" "s/#define HAVE_GETENTROPY 1/#define HAVE_GETENTROPY 0/" pyconfig.h

make -j4 || exit 1
make frameworkinstallframework > /dev/null || exit 1

PYTHONHOME=$PYTHON_FRAMEWORK_DIR/Python.framework/Versions/Current \
       $PYTHON_FRAMEWORK_DIR/Python.framework/Versions/Current/bin/pip3 install jupyter || exit 1

cd ../..
