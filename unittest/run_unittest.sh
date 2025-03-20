#!/bin/bash

pwd=`pwd`

echo ""
echo "================> compile all unittest..."
rm -rf ${pwd}/build
mkdir ${pwd}/build

cd ${pwd}/build && cmake -DCMAKE_BUILD_TYPE=Debug ..
make clean && make -j6
if [ $? -ne 0 ]; then
    exit 1
fi

echo ""
echo "================> run all unittest..."
cd ${pwd}
for cpp in `ls *test.cpp`;
do
    file_name=$(basename "$cpp" .cpp)
    echo "run unittest: ${file_name}"
    ${pwd}/build/${file_name}
done

echo ""
echo "================> get coverage all unittest..."
cd ${pwd}
sh get_coverage.sh

echo ""
echo "run all unittest success!!!"

