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

build release="Debug":
    @unset CC
    @unset CXX
    mkdir -p docs/images
    ln -sf $(pwd)/data/screenshots $(pwd)/docs/images/
    mkdir -p build
    cd build/ && CMAKE_CXX_COMPILER_LAUNCHER=ccache cmake .. -GNinja -DCMAKE_BUILD_TYPE={{release}}
    cd build/ && ninja

rmbuild:
    rm -rf ./build/

clang:
    export CC=/usr/bin/clang
    export CXX=/usr/bin/clang++
    just build

[macos]
deps:
    @otool -L build/libdatoviz.dylib | sort -r

[linux]
deps:
    @ldd build/libdatoviz.so


# -------------------------------------------------------------------------------------------------
# Python
# -------------------------------------------------------------------------------------------------

headers:
    python tools/parse_headers.py

ctypes: headers
    python tools/generate_ctypes.py

cython:
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

tree:
    tree -I external -I "build*" -I data -I bin -I libs -I tools -I "packaging*" -I docs -I cmake -I "*.py" -I "*.pxd" -I "*.pyx" -I "*.json" -I "*.out"

cloc:
    cloc . --exclude-dir=bin,build,build_clang,cmake,data,datoviz,docs,external,libs,packaging,tools


# -------------------------------------------------------------------------------------------------
# Tests
# -------------------------------------------------------------------------------------------------

[linux]
test test_name="":
    ./build/datoviz test {{test_name}}

[macos]
test test_name="":
    @VK_DRIVER_FILES="libs/vulkan/macos/MoltenVK_icd.json" ./build/datoviz test {{test_name}}

pytest:
    DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:$(pwd)/build pytest datoviz/tests/ -vv


# -------------------------------------------------------------------------------------------------
# Demo
# -------------------------------------------------------------------------------------------------

[linux]
demo:
    @LD_LIBRARY_PATH=build/ python3 -c "import ctypes; ctypes.cdll.LoadLibrary('libdatoviz.so').dvz_demo()"

[macos]
demo:
    @DYLD_LIBRARY_PATH=build/ VK_DRIVER_FILES="$(pwd)/libs/vulkan/macos/MoltenVK_icd.json" python3 -c "import ctypes; ctypes.cdll.LoadLibrary('libdatoviz.dylib').dvz_demo()"


# -------------------------------------------------------------------------------------------------
# Example
# -------------------------------------------------------------------------------------------------

[linux]
runexample name="":
    ./build/example_{{name}}

[macos]
runexample name="":
    @VK_DRIVER_FILES="libs/vulkan/macos/MoltenVK_icd.json" ./build/example_{{name}}

example name="":
    gcc -o build/example_{{name}} examples/{{name}}.c -Iinclude/ -Lbuild/ -Wl,-rpath,build -lm -ldatoviz
    just runexample {{name}}


# -------------------------------------------------------------------------------------------------
# Swiftshader
# -------------------------------------------------------------------------------------------------

[linux]
swiftshader +args:
    @VK_ICD_FILENAMES="data/swiftshader/linux/vk_swiftshader_icd.json" {{args}}

[macos]
swiftshader +args:
    @VK_ICD_FILENAMES="data/swiftshader/macos/vk_swiftshader_icd.json" {{args}}


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

    cp build/libdatoviz.so* $DEB/usr/local/lib/
    cp libs/vulkan/linux/libvulkan.so* $DEB/usr/local/lib/
    cp include/datoviz*.h $DEB/usr/local/include/

    # Build the package.
    fakeroot dpkg-deb --build $DEB

    # Move it.
    mv packaging/deb.deb packaging/datoviz_0.2.0_amd64.deb
    rm -rf $DEB

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
    docker build -t datoviz_deb_test .

    # Run the Docker container
    docker run --runtime=nvidia --gpus all -e DISPLAY=$DISPLAY -v /tmp/.X11-unix/:/tmp/.X11-unix/ --rm datoviz_deb_test

    rm Dockerfile


# -------------------------------------------------------------------------------------------------
# Python packaging
# -------------------------------------------------------------------------------------------------

wheel:
    @python setup.py bdist_wheel

testwheel:
    #!/usr/bin/env sh

    if [ ! -f dist/datoviz-*-py3-none-any.whl ]; then
        just deb
    fi

    # Create a Dockerfile for testing
    echo "FROM ubuntu:24.04

    RUN apt-get update
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
    RUN apt-get install -y python3 python3-pip python3-venv

    ENV NVIDIA_DRIVER_CAPABILITIES=all
    ENV NVIDIA_VISIBLE_DEVICES=all

    COPY dist/datoviz-*-py3-none-any.whl /tmp/
    RUN python3 -m venv /tmp/venv
    RUN /tmp/venv/bin/pip install /tmp/datoviz-*-py3-none-any.whl

    WORKDIR /root
    CMD /tmp/venv/bin/python3 -c \"import datoviz; datoviz.demo()\"

    " > Dockerfile

    # Build the Docker image
    docker build -t datoviz_wheel_test .

    # Run the Docker container
    docker run --runtime=nvidia --gpus all -e DISPLAY=$DISPLAY -v /tmp/.X11-unix/:/tmp/.X11-unix/ --rm datoviz_wheel_test

    rm Dockerfile

pkg:
    #!/usr/bin/env sh
    PKGROOT="packaging/pkgroot"
    PKG="packaging/pkg"
    INCLUDEDIR="/usr/local/include/datoviz"
    LIBDIR="/usr/local/lib/datoviz"

    # Clean up and prepare the directory structure.
    mkdir -p $PKGROOT $PKG
    rm -rf $PKGROOT/* $PKG/*
    mkdir -p $PKGROOT$INCLUDEDIR
    mkdir -p $PKGROOT$LIBDIR

    # Copy the header files.
    cp include/datoviz*.h $PKGROOT$INCLUDEDIR
    cp libs/vulkan/macos/MoltenVK_icd.json $PKGROOT$LIBDIR

    # Copy the shared libraries.
    cp -a build/libdatoviz.*dylib $PKGROOT$LIBDIR
    cp -a libs/vulkan/macos/libvulkan.*dylib $PKGROOT$LIBDIR
    cp -a libs/vulkan/macos/libMoltenVK.*dylib $PKGROOT$LIBDIR

    # Copy the dependencies and adjust their rpaths.
    # cp -a $(otool -L build/libdatoviz.dylib | grep brew | awk '{print $1}') $PKGROOT$LIBDIR
    # Path to libdatoviz.
    LIB=$PKGROOT$LIBDIR/libdatoviz.dylib
    # List all Homebrew dependencies, copy them to the package tree, and update the rpath of
    # libdatoviz to point to these local copies.
    otool -L $LIB | grep brew | awk '{print $1}' | while read -r dep; do
        filename=$(basename "$dep")
        cp -a $dep $PKGROOT$LIBDIR/
        echo $PKGROOT$LIBDIR/$filename
        install_name_tool -change "$dep" "@loader_path/$filename" $LIB
    done
    # Show the dependencies of the packaged datoviz library.
    otool -L $LIB | sort -r

    # Build the package.
    pkgbuild --root $PKGROOT --identifier com.datoviz --version {{VERSION}} --install-location / $PKG/datoviz.pkg

    # Display information about the contents of the .pkg file.
    pkgutil --expand $PKG/datoviz.pkg $PKG/extracted
    cd $PKG/extracted && cat Payload | gunzip -dc | cpio -i
    tree . -ugh && cd -

    # Move it.
    mv $PKG/datoviz.pkg packaging/datoviz_{{VERSION}}.pkg
    # rm -rf $PKGROOT $PKG

testpkg vm_ip_address:
    #!/usr/bin/env sh
    IP="{{vm_ip_address}}"
    TMPDIR=/tmp/datoviz_example

    # Check if the pkg package exists, if not, build it
    if [ ! -f packaging/datoviz_{{VERSION}}.pkg ]; then
        just pkg
    fi

    # Copy the .pkg file to the VM
    ssh -T $USER@$IP "mkdir -p $TMPDIR && rm -rf $TMPDIR/*"
    scp packaging/datoviz_{{VERSION}}.pkg \
        examples/scatter.c \
        $USER@$IP:$TMPDIR

    # Connect to the VM and install the .pkg file
    ssh -T $USER@$IP << 'EOF'
    # Install the .pkg package
    TMPDIR=/tmp/datoviz_example
    echo "$USER" | sudo -S installer -pkg $TMPDIR/datoviz_{{VERSION}}.pkg -target /
    cd $TMPDIR
    ls -la $TMPDIR
    clang -o $TMPDIR/example_scatter $TMPDIR/scatter.c \
        -I/usr/local/include/datoviz/ -L/usr/local/lib/datoviz/ \
        -Wl,-rpath,/usr/local/lib/datoviz -lm -ldatoviz

    echo "Compilation finished. The example executable is located at $TMPDIR/example_scatter"
    EOF



# -------------------------------------------------------------------------------------------------
# Cleaning
# -------------------------------------------------------------------------------------------------

clean:
    just rmbuild

rebuild:
    just rmbuild
    just build
