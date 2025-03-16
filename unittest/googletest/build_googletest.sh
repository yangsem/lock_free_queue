#!/bin/bash

pwd=`pwd`
gtest_lib=${pwd}/lib

rm -rf ${gtest_lib}
rm -rf ${pwd}/googletest-1.12.x
unzip googletest-1.12.x.zip
cd googletest-1.12.x
mkdir -p build
cd build
cmake -DCMAKE_CXX_STANDARD=11 ..
make -j6

mkdir -p ${gtest_lib}
cp -r lib/* ${gtest_lib}

cd ${pwd}
rm -rf googletest-1.12.x

echo "googletest build and library copy completed."
