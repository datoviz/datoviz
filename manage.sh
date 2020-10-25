#!/bin/bash

if [ $# -eq 0 ]
  then
    echo "No arguments supplied"
    exit
fi

if [ $1 == "build" ]
then
    if [ ! -L "data" ]; then
        ln -s $(pwd)/../visky-data/data data
    fi
    mkdir -p build &&
    python3 bindings/cython/utils/gencython.py && \
    cd build && \
    cmake .. -GNinja && \
    VKY_EXAMPLE= ninja && \
    cd .. #&& \
    # if [ ! -L "$(pwd)/bindings/cython/visky/pyvisky.so" ]; then
    #     ln -s $(pwd)/build/libpyvisky.so $(pwd)/bindings/cython/visky/pyvisky.so
    # fi
fi

if [ $1 == "clang" ]
then
    mkdir -p build_clang &&
    cd build_clang && \
    CC=/usr/bin/clang CXX=/usr/bin/clang++ cmake .. -GNinja && \
    ninja && \
    cd ..
fi

if [ $1 == "download" ]
then
    wget https://github.com/viskydev/visky-data/archive/master.zip -o data.zip && unzip data.zip && rm data.zip
fi

if [ $1 == "fixtest" ]
then
    mv test/screenshots/$2_fail.ppm test/screenshots/$2.ppm
fi

if [ $1 == "format" ]
then
    find examples/ test/ src/ include/ -iname *.h -o -iname *.c | xargs clang-format -i
fi

if [ $1 == "memcheck" ]
then
    valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         --suppressions=.valgrind.exceptions.txt \
         --log-file=.valgrind.out.txt \
         ./build/$2
fi

if [ $1 == "test" ]
then
    if [ -z "$2" ]
    then
        mkdir -p build &&
        cd build && \
        cmake .. && \
        cmake --build . && \
        CTEST_OUTPUT_ON_FAILURE=1 ctest && \
        cd ..
    else
        # bash manage.sh build
        VKY_DEBUG_TEST= build/visky_test $2
    fi
fi

if [ $1 == "prof" ]
then
    valgrind --tool=callgrind --dump-instr=yes --simulate-cache=yes --collect-jumps=yes $2
fi

if [ $1 == "run" ]
then
    VKY_EXAMPLE=$2 ./manage.sh build && ./build/$2
fi
