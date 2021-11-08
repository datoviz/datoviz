#!/bin/bash
if [ $# -eq 0 ]
  then
    echo "No arguments supplied"
    exit
fi

# -------------------------------------------------------------------------------------------------
# Building
# -------------------------------------------------------------------------------------------------

build () {
    # Make sure macOS uses clang and not gcc
    if [[ "$OSTYPE" == "darwin"* ]]; then
        export CC=/usr/bin/clang
        export CXX=/usr/bin/clang++
    fi
    ln -sf $(pwd)/data/screenshots $(pwd)/docs/images/ &&
    mkdir -p build &&
    cd build && \
    cmake .. -GNinja && \
    ninja && \
    cd ..
}



# -------------------------------------------------------------------------------------------------
# Entry-point
# -------------------------------------------------------------------------------------------------

if [ $1 == "build" ]
then
    build
fi
