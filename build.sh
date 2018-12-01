#!/bin/sh

# This script is a work in progress

####   Constants    ####
BUILDR2=1
ERR=0

#### User variables ####
BUILD="build"
QMAKE_CONF=""
ROOT_DIR=$(pwd)
R2_PREFIX=""
#QMAKE_CONF="CUTTER_ENABLE_JUPYTER=false CUTTER_ENABLE_QTWEBENGINE=false"

build_global_r2() {
  R2_PREFIX="/usr"
  answer="Y"
  printf "A (new?) version of radare2 will be installed. Do you agree? [Y/n] "
  read -r answer
  if [ -z "$answer" ] || [ "$answer" = "Y" ] || [ "$answer" = "y" ]; then
    git submodule init && git submodule update
    cd radare2 || exit 1
    ./sys/install.sh
    cd ..
  else
    echo "Sorry but this script won't work otherwise. Read the README."
    exit 1
  fi
}

build_local_r2() {
  R2_PREFIX=${ROOT_DIR}/r2-bins
  meson --prefix="${R2_PREFIX}" radare2 r2-build
  ninja -C r2-build
  ninja -C r2-build install
}

create_tanslations() {
  lrelease ./src/Cutter.pro
}

do_build() {
  create_translations

  # Checking for radare2
  if r2 -v 1>/dev/null 2>&1; then
    R2COMMIT=$(r2 -v | tail -n1 | sed "s,commit: \\(.*\\) build.*,\\1,")
    SUBMODULE=$(git submodule | awk '{print $1}')
    if [ "$R2COMMIT" = "$SUBMODULE" ]; then
      BUILDR2=0
    fi
  fi

  # Check if either qmake or qmake-qt5 is available
  qmakepath=$(which qmake-qt5)
  if [ -z "$qmakepath" ]; then
    qmakepath=$(which qmake)
  fi
  if [ -z "$qmakepath" ]; then
    echo "You need qmake to build Cutter."
    echo "Please make sure qmake is in your PATH environment variable."
    exit 1
  fi

  # Check if meson and ninja are available
  if meson 1>/dev/null 2>&1; then
    HAS_MESON=1
    echo "Meson found."
  fi
  if ninja 1>/dev/null 2>&1; then
    HAS_NINJA=1
    echo "Ninja found."
  fi

  # Build radare2
  if [ ${BUILDR2} -eq 1 ]; then
    if [ ${HAS_MESON} -eq 1 ] && [ ${HAS_NINJA} -eq 1 ]; then
      build_local_r2
    else
      build_global_r2
    fi
    export PKG_CONFIG_PATH="${R2_PREFIX}/lib/pkgconfig"
    exit 0
  else
    echo "Correct radare2 version found, skipping..."
  fi

  # Build Cutter
  mkdir -p "${BUILD}"
  cd "${BUILD}" || exit 1
  "$qmakepath" ../src/Cutter.pro ${QMAKE_CONF}
  make -j4
  ERR=$((ERR+$?))

  # Move translations
  mkdir -p "${ROOT_DIR}/translations"
  find "${ROOT_DIR}/src/translations" -maxdepth 1  -type f | grep "cutter_..\.qm" | while read -r SRC_FILE; do
  mv "${SRC_FILE}" "${ROOT_DIR}/translations"
done

cd ..

# Done
if [ ${ERR} -gt 0 ]; then
  echo "Something went wrong!"
else
  echo "Build complete. Binary available at: ${BUILD}/Cutter"
fi
}

do_build
