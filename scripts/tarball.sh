#!/bin/bash

NAME=${1:-Cutter}

set -xe
cd $(dirname "${BASH_SOURCE[0]}")/..

shopt -s extglob
shopt -s dotglob
mkdir "${NAME}"
cp -r !(${NAME}) "${NAME}"

pushd "${NAME}"
git clean -dxff .
git submodule update --init --recursive

pushd rizin
git clean -dxff .
# Possible option: pre-download all subproject, however this makes the tarball huge.
# As opposed to meson dist used for rizin tarballs, this will not just download the ones
# used in a default build, but all of them, including multiple capstone variants.
# meson subprojects download
popd

pushd src/translations
git clean -dxff .
popd

find . -name ".git*" | xargs rm -rfv
popd

tar -czvf "${NAME}-src.tar.gz" "${NAME}"
