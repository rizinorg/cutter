#!/bin/sh

SCRIPTPATH=$(realpath "$(dirname "${BASH_SOURCE[0]}")")

DIR="$SCRIPTPATH/.."
cd "$DIR" || exit 1
BREAKPAD_FRAMEWORK_DIR="$DIR/breakpad/framework"
BREAKPAD_DUMP_SYMS_DIR="$DIR/breakpad/bin"
git clone https://github.com/google/breakpad.git || exit 1
mkdir $BREAKPAD_FRAMEWORK_DIR
mkdir $BREAKPAD_DUMP_SYMS_DIR
cd breakpad || exit 1
git checkout 4d550cceca107f36c4bc1ea1126b7d32cc50f424 || exit 1
git apply "$SCRIPTPATH/breakpad_macos.patch" || exit 1
cd src/client/mac/ && xcodebuild -sdk macosx || exit 1
cp -R build/Release/Breakpad.framework "$BREAKPAD_FRAMEWORK_DIR" || exit 1

cd $DIR/breakpad || exit 1
cp -R src/. framework/Breakpad.framework/Headers || exit 1

export BREAKPAD_FRAMEWORK_DIR=$BREAKPAD_FRAMEWORK_DIR
cd $DIR
