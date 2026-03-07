#!/bin/sh
# Cross-build in Linux for Windows

rm  -r  build
mkdir  -p  build
cd build

cmake  ..  --no-warn-unused-cli  -DSQUARELINE_BUILD_HOST__UNIX=1  -DSQUARELINE_BUILD_TARGET__WINDOWS=1  -DCMAKE_BUILD_TYPE=Debug #Release
make  -j2

cd ..

cp  SDL2.dll  build/

./run-wincross.sh

