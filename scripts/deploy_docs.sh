#!/bin/bash

cd $(dirname "${BASH_SOURCE[0]}")/..

git clone git@github.com:radareorg/cutter.re.git || exit 1
rm -rf cutter.re/docs
cp -a docs/build/html cutter.re/docs || exit 1

cd cutter.re || exit 1
git add . || exit 1
git push origin master || exit 1

