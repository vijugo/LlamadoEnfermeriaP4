#!/bin/sh
#Build natively on Linux with gcc

rm  -r  build
mkdir  -p  build
cd build

cmake  ..  --no-warn-unused-cli  -DSQUARELINE_BUILD_HOST__UNIX=1  -DSQUARELINE_BUILD_TARGET__UNIX=1  -DCMAKE_BUILD_TYPE=Debug #Release
make  -j2

cd ..

./run.sh

