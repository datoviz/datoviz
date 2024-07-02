# -------------------------------------------------------------------------------------------------
# Constants
# TODO: move these elsewhere?
# -------------------------------------------------------------------------------------------------

VERSION := "0.2.0"
MAINTAINER := "Cyrille Rossant <cyrille.rossant@gmail.com>"
DESCRIPTION := "A C library for high-performance GPU scientific visualization"


# -------------------------------------------------------------------------------------------------
# Management script
# -------------------------------------------------------------------------------------------------

default:
    @echo "No arguments supplied"
    @exit 1


# -------------------------------------------------------------------------------------------------
# Building
# -------------------------------------------------------------------------------------------------

build:
    @unset CC
    @unset CXX
    mkdir -p docs/images
    ln -sf $(pwd)/data/screenshots $(pwd)/docs/images/
    mkdir -p build
    cd build/ && CMAKE_CXX_COMPILER_LAUNCHER=ccache cmake .. -GNinja
    cd build/ && ninja

rmbuild:
    rm -rf ./build/

clang:
    export CC=/usr/bin/clang
    export CXX=/usr/bin/clang++
    just build


# -------------------------------------------------------------------------------------------------
# Cython
# -------------------------------------------------------------------------------------------------

parse_headers:
    python tools/parse_headers.py

build_cython:
    rm -rf datoviz/*.c datoviz/*.so datoviz/__pycache__ &&
    python tools/generate_cython.py &&
    python setup.py build_ext -i &&
    python setup.py develop --user


# -------------------------------------------------------------------------------------------------
# Code quality
# -------------------------------------------------------------------------------------------------

format:
    find tests/ src/ include/ -iname *.h -o -iname *.c | xargs clang-format -i

valgrind args="":
    # NOTE: need to remove -pg compiler option before running valgrind
    valgrind \
        --leak-check=full \
        --show-leak-kinds=all \
        --keep-debuginfo=yes \
        --track-origins=yes \
        --verbose \
        --suppressions=.valgrind.exceptions.txt \
        --log-file=.valgrind.out.txt \
        {{args}}

cppcheck:
    cppcheck --enable=all --inconclusive src/ include/ cli/ tests/ -i external -I include/datoviz
    # 2> .cppcheck.out.txt && \
    # echo ".cppcheck.out.txt saved"

prof:
    gprof build/datoviz gmon.out
exports:
    nm -D --defined-only build/libdatoviz.so


# -------------------------------------------------------------------------------------------------
# Tests
# -------------------------------------------------------------------------------------------------

test test_name="":
    ./build/datoviz test {{test_name}}

pytest:
    DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:$(pwd)/build pytest datoviz/tests/ -vv


# -------------------------------------------------------------------------------------------------
# Example
# -------------------------------------------------------------------------------------------------

example name="":
    gcc -o build/example_{{name}} examples/{{name}}.c -Iinclude/ -Lbuild/ -Wl,-rpath,build -lm -ldatoviz
    ./build/example_scatter


# -------------------------------------------------------------------------------------------------
# Packaging
# -------------------------------------------------------------------------------------------------

deb:
    #!/usr/bin/env sh
    DEB="packaging/deb/"

    # Clean up and prepare the directory structure.
    rm -rf $DEB
    mkdir -p $DEB/DEBIAN
    mkdir -p $DEB/usr/local/include
    mkdir -p $DEB/usr/local/lib

    # Create the control file.
    echo "Package: datoviz
    Version: {{VERSION}}
    Section: libs
    Priority: optional
    Architecture: amd64
    Maintainer: {{MAINTAINER}}
    Description: {{DESCRIPTION}}" > $DEB/DEBIAN/control

    # Copy libdatoviz
    cp build/libdatoviz.so* $DEB/usr/local/lib/
    # Copy libvulkan
    cp libs/vulkan/libvulkan.so* $DEB/usr/local/lib/
    # Copy the datoviz header files
    cp include/datoviz*.h $DEB/usr/local/include/

    # Build the package.
    fakeroot dpkg-deb --build $DEB

    # Move it.
    mv packaging/deb.deb packaging/datoviz_0.2.0_amd64.deb

testdeb:
    #!/usr/bin/env sh

    # Check if the deb package exists, if not, build it
    if [ ! -f packaging/datoviz_*_amd64.deb ]; then
        just deb
    fi

    # Create a Dockerfile for testing
    echo "FROM ubuntu:24.04

    RUN apt-get update && apt-get install -y build-essential
    RUN apt-get install -y \
        libx11-dev \
        libxrandr-dev \
        libxinerama-dev \
        libxcursor-dev \
        libxi-dev \
        vulkan-tools \
        mesa-utils \
        nvidia-driver-460 \
        nvidia-utils-460 \
        x11-apps

    ENV NVIDIA_DRIVER_CAPABILITIES=all
    ENV NVIDIA_VISIBLE_DEVICES=all

    COPY packaging/datoviz_*_amd64.deb /tmp/
    RUN dpkg -i /tmp/datoviz_*_amd64.deb || apt-get install -f -y

    COPY examples/scatter.c /root/
    WORKDIR /root

    RUN gcc -o example_scatter scatter.c -I/usr/local/include/ -L/usr/local/lib/ -Wl,-rpath,/usr/local/lib -lm -ldatoviz
    CMD ./example_scatter

    " > Dockerfile

    # Build the Docker image
    docker build -t datoviz_test .

    # Run the Docker container
    docker run --runtime=nvidia --gpus all -e DISPLAY=$DISPLAY -v /tmp/.X11-unix/:/tmp/.X11-unix/ --rm datoviz_test

    rm Dockerfile


# -------------------------------------------------------------------------------------------------
# Entry-point
# -------------------------------------------------------------------------------------------------

clean:
    just rmbuild

rebuild:
    just rmbuild
    just build
