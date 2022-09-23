#!/bin/bash

# -------------------------------------------------------------------------------------------------
# Management script
# -------------------------------------------------------------------------------------------------

if [ $# -eq 0 ]
  then
    echo "No arguments supplied"
    exit
fi


# -------------------------------------------------------------------------------------------------
# Building
# -------------------------------------------------------------------------------------------------

function build {
    mkdir -p docs/images &&
    ln -sf $(pwd)/data/screenshots $(pwd)/docs/images/ &&
    mkdir -p build &&
    cd build && \
    cmake .. -GNinja && \
    ninja && \
    cd ..
}

function rmbuild {
    rm -rf build
}


# -------------------------------------------------------------------------------------------------
# Cython
# -------------------------------------------------------------------------------------------------

function parse_headers {
    python tools/parse_headers.py
}

function build_cython {
    rm -rf datoviz/*.c datoviz/*.so datoviz/__pycache__ && \
    python tools/generate_cython.py && \
    python setup.py build_ext -i && \
    python setup.py develop --user
}



# -------------------------------------------------------------------------------------------------
# Tests
# -------------------------------------------------------------------------------------------------

function test {
    ./build/datoviz test $1
}



# -------------------------------------------------------------------------------------------------
# Code quality
# -------------------------------------------------------------------------------------------------

if [ $1 == "format" ]
then
    find examples/ tests/ src/ include/ -iname *.h -o -iname *.c | xargs clang-format -i
fi

if [ $1 == "valgrind" ]
then
    valgrind \
        --leak-check=full \
        --show-leak-kinds=all \
        --keep-debuginfo=yes \
        --track-origins=yes \
        --verbose \
        --suppressions=.valgrind.exceptions.txt \
        --log-file=.valgrind.out.txt \
        ${@:2}
fi

if [ $1 == "cppcheck" ]
then
    cppcheck --enable=all --inconclusive src/ include/ cli/ tests/ -i external -I include/datoviz
    # 2> .cppcheck.out.txt && \
    # echo ".cppcheck.out.txt saved"
fi

if [ $1 == "prof" ]
then
    gprof build/datoviz gmon.out
fi




# -------------------------------------------------------------------------------------------------
# Entry-point
# -------------------------------------------------------------------------------------------------

if [ $1 == "build" ]
then
    build
elif [ $1 == "clean" ]
then
    rmbuild
elif [ $1 == "clang" ]
then
    CC=/usr/bin/clang CXX=/usr/bin/clang++ build
elif [ $1 == "rebuild" ]
then
    rmbuild
    build
elif [ $1 == "cython" ]
then
    build_cython
elif [ $1 == "parseheaders" ]
then
    parse_headers
elif [ $1 == "test" ]
then
    test $2
elif [ $1 == "pytest" ]
then
    DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:$(pwd)/build pytest datoviz/tests/ -vv
fi
