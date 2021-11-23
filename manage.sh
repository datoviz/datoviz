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

function test {
    ./build/datoviz test $2
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
elif [ $1 == "test" ]
then
    test
fi
