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

if [ -n "$1" ]; then
    echo ""
    echo "================> run one unittest: $1"
    ${pwd}/build/$1
    if [ $? -ne 0 ]; then
        exit 1
    fi
else
    echo ""
    echo "================> run all unittest..."
    cd ${pwd}
    for cpp in `ls *test.cpp`;
    do
        file_name=$(basename "$cpp" .cpp)
        echo "run unittest: ${file_name}"
        ${pwd}/build/${file_name}
        if [ $? -ne 0 ]; then
            exit 1
        fi
    done
fi

echo ""
echo "================> get coverage all unittest..."
cd ${pwd}
sh get_coverage.sh

echo ""
echo "run all unittest success!!!"

