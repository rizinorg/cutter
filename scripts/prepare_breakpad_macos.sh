#!/bin/bash

DIR="`pwd`"
BREAKPAD_FRAMEWORK_DIR="$DIR/breakpad/framework"
git clone https://github.com/google/breakpad.git
mkdir $BREAKPAD_FRAMEWORK_DIR
cd breakpad/src/client/mac/ && xcodebuild -sdk macosx
cp -R build/Release/Breakpad.framework "$BREAKPAD_FRAMEWORK_DIR"

cd $DIR/breakpad
cp -R src/. framework/Breakpad.framework/Headers

export BREAKPAD_FRAMEWORK_DIR=$BREAKPAD_FRAMEWORK_DIR
cd $DIR
