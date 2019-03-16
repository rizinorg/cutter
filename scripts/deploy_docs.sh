#!/bin/bash

cd $(dirname "${BASH_SOURCE[0]}")/..

echo "Cloning current cutter.re"

git clone git@github.com:radareorg/cutter.re.git || exit 1

echo "Updating docs"

rm -rf cutter.re/docs
cp -a docs/build/html cutter.re/docs || exit 1

echo "Committing new changes"

cd cutter.re || exit 1
git add . || exit 1
git diff --cached --quiet && echo "No changes." && exit 0
git commit -m "Update docs from radareorg/cutter"
git push origin master || exit 1

