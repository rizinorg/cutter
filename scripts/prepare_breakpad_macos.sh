#!/bin/bash

DIR="`pwd`/src"
cd $DIR
BREAKPAD_FRAMEWORK_DIR="$DIR/breakpad/framework"
BREAKPAD_DUMP_SYMS_DIR="$DIR/breakpad/bin"
git clone https://github.com/google/breakpad.git
mkdir $BREAKPAD_FRAMEWORK_DIR
mkdir $BREAKPAD_DUMP_SYMS_DIR
cd breakpad/src/client/mac/ && xcodebuild -sdk macosx
cp -R build/Release/Breakpad.framework "$BREAKPAD_FRAMEWORK_DIR"

cd $DIR/breakpad
cp -R src/. framework/Breakpad.framework/Headers

export BREAKPAD_FRAMEWORK_DIR=$BREAKPAD_FRAMEWORK_DIR
cd $DIR
cd ..
