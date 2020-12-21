#!/bin/sh

set -euo pipefail

SCRIPTPATH=$(realpath "$(dirname "${BASH_SOURCE[0]}")")

DIR="$SCRIPTPATH/.."
cd "$DIR"
BREAKPAD_FRAMEWORK_DIR="$DIR/breakpad/framework"
BREAKPAD_DUMP_SYMS_DIR="$DIR/breakpad/bin"
git clone https://github.com/google/breakpad.git
mkdir $BREAKPAD_FRAMEWORK_DIR
mkdir $BREAKPAD_DUMP_SYMS_DIR
cd breakpad
git checkout 4d550cceca107f36c4bc1ea1126b7d32cc50f424
git apply "$SCRIPTPATH/breakpad_macos.patch"
cd src/client/mac/ && xcodebuild -sdk macosx
cp -R build/Release/Breakpad.framework "$BREAKPAD_FRAMEWORK_DIR"

cd $DIR/breakpad
cp -R src/. framework/Breakpad.framework/Headers

export BREAKPAD_FRAMEWORK_DIR=$BREAKPAD_FRAMEWORK_DIR
cd $DIR
