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
#


# -------------------------------------------------------------------------------------------------
# Building
# -------------------------------------------------------------------------------------------------

checkstructs:
    @python -c "from datoviz import _check_struct_sizes; _check_struct_sizes('build/struct_sizes.json');"
#


build release="Debug":
    @set -e
    @unset CC
    @unset CXX
    @mkdir -p docs/images
    #@ln -sf $(pwd)/data/screenshots $(pwd)/docs/images/
    @mkdir -p build
    @cd build/ && CMAKE_CXX_COMPILER_LAUNCHER=ccache cmake .. -GNinja -DCMAKE_BUILD_TYPE={{release}}
    @cd build/ && ninja
#

rmbuild:
    rm -rf ./build/
#

clang:
    export CC=/usr/bin/clang
    export CXX=/usr/bin/clang++
    just build
#

buildmany release="Debug":
    #!/usr/bin/env sh
    DOCKER_IMAGE="quay.io/pypa/manylinux_2_28_x86_64"
    BUILD_DIR="build_many"
    CONTAINER_NAME="datoviz-buildmany"

    # Create the build directory
    mkdir -p $BUILD_DIR $BUILD_DIR/$BUILD_DIR
    # HACK: do NOT use the shipped Ubuntu libraries in the RedHat-based Docker container
    rsync -a -v --exclude "libvulkan*" --exclude "glslc" --exclude "justfile" \
        bin cli cmake data external include libs src tests tools \
        CMakeLists.txt *.map "$BUILD_DIR/"

    # Create a temporary Dockerfile
    cat <<EOF > $BUILD_DIR/Dockerfile
    FROM $DOCKER_IMAGE

    # Install dependencies
    RUN yum install -y epel-release
    RUN dnf config-manager --set-enabled powertools && \
        dnf install -y https://pkgs.dyn.su/el8/base/x86_64/raven-release-1.0-2.el8.noarch.rpm && \
        dnf --enablerepo=epel group
    RUN yum install --enablerepo=raven-extras -y \
        ccache \
        cmake \
        ninja-build \
        gcc \
        gcc-c++ \
        libXrandr-devel \
        libXinerama-devel \
        libXcursor-devel \
        libXi-devel \
        freetype-devel \
        vulkan \
        vulkan-tools \
        vulkan-headers \
        vulkan-loader \
        glslc

    # Set up environment variables
    ENV CCACHE_DIR=/ccache
    ENV PATH=/usr/lib/ccache:\$PATH

    # Copy source files into the container
    COPY . /workspace

    # Set the working directory
    WORKDIR /workspace

    # Build the project
    RUN mkdir -p $BUILD_DIR && \
        cd $BUILD_DIR && \
        CMAKE_CXX_COMPILER_LAUNCHER=ccache cmake .. -GNinja -DCMAKE_MESSAGE_LOG_LEVEL=INFO -DCMAKE_BUILD_TYPE=$release || true && \
        ninja || true
    RUN cd $BUILD_DIR && \
        CMAKE_CXX_COMPILER_LAUNCHER=ccache cmake .. -GNinja -DCMAKE_MESSAGE_LOG_LEVEL=INFO -DCMAKE_BUILD_TYPE=$release && \
        ninja

    EOF

    # Build the Docker image
    docker build -t $CONTAINER_NAME $BUILD_DIR

    # Run the Docker container and keep it running after the build
    docker run --name $CONTAINER_NAME-container $CONTAINER_NAME

    # Copy the built files from the container to the host directory
    docker cp -L $CONTAINER_NAME-container:/workspace/$BUILD_DIR/libdatoviz.so $BUILD_DIR/
    docker cp -L $CONTAINER_NAME-container:/usr/lib64/libvulkan.so.1 $BUILD_DIR/libvulkan.so
    docker cp -L $CONTAINER_NAME-container:/usr/bin/glslc $BUILD_DIR/

    # Clean up the container
    docker rm $CONTAINER_NAME-container

    # Clean up the temporary Dockerfile
    rm $BUILD_DIR/Dockerfile
#

pydev: # install the Python binding on a development machine
    @pip install -e .
#


# -------------------------------------------------------------------------------------------------
# Tests
# -------------------------------------------------------------------------------------------------

[linux]
test test_name="":
    ./build/datoviz test {{test_name}}
#

[macos]
test test_name="":
    @VK_DRIVER_FILES="libs/vulkan/macos/MoltenVK_icd.json" ./build/datoviz test {{test_name}}
#

[windows]
test test_name="":
    ./build/datoviz.exe test {{test_name}}
#


# -------------------------------------------------------------------------------------------------
# Demo
# -------------------------------------------------------------------------------------------------

demo:
    ./build/datoviz demo

[linux]
libdemo:
    @LD_LIBRARY_PATH=build/ python3 -c "import ctypes; ctypes.cdll.LoadLibrary('libdatoviz.so').dvz_demo()"
#

[macos]
libdemo:
    @DYLD_LIBRARY_PATH=build/ VK_DRIVER_FILES="$(pwd)/libs/vulkan/macos/MoltenVK_icd.json" python3 -c "import ctypes; ctypes.cdll.LoadLibrary('libdatoviz.dylib').dvz_demo()"
#

[windows]
libdemo:
    python3 -c "import ctypes; ctypes.cdll.LoadLibrary('libdatoviz.dll').dvz_demo()"
#

python *args:
    @PYTHONPATH=. python {{args}}


# -------------------------------------------------------------------------------------------------
# Shared library
# -------------------------------------------------------------------------------------------------

headers:
    @python tools/parse_headers.py
#

exports:
    @nm -D --defined-only build/libdatoviz.so
#

[macos]
deps:
    @otool -L build/libdatoviz.dylib | sort -r
#

[linux]
deps:
    @ldd build/libdatoviz.so
#

[macos]
rpath:
    @otool -l build/libdatoviz.dylib | awk '/LC_RPATH/ {getline; getline; print $2}'
#

[linux]
rpath:
    @objdump -x build/libdatoviz.so | grep 'R.*PATH'
#


# -------------------------------------------------------------------------------------------------
# Swiftshader
# -------------------------------------------------------------------------------------------------

[linux]
swiftshader +args:
    @VK_ICD_FILENAMES="data/swiftshader/linux/vk_swiftshader_icd.json" {{args}}
#

[macos]
swiftshader +args:
    @VK_ICD_FILENAMES="data/swiftshader/macos/vk_swiftshader_icd.json" {{args}}
#


# -------------------------------------------------------------------------------------------------
# Code quality
# -------------------------------------------------------------------------------------------------

format:
    find tests/ src/ include/ -iname *.h -o -iname *.c | xargs clang-format -i
#

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
#

cppcheck:
    cppcheck --enable=all --inconclusive src/ include/ cli/ tests/ -i external -I include/datoviz
    # 2> .cppcheck.out.txt && \
    # echo ".cppcheck.out.txt saved"
#

prof:
    gprof build/datoviz gmon.out
#

tree:
    tree -I external -I "build*" -I data -I bin -I libs -I tools -I "packaging*" -I docs -I cmake -I "*.py" -I "*.pxd" -I "*.pyx" -I "*.json" -I "*.out"
#

cloc:
    cloc . --exclude-dir=bin,build,build_clang,cmake,data,datoviz,docs,external,libs,packaging,tools
#


# -------------------------------------------------------------------------------------------------
# Linux packaging
# -------------------------------------------------------------------------------------------------

[linux]
deb: checkstructs
    #!/usr/bin/env sh
    DEB="packaging/deb/"
    INCLUDEDIR="/usr/local/include/datoviz"
    LIBDIR="/usr/local/lib/datoviz"
    LIB=$DEB$LIBDIR/libdatoviz.so

    # Clean up and prepare the directory structure.
    rm -rf $DEB
    mkdir -p $DEB/DEBIAN
    mkdir -p $DEB$INCLUDEDIR
    mkdir -p $DEB$LIBDIR

    # Create the control file.
    echo "Package: datoviz
    Version: {{VERSION}}
    Section: libs
    Priority: optional
    Architecture: amd64
    Maintainer: {{MAINTAINER}}
    Description: {{DESCRIPTION}}" > $DEB/DEBIAN/control

    # Copy the header files.
    cp -a include/datoviz*.h $DEB$INCLUDEDIR

    # Copy the libraries.
    cp -a build/libdatoviz.so* $DEB$LIBDIR
    cp -a libs/vulkan/linux/libvulkan.so* $DEB$LIBDIR

    # Copy the Python ctypes wrapper/
    cp -a datoviz/__init__.py $DEB$LIBDIR/__init__.py

    # Remove the first rpath
    patchelf --remove-rpath $LIB

    # Show the dependencies of the packaged datoviz library.
    echo "Dependencies:"
    ldd $LIB | sort -r

    # Create the post-install script.
    echo "#!/usr/bin/env sh
    SITE_PACKAGES=\$(python3 -m site --user-site)
    mkdir -p \$SITE_PACKAGES
    ln -sf /usr/local/lib/datoviz \$SITE_PACKAGES/datoviz" > $DEB/DEBIAN/postinst
    chmod 755 $DEB/DEBIAN/postinst

    # Build the package.
    fakeroot dpkg-deb --build $DEB

    # Display the tree structure of the package.
    TEMP_DIR=$(mktemp -d)
    dpkg-deb -x "packaging/deb.deb" "$TEMP_DIR"
    tree -h "$TEMP_DIR"
    rm -rf "$TEMP_DIR"

    # Move it.
    mv packaging/deb.deb packaging/datoviz_{{VERSION}}_amd64.deb
    rm -rf $DEB
#

[linux]
testdeb:
    #!/usr/bin/env sh

    # Check if the deb package exists, if not, build it
    if [ ! -f packaging/datoviz_*_amd64.deb ]; then
        just deb
    fi

    # Create a Dockerfile for testing
    echo "$(cat Dockerfile_ubuntu)

    COPY packaging/datoviz_*_amd64.deb /tmp/
    RUN dpkg -i /tmp/datoviz_*_amd64.deb || apt-get install -f -y

    COPY examples/scatter.c /root/
    WORKDIR /root

    # Build a C standalone file depending on libdatoviz.
    RUN gcc -o example_scatter scatter.c \
        -DOS_LINUX=1 \
        -I/usr/local/include/datoviz \
        -L/usr/local/lib/datoviz \
        -Wl,-rpath,/usr/local/lib/datoviz \
        -lm -ldatoviz

    # Run the compiled C example and also try the Python import.
    CMD ./example_scatter && python3 -c 'import datoviz; datoviz.demo()'

    " > Dockerfile

    # Build the Docker image
    docker build -t datoviz_deb_test .

    # Run the Docker container
    docker run --runtime=nvidia --gpus all -e DISPLAY=$DISPLAY -v /tmp/.X11-unix/:/tmp/.X11-unix/ --rm datoviz_deb_test

    rm Dockerfile
#


# -------------------------------------------------------------------------------------------------
# macOS packaging
# -------------------------------------------------------------------------------------------------

[macos]
pkg: checkstructs
    #!/usr/bin/env sh
    PKGROOT="packaging/pkgroot/Payload"
    PKGSCRIPTS="packaging/pkgroot/Scripts"
    INCLUDEDIR="/usr/local/include/datoviz"
    LIBDIR="/usr/local/lib/datoviz"
    PKG="packaging/pkg"

    # Clean up and prepare the directory structure.
    mkdir -p $PKGROOT $PKGSCRIPTS $PKG
    rm -rf $PKGROOT/* $PKG/*
    mkdir -p $PKGROOT$INCLUDEDIR
    mkdir -p $PKGROOT$LIBDIR

    # Copy the header files.
    cp include/datoviz*.h $PKGROOT$INCLUDEDIR

    # Define INCLUDE_VK_DRIVER_FILES in the header file so that the VK_DRIVER_FILES env variable
    # is set to the correct file installed by the pkg package.
    sed -i '' '1i\
    #define INCLUDE_VK_DRIVER_FILES
    ' "$PKGROOT$INCLUDEDIR/datoviz.h"

    cp -a datoviz/__init__.py $PKGROOT$LIBDIR/__init__.py
    cp -a libs/vulkan/macos/MoltenVK_icd.json $PKGROOT$LIBDIR

    # Copy the shared libraries.
    cp -a build/libdatoviz.*dylib $PKGROOT$LIBDIR
    cp -a libs/vulkan/macos/libvulkan.*dylib $PKGROOT$LIBDIR
    cp -a libs/vulkan/macos/libMoltenVK.*dylib $PKGROOT$LIBDIR

    # Post-install script for Python installation
    # Create a symlink from the local site-packages to /usr/local/lib/datoviz so that
    # one can do "import datoviz" in Python, it will load /usr/local/lib/datoviz/__init__.py
    # which contains the ctypes bindings.
    cat << 'EOF' > $PKGSCRIPTS/postinstall
    #!/bin/bash
    echo "Starting postinstall script"
    PYTHON_SITE_PACKAGES=$(python3 -c 'import site; print(site.getusersitepackages())')
    mkdir -p $PYTHON_SITE_PACKAGES
    echo "Creating symlink to $PYTHON_SITE_PACKAGES"
    ln -sf /usr/local/lib/datoviz "$PYTHON_SITE_PACKAGES/datoviz"
    EOF

    # Make the postinstall script executable
    chmod +x $PKGSCRIPTS/postinstall

    # Copy the dependencies and adjust their rpaths.
    # ALTERNATIVE:
    # cp -a $(otool -L build/libdatoviz.dylib | grep brew | awk '{print $1}') $PKGROOT$LIBDIR
    # Path to libdatoviz.
    LIB=$PKGROOT$LIBDIR/libdatoviz.dylib
    # List all Homebrew/Vulkan dependencies, copy them to the package tree, and update the rpath of
    # libdatoviz to point to these local copies.
    otool -L $LIB | grep -E "brew|rpath" | awk '{print $1}' | while read -r dep; do
        filename=$(basename "$dep")
        echo $filename
        if [[ "$dep" != *"rpath"* ]]; then
            cp -a $dep $PKGROOT$LIBDIR/
        fi
        install_name_tool -change "$dep" "@loader_path/$filename" $LIB
    done

    # Remove the rpath that links to a build directory.
    target_rpath=$(otool -l $LIB | awk '/LC_RPATH/ {getline; getline; if ($2 ~ /libs\/vulkan\/macos/) print $2}')
    if [ -n "$target_rpath" ]; then
        install_name_tool -delete_rpath "$target_rpath" $LIB
    fi

    # Show the dependencies of the packaged datoviz library.
    echo "Dependencies:"
    otool -L $LIB | sort -r

    # Show the rpath.
    echo "rpath:"
    otool -l "$LIB" | awk '/LC_RPATH/ {getline; getline; print $2}'

    # Build the package.
    pkgbuild --root $PKGROOT --scripts $PKGSCRIPTS --identifier com.datoviz --version {{VERSION}} --install-location / $PKG/datoviz.pkg
    # NOTE: unneeded:
    # productbuild --package-path $PKG --package $PKG/datoviz.pkg $PKG/datoviz_installer.pkg

    # Display information about the contents of the .pkg file.
    pkgutil --expand $PKG/datoviz.pkg $PKG/extracted
    cd $PKG/extracted && cat Payload | gunzip -dc | cpio -i
    tree . -ugh && cd -

    # Move it.
    cp $PKG/datoviz.pkg packaging/datoviz_{{VERSION}}.pkg
    rm -rf $PKGROOT $PKG $PKGSCRIPTS
    rmdir packaging/pkgroot
#

[macos]
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
        -DOS_MACOS=1 \
        -I/usr/local/include/datoviz/ -L/usr/local/lib/datoviz/ \
        -Wl,-rpath,/usr/local/lib/datoviz -lm -ldatoviz

    echo "Compilation finished. The example executable is located at $TMPDIR/example_scatter"
    EOF
#


# -------------------------------------------------------------------------------------------------
# Python ctypes wrapper generation
# -------------------------------------------------------------------------------------------------

ctypes:
    @python tools/generate_ctypes.py
#

fullctypes: build headers
    @just ctypes
    @just checkstructs
#


# -------------------------------------------------------------------------------------------------
# Python packaging: generic
# -------------------------------------------------------------------------------------------------

showwheel:
    @unzip -l dist/*.whl
#

renamewheel:
    #!/usr/bin/env sh
    set -e

    # Rename the wheel depending on the current platform.
    if [ ! -f dist/*any.whl ]; then
        echo "No universal wheel to rename in dist/"
        exit 1
    fi

    WHEELPATH=$(ls dist/*any.whl 2>/dev/null)
    PLATFORM_TAG=$(python -c "from wheel.bdist_wheel import get_platform; print(get_platform('datoviz'))")
    TAG="cp3-none-$PLATFORM_TAG"

    python -m wheel tags --platform-tag $PLATFORM_TAG $WHEELPATH
    rm $WHEELPATH
#


# -------------------------------------------------------------------------------------------------
# Python packaging: Linux
# -------------------------------------------------------------------------------------------------

[linux]
wheel: checkstructs
    #!/usr/bin/env sh
    set -e
    PKGROOT="packaging/wheel"
    DVZDIR="$PKGROOT/datoviz"
    DISTDIR="dist"

    # Clean up and prepare the directory structure.
    mkdir -p $PKGROOT $DISTDIR
    mkdir -p $DVZDIR

    # Copy the header files.
    cp datoviz/__init__.py $DVZDIR
    cp pyproject.toml $PKGROOT/
    cp build/libdatoviz.so $DVZDIR
    cp libs/vulkan/linux/libvulkan.so $DVZDIR

    # Build the wheel.
    cd $PKGROOT
    pip wheel . -w "../../$DISTDIR"
    cd -
    just renamewheel

    rm -rf $PKGROOT
#

[linux]
wheelmany: checkstructs
    #!/usr/bin/env sh
    PKGROOT="packaging/wheel"
    DVZDIR="$PKGROOT/datoviz"
    DISTDIR="dist"
    DOCKER_IMAGE="quay.io/pypa/manylinux_2_28_x86_64"

    # Clean up and prepare the directory structure.
    mkdir -p $PKGROOT $DISTDIR
    mkdir -p $DVZDIR

    # Copy the header files.
    cp datoviz/__init__.py $DVZDIR
    cp pyproject.toml $PKGROOT/
    cp build_many/libdatoviz.so $DVZDIR
    cp build_many/libvulkan.so $DVZDIR

    # Create a temporary Dockerfile
    cat <<EOF > $PKGROOT/Dockerfile
    FROM $DOCKER_IMAGE

    # Set up environment variables
    ENV PKGROOT=/pkg
    ENV DVZDIR=\$PKGROOT/datoviz

    # Install dependencies
    RUN /opt/python/cp38-cp38/bin/pip install --upgrade pip setuptools wheel

    # Copy the package files
    RUN mkdir -p \$PKGROOT
    WORKDIR \$PKGROOT
    COPY . \$PKGROOT

    # Build the wheel
    CMD /opt/python/cp38-cp38/bin/pip wheel \$PKGROOT -w "/pkg/dist"
    EOF

    # Build the Docker image and create the wheel
    docker build -t datoviz-wheelmany $PKGROOT
    docker run --rm -v $(pwd)/$DISTDIR:/pkg/dist datoviz-wheelmany
    ls -lah $(pwd)/$DISTDIR

    # Clean up
    rm -rf $PKGROOT
#

[linux]
testwheel:
    #!/usr/bin/env sh

    if [ ! -f dist/datoviz-*.whl ]; then
        just wheel
    fi

    # Create a Dockerfile for testing
    echo "$(cat Dockerfile_ubuntu)

    COPY dist/datoviz-*.whl /tmp/
    RUN python3 -m venv /tmp/venv
    RUN /tmp/venv/bin/pip install /tmp/datoviz-*.whl

    WORKDIR /root
    CMD /tmp/venv/bin/python3 -c \"import datoviz; datoviz.demo()\"

    " > Dockerfile

    # Build the Docker image
    docker build -t datoviz_wheel_test .

    # Run the Docker container
    docker run --runtime=nvidia --gpus all -e DISPLAY=$DISPLAY -v /tmp/.X11-unix/:/tmp/.X11-unix/ --rm datoviz_wheel_test

    rm Dockerfile
#


# -------------------------------------------------------------------------------------------------
# Python packaging: macOS
# -------------------------------------------------------------------------------------------------

[macos]
wheel: checkstructs
    #!/usr/bin/env sh
    PKGROOT="packaging/wheel"
    DVZDIR="$PKGROOT/datoviz"
    DISTDIR="dist"

    # Clean up and prepare the directory structure.
    mkdir -p $PKGROOT $DISTDIR
    mkdir -p $DVZDIR

    # Copy the header files.
    cp datoviz/__init__.py $DVZDIR
    cp pyproject.toml $PKGROOT/
    cp build/libdatoviz.dylib $DVZDIR
    cp libs/vulkan/macos/libvulkan.1.dylib $DVZDIR
    cp libs/vulkan/macos/libMoltenVK.dylib $DVZDIR
    cp libs/vulkan/macos/MoltenVK_icd.json $DVZDIR

    # Copy the dependencies and adjust their rpaths.
    # Path to libdatoviz.
    LIB=$DVZDIR/libdatoviz.dylib
    # List all Homebrew/Vulkan dependencies, copy them to the package tree, and update the rpath of
    # libdatoviz to point to these local copies.
    otool -L $LIB | grep -E "brew|rpath" | awk '{print $1}' | while read -r dep; do
        filename=$(basename "$dep")
        if [[ "$dep" != *"rpath"* ]]; then
            cp -a $dep $DVZDIR
        fi
        install_name_tool -change "$dep" "@loader_path/$filename" $LIB
    done

    # Remove the rpath that links to a build directory.
    target_rpath=$(otool -l $LIB | awk '/LC_RPATH/ {getline; getline; if ($2 ~ /libs\/vulkan\/macos/) print $2}')
    if [ -n "$target_rpath" ]; then
        install_name_tool -delete_rpath "$target_rpath" $LIB
    fi

    # Create the wheel.
    cd $PKGROOT
    pip wheel . -w "../../$DISTDIR"

    # Cleanup.
    cd -
    rm -rf $PKGROOT

    # Rename the wheel.
    just renamewheel

    # Show the wheel contents.
    just showwheel
#


[macos]
testwheel vm_ip_address:
    #!/usr/bin/env sh
    IP="{{vm_ip_address}}"
    TMPDIR=/tmp/datoviz_example

    # Check if the pkg package exists, if not, build it
    if ! ls dist/datoviz*.whl 1> /dev/null 2>&1; then
        echo "Wheel file not found in dist/"
        exit
    fi
    WHEEL_PATH=$(ls dist/datoviz*.whl)

    # Copy the .wheel file to the VM
    ssh -T $USER@$IP "mkdir -p "$TMPDIR/" && rm -rf $TMPDIR/*"
    scp $WHEEL_PATH $USER@$IP:$TMPDIR

    # Connect to the VM and install the .pkg file
    ssh -T $USER@$IP << 'EOF'
    TMPDIR=/tmp/datoviz_example
    WHEEL_FILENAME="datoviz-{{VERSION}}-py3-none-any.whl"
    PYTHONPATH=~/.local/lib/python3.8/site-packages
    mkdir -p $PYTHONPATH
    python3 -m pip install "$TMPDIR/$WHEEL_FILENAME" --upgrade --target $PYTHONPATH
    PYTHONPATH=$PYTHONPATH python3 -c "import datoviz; import datoviz; datoviz.demo()"
    EOF
#


# -------------------------------------------------------------------------------------------------
# Documentation
# -------------------------------------------------------------------------------------------------

doc: headers
    @python tools/generate_doc.py
#


# -------------------------------------------------------------------------------------------------
# Examples
# -------------------------------------------------------------------------------------------------

[linux]
runexample name="":
    ./build/example_{{name}}
#

[macos]
runexample name="":
    @VK_DRIVER_FILES="libs/vulkan/macos/MoltenVK_icd.json" ./build/example_{{name}}
#

example name="":
    gcc -o build/example_{{name}} examples/{{name}}.c -Iinclude/ -Lbuild/ -Wl,-rpath,build -lm -ldatoviz
    just runexample {{name}}
#


# -------------------------------------------------------------------------------------------------
# Cleaning
# -------------------------------------------------------------------------------------------------

clean:
    just rmbuild
#

rebuild:
    just rmbuild
    just build || just build
#
