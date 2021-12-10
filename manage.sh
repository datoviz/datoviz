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
# Entry-point
# -------------------------------------------------------------------------------------------------

if [ $1 == "build" ]
then
    build
elif [ $1 == "rebuild" ]
then
    rmbuild
    build
elif [ $1 == "cython" ]
then
    build_cython
elif [ $1 == "test" ]
then
    test $2
elif [ $1 == "pytest" ]
then
    DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:$(pwd)/build pytest datoviz/tests/ -vv
fi
