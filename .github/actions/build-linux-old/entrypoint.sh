#!/bin/bash

set -euo pipefail

pwd
ls

#export TZ=UTC
#ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

apt-get -y update

# latest git and cmake
export GIT_VERSION="git-2.36.1"
export CMAKE_VERSION="3.25.3"

apt-get -y install wget libcurl4-gnutls-dev libexpat1-dev gettext libz-dev libssl-dev build-essential

wget "https://www.kernel.org/pub/software/scm/git/$GIT_VERSION.tar.gz"
tar -zxf "$GIT_VERSION.tar.gz"
# build.
#make -C "$GIT_VERSION" prefix=/usr install -j > "$GIT_VERSION/build.log"
# ensure git is installed.
#git version
wget "https://github.com/Kitware/CMake/releases/download/v$CMAKE_VERSION/cmake-$CMAKE_VERSION-linux-x86_64.sh"
bash ./cmake-$CMAKE_VERSION-linux-x86_64.sh --skip-license --prefix=/usr
# ensure cmake is installed.
cmake --version
# cleanup dev environment.
rm -rf "$GIT_VERSION.tar.gz" "$GIT_VERSION" cmake-$CMAKE_VERSION-linux-x86_64.sh
unset CMAKE_VERSION
unset GIT_VERSION

apt-get -y install libgraphviz-dev \
    mesa-common-dev \
    libxkbcommon-x11-dev \
    ninja-build \
    python3-pip \
    curl \
    libpcre2-dev \
    libfuse2 \
    pkg-config \
    git


if [ "$image" = "ubuntu:18.04" ]; then
    # install additional packages needed for appimage
    apt-get -y install gcc-7 \
                        libglu1-mesa-dev \
                        freeglut3-dev \
                        mesa-common-dev \
                        libclang-8-dev \
                        llvm-8
    ln -s /usr/bin/llvm-config-8 /usr/bin/llvm-config
fi
if [ "$image" = "ubuntu:18.04" ] || [ "$image" = "ubuntu:20.04" ]; then
    # install additional packages needed for appimage
    apt-get -y install libxcb1-dev \
                        libxkbcommon-dev \
                        libxcb-*-dev \
                        libegl1
fi
if [ "$image" = "ubuntu:20.04" ] && [ "$system_deps" = "false" ]; then
    # install additional packages needed for appimage
    apt-get -y install libclang-12-dev \
                        llvm-12 \
                        libsm6 \
                        libwayland-dev  \
                        libgl1-mesa-dev
fi
if [ "$image" = "ubuntu:18.04" ] && [ "$system_deps" = "true" ]; then
    apt-get -y install qt5-default \
                        libqt5svg5-dev \
                        qttools5-dev \
                        qttools5-dev-tools
fi
if [ "$image" = "ubuntu:22.04" ]; then
    apt-get -y install libclang-12-dev \
                        llvm-12 \
                        qt6-base-dev \
                        qt6-tools-dev \
                        qt6-tools-dev-tools \
                        libqt6svg6-dev \
                        libqt6core5compat6-dev \
                        libqt6svgwidgets6 \
                        qt6-l10n-tools \
                        gcc-12 \
                        g++-12
fi

# https://github.com/rizinorg/cutter/runs/7170222817?check_suite_focus=true
python3 -m pip install meson==0.61.5


if [ "$system_deps" = "false" ]
then
    CUTTER_QT="$qt_major" scripts/fetch_deps.sh
    set +u # TODO: remove temp code after updating cutter_deps
    . cutter-deps/env.sh
    set -u
    #export LD_LIBRARY_PATH="`llvm-config --libdir`:$LD_LIBRARY_PATH"
fi
#if [ "${{ matrix.cc-override }}" != "default" ]
#then
#    export CC="${{matrix.cc-override}}"
#    export CXX="${{matrix.cxx-override}}"
#fi

# otherwise git complains about dubious ownership, due to code being checked out outside the container with a different user
git config --global --add safe.directory /github/workspace/rizin

if [ $qt_major = 6] 
then
    CMAKE_QT_ARG='ON'
else
    CMAKE_QT_ARG='OFF'
fi

mkdir build
cd build
if [ "$system_deps" = "false" ]
then
    locale
    locale -a
    export LANG="C.UTF-8"
    export LC_ALL="C.UTF-8"
    cmake \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCUTTER_ENABLE_PYTHON=ON \
    -DPython3_ROOT_DIR="$CUTTER_DEPS_PYTHON_PREFIX" \
    -DCUTTER_ENABLE_PYTHON_BINDINGS=ON \
    -DCUTTER_ENABLE_GRAPHVIZ=ON \
    -DCUTTER_USE_BUNDLED_RIZIN=ON \
    -DCUTTER_APPIMAGE_BUILD=ON \
    -DCUTTER_ENABLE_PACKAGING=ON \
    -DCUTTER_ENABLE_KSYNTAXHIGHLIGHTING=OFF \
    -DCUTTER_ENABLE_SIGDB=ON \
    -DCUTTER_ENABLE_DEPENDENCY_DOWNLOADS=ON \
    -DCUTTER_PACKAGE_RZ_GHIDRA=ON \
    -DCUTTER_PACKAGE_JSDEC=ON \
    -DCUTTER_PACKAGE_RZ_LIBSWIFT=ON \
    -DCUTTER_PACKAGE_RZ_LIBYARA=ON \
    -DCUTTER_PACKAGE_RZ_SILHOUETTE=ON \
    -DCMAKE_INSTALL_PREFIX=appdir/usr \
    -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=ON \
    -DCUTTER_QT6=$CMAKE_QT_ARG \
    ..
else
    cmake \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCUTTER_USE_BUNDLED_RIZIN=ON \
    -DCUTTER_QT6=$CMAKE_QT_ARG \
    ..
fi
ninja
if [ "$package" = "true" ]
then
    export CUTTER_VERSION=$(python ../scripts/get_version.py)
    export VERSION=$CUTTER_VERSION
    ninja install
    if [ $qt_major == "6" ]
    then
        pyside_ver=6
    else
        pyside_ver=2
    fi
    "../scripts/appimage_embed_python.sh" appdir $pyside_ver
    APP_PREFIX=`pwd`/appdir/usr
    export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}:$APP_PREFIX/lib/rizin/plugins"
    export PATH=$PATH:${APP_PREFIX}/bin
    wget -c "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
    chmod a+x linuxdeployqt*.AppImage
    rm -fv "../cutter-deps/qt/plugins/imageformats/libqjp2.so"
    if [ "$qt_major" == "5" ]; then
    export APPIMAGE_FILE="Cutter-${PACKAGE_ID}-Linux-Qt5-x86_64.AppImage"
    ./linuxdeployqt*.AppImage --appimage-extract-and-run \
        ./appdir/usr/share/applications/*.desktop \
        -executable=./appdir/usr/bin/python3 \
        -appimage \
        -no-strip -exclude-libs=libnss3.so,libnssutil3.so,libqjp2.so \
        -ignore-glob=usr/lib/python3.12/**/* \
        -verbose=2
    else
    export APPIMAGE_FILE="Cutter-${PACKAGE_ID}-Linux-x86_64.AppImage"
    ./linuxdeployqt*.AppImage --appimage-extract-and-run \
        ./appdir/usr/share/applications/*.desktop \
        -executable=./appdir/usr/bin/python3 \
        -appimage \
        -no-strip -exclude-libs=libnss3.so,libnssutil3.so,libqjp2.so \
        -exclude-libs="libwayland-client.so,libwayland-egl.so,libwayland-cursor.so" \
        -ignore-glob=usr/lib/python3.12/**/* \
        -extra-plugins="platforms/libqwayland-egl.so,platforms/libqwayland-generic.so,wayland-decoration-client,wayland-graphics-integration-client,wayland-shell-integration,wayland-graphics-integration-server" \
        -verbose=2
    fi
    find ./appdir -executable -type f -exec ldd {} \; | cut -d " " -f 2-3 | sort | uniq
    # find ./appdir -executable -type f -exec ldd {} \; | grep " => /usr" | cut -d " " -f 2-3 | sort | uniq
    
    mv Cutter-*-x86_64.AppImage "$APPIMAGE_FILE"
    echo PACKAGE_NAME=$APPIMAGE_FILE >> $GITHUB_ENV
    echo PACKAGE_NAME=$APPIMAGE_FILE >> $GITHUB_OUTPUT
    echo PACKAGE_PATH=build/$APPIMAGE_FILE >> $GITHUB_ENV
    echo PACKAGE_PATH=build/$APPIMAGE_FILE >> $GITHUB_OUTPUT
    echo UPLOAD_ASSET_TYPE=application/x-executable >> $GITHUB_ENV
fi