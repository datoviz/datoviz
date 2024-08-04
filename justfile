# -------------------------------------------------------------------------------------------------
# Constants
# -------------------------------------------------------------------------------------------------

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
    #!/usr/bin/env python
    import ctypes
    import json

    def _check_struct_sizes(json_path):
        """Check the size of the ctypes structs and unions with respect to the sizes output by
        the CMake process (small executable in tools/struct_sizes.c compiled and executed by CMake).
        """
        with open(json_path, "r") as f:
            sizes = json.load(f)
        import datoviz as dvz
        for name, size_c in sizes.items():
            obj = getattr(dvz, name)
            assert obj
            size_ctypes = ctypes.sizeof(obj)
            assert size_ctypes > 0
            if size_c != size_ctypes:
                raise ValueError(
                    f"Mismatch struct/union size error with {name}, "
                    f"C struct/union size is {size_c} whereas the ctypes size is {size_ctypes}")
        print(f"Sizes of {len(sizes)} structs/unions successfully checked.")

    _check_struct_sizes('build/struct_sizes.json')
#

[unix]
build release="Debug":
    @set -e
    @unset CC
    @unset CXX
    @mkdir -p docs/images
    @mkdir -p build
    @cd build/ && CMAKE_CXX_COMPILER_LAUNCHER=ccache cmake .. -GNinja -DCMAKE_BUILD_TYPE={{release}}
    @cd build/ && ninja
#

[windows]
build release="Debug":
    #!/usr/bin/env sh
    set -e
    unset CC
    unset CXX
    mkdir -p build
    cp libs/vulkan/windows/vulkan-1.dll build/

    # Copy mingw64 shared libraries.
    MINGW64_DIR="$(dirname $(which gcc))"
    cp "$MINGW64_DIR/libgcc_s_seh-1.dll" build/
    cp "$MINGW64_DIR/libstdc++-6.dll" build/
    cp "$MINGW64_DIR/libwinpthread-1.dll" build/

    pushd build/
    CMAKE_CXX_COMPILER_LAUNCHER=ccache cmake .. --preset=default -DCMAKE_BUILD_TYPE={{release}}
    cmake --build .
    popd
#

release: headers symbols
    just build "Release" || just build "Release"
#

rmbuild:
    rm -rf ./build/
#

clang:
    export CC=/usr/bin/clang
    export CXX=/usr/bin/clang++
    just build
#

buildmany release="Release":
    #!/usr/bin/env sh
    set -e
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
    @pip install -r requirements-dev.txt
    @pip install -e .
#

version:
    #!/usr/bin/env sh
    set -e
    VERSION=$(awk '
    /#define DVZ_VERSION_MAJOR/ { major = $3 }
    /#define DVZ_VERSION_MINOR/ { minor = $3 }
    /#define DVZ_VERSION_PATCH/ { patch = $3 }
    /#define DVZ_VERSION_DEV/ { dev = $3 }
    END { print major "." minor "." patch dev }
    ' "include/datoviz_version.h")
    echo ${VERSION}
#

bump version:
    #!/usr/bin/env python
    from datetime import datetime
    import os
    import re

    # Define the version
    version = "{{version}}"
    major, minor, patch = version.split('.')
    dev = ""
    if "-" in patch:
        patch, dev = patch.split("-")
        dev = "-" + dev
    # dev variable contains either "" or "-dev"

    # Function to update file content using regex
    def update_file(file_path, patterns_replacements):
        with open(file_path, 'r') as file:
            content = file.read()
        for pattern, replacement in patterns_replacements:
            content = re.sub(pattern, replacement, content, flags=re.MULTILINE)
        with open(file_path, 'w') as file:
            file.write(content)

    # Update the include file with the new version numbers
    include_file = "include/datoviz_version.h"
    include_patterns_replacements = [
        (r'#define DVZ_VERSION_MAJOR \d+', f'#define DVZ_VERSION_MAJOR {major}'),
        (r'#define DVZ_VERSION_MINOR \d+', f'#define DVZ_VERSION_MINOR {minor}'),
        (r'#define DVZ_VERSION_PATCH \d+', f'#define DVZ_VERSION_PATCH {patch}'),
        (r'#define DVZ_VERSION_DEVEL[^\n]*', f'#define DVZ_VERSION_DEVEL {dev}'.strip())
    ]
    update_file(include_file, include_patterns_replacements)
    print(f"Updated {include_file}")

    # Get today's date in ISO format (YYYY-MM-DD)
    today_date = datetime.now().strftime("%Y-%m-%d")

    # Update the CITATION.cff file with the new version number
    citation_file = "CITATION.cff"
    citation_patterns_replacements = [
        (r'^version: .+', f'version: {version}'),
        (r'^date-released: .+', f'date-released: {today_date}'),
    ]
    update_file(citation_file, citation_patterns_replacements)
    print(f"Updated {citation_file}")

    # Update the pyproject.toml file with the new version number
    toml_file = "pyproject.toml"
    toml_patterns_replacements = [
        (r'^version = ".+"', f'version = "{version}"')
    ]
    update_file(toml_file, toml_patterns_replacements)
    print(f"Updated {toml_file}")

    # Call the `just ctypes` command
    os.system("just ctypes")
    print("Updated ctypes wrapper")
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
# Info
# -------------------------------------------------------------------------------------------------

[unix]
info:
    @./build/datoviz info
#

[windows]
info:
    @build/datoviz.exe info
#


# -------------------------------------------------------------------------------------------------
# Demo
# -------------------------------------------------------------------------------------------------

demo:
    ./build/datoviz demo
#

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
    python -c "import ctypes; ctypes.cdll.LoadLibrary('libdatoviz.dll').dvz_demo()"
#

python *args:
    @PYTHONPATH=. python {{args}}
#


# -------------------------------------------------------------------------------------------------
# Shared library
# -------------------------------------------------------------------------------------------------

headers:
    @python tools/parse_headers.py
#

symbols:
    #!/usr/bin/env python
    import json
    with open("tools/headers.json", "r") as f:
        headers = json.load(f)
    with open("symbols.map", "w") as f:
        for fn, items in headers.items():
            for function in items["functions"].keys():
                f.write(f"{function}\n")
#

[linux]
exports:
    @nm -D --defined-only build/libdatoviz.so
#

[macos]
exports:
    @nm -gU build/libdatoviz.dylib
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
    set -e
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
    Version: $(just version)
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
    mv packaging/deb.deb packaging/datoviz_$(just version)_amd64.deb
    rm -rf $DEB
#

[linux]
testdeb:
    #!/usr/bin/env sh
    set -e

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
    set -e
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
    pkgbuild --root $PKGROOT --scripts $PKGSCRIPTS --identifier com.datoviz --version $(just version) --install-location / $PKG/datoviz.pkg
    # NOTE: unneeded:
    # productbuild --package-path $PKG --package $PKG/datoviz.pkg $PKG/datoviz_installer.pkg

    # Display information about the contents of the .pkg file.
    pkgutil --expand $PKG/datoviz.pkg $PKG/extracted
    cd $PKG/extracted && cat Payload | gunzip -dc | cpio -i
    tree . -ugh && cd -

    # Move it.
    cp $PKG/datoviz.pkg packaging/datoviz_$(just version).pkg
    rm -rf $PKGROOT $PKG $PKGSCRIPTS
    rmdir packaging/pkgroot
#

[macos]
testpkg vm_ip_address:
    #!/usr/bin/env sh
    set -e
    IP="{{vm_ip_address}}"
    TMPDIR=/tmp/datoviz_example

    # Check if the pkg package exists, if not, build it
    if [ ! -f packaging/datoviz_$(just version).pkg ]; then
        just pkg
    fi

    # Copy the .pkg file to the VM
    ssh -T $USER@$IP "mkdir -p $TMPDIR && rm -rf $TMPDIR/*"
    scp packaging/datoviz_$(just version).pkg \
        examples/scatter.c \
        $USER@$IP:$TMPDIR

    # Connect to the VM and install the .pkg file
    ssh -T $USER@$IP << 'EOF'
    # Install the .pkg package
    TMPDIR=/tmp/datoviz_example
    echo "$USER" | sudo -S installer -pkg $TMPDIR/datoviz_$(just version).pkg -target /
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

fullctypes: headers ctypes checkstructs
#


# -------------------------------------------------------------------------------------------------
# Python tests
# -------------------------------------------------------------------------------------------------

pytest:
    @pytest tests.py
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

    echo "Rename $WHEELPATH"
    python -m wheel tags --platform-tag $PLATFORM_TAG $WHEELPATH
    rm $WHEELPATH
#


# -------------------------------------------------------------------------------------------------
# Python packaging: Linux
# -------------------------------------------------------------------------------------------------

[linux]
wheel: checkstructs && showwheel
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
    pip wheel . -w "../../$DISTDIR" --no-deps
    cd -
    just renamewheel

    rm -rf $PKGROOT
#

[linux]
wheelmany: checkstructs && showwheel
    #!/usr/bin/env sh
    set -e
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
    CMD /opt/python/cp38-cp38/bin/pip wheel \$PKGROOT -w "/pkg/dist" --no-deps
    EOF

    # Build the Docker image and create the wheel
    docker build -t datoviz-wheelmany $PKGROOT
    docker run --rm -v $(pwd)/$DISTDIR:/pkg/dist datoviz-wheelmany
    ls -lah $(pwd)/$DISTDIR

    # Fix permissions.
    sudo chown $(whoami):$(id -gn) $(pwd)/$DISTDIR/*.whl

    # Rename the wheel
    just renamewheel

    # Clean up
    rm -rf $PKGROOT
#

[linux]
testwheel:
    #!/usr/bin/env sh
    set -e

    if [ ! -f dist/datoviz-*.whl ]; then
        echo "Build the wheel first."
        exit
    fi

    # This command allows connections to the X server from any user.
    xhost +

    # Create a Dockerfile for testing
    echo "$(cat Dockerfile_ubuntu)

    COPY dist/datoviz-*.whl /tmp/
    RUN python3 -m venv /tmp/venv
    RUN /tmp/venv/bin/pip install /tmp/datoviz-*.whl

    WORKDIR /root
    CMD [\"/tmp/venv/bin/python\", \"-c\", \"import datoviz; datoviz.demo()\"]

    " > Dockerfile

    # Build the Docker image
    docker build -t datoviz_wheel_test .

    # Run the Docker container
    docker run --runtime=nvidia --gpus all -e DISPLAY=$DISPLAY -v /tmp/.X11-unix/:/tmp/.X11-unix/ --rm datoviz_wheel_test

    rm Dockerfile
#


# -------------------------------------------------------------------------------------------------
# Python packaging: Windows
# -------------------------------------------------------------------------------------------------

[windows]
wheel: checkstructs && showwheel
    #!/usr/bin/env bash
    set -e
    PKGROOT="packaging/wheel"
    DVZDIR="$PKGROOT/datoviz"
    DISTDIR="dist"

    # Clean up and prepare the directory structure.
    rm -rf "$PKGROOT" "$DISTDIR"
    mkdir -p "$PKGROOT" "$DVZDIR" "$DISTDIR"

    # Copy the header files.
    cp datoviz/__init__.py "$DVZDIR"
    cp pyproject.toml "$PKGROOT/"
    cp build/*.dll "$DVZDIR"

    # Copy mingw64 shared libraries.
    MINGW64_DIR="$(dirname $(which gcc))"
    cp "$MINGW64_DIR/libgcc_s_seh-1.dll" "$DVZDIR"
    cp "$MINGW64_DIR/libstdc++-6.dll" "$DVZDIR"
    cp "$MINGW64_DIR/libwinpthread-1.dll" "$DVZDIR"

    # Build the wheel.
    pushd "$PKGROOT"
    pip wheel . -w "../../$DISTDIR" --no-deps
    popd
    just renamewheel

    # Clean up.
    rm -rf "$PKGROOT"
#

[windows]
testwheel:
    #!/usr/bin/env bash

    # Ensure the wheel exists
    if [ ! -f dist/datoviz-*.whl ]; then
        just wheel
    fi

    # Create a new virtual environment
    python -m venv test_env

    # Activate the virtual environment
    source test_env/Scripts/activate

    # Install the wheel
    pip install dist/datoviz-*.whl

    # Run a test command
    pushd test_env
    python -c "import datoviz; datoviz.demo()"
    popd

    # Deactivate the virtual environment
    deactivate

    # Optionally clean up the environment
    rm -rf test_env
#


# -------------------------------------------------------------------------------------------------
# Python packaging: macOS
# -------------------------------------------------------------------------------------------------

[macos]
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
    pip wheel . -w "../../$DISTDIR" --no-deps

    # Cleanup.
    cd -
    rm -rf $PKGROOT

    # Rename the wheel.
    just renamewheel

    # Show the wheel contents.
    just showwheel
#


[macos]
testwheel vm_ip_address="":
    #!/usr/bin/env sh
    set -e
    IP="{{vm_ip_address}}"
    TMPDIR=/tmp/datoviz_example

    if [ ! $IP]; then
        # Create a new virtual environment
        python -m venv test_env

        # Activate the virtual environment
        source test_env/bin/activate

        # Install the wheel
        pip install dist/datoviz-*.whl

        # Run a test command
        pushd test_env
        python -c "import datoviz; datoviz.demo()"
        popd

        # Deactivate the virtual environment
        deactivate

        # Optionally clean up the environment
        rm -rf test_env
        exit
    fi

    # Check if the pkg package exists, if not, build it
    if ! ls dist/datoviz*.whl 1> /dev/null 2>&1; then
        echo "Wheel file not found in dist/"
        exit
    fi
    WHEEL_PATH=$(ls dist/datoviz*.whl)

    # Copy the .wheel file to the VM
    ssh -T $USER@$IP "mkdir -p "$TMPDIR/" && rm -rf '$TMPDIR/*'"
    scp $WHEEL_PATH $USER@$IP:$TMPDIR

    # Connect to the VM and install the .pkg file
    ssh -T $USER@$IP << 'EOF'

    TMPDIR=/tmp/datoviz_example
    WHEEL_FILENAME=$(ls $TMPDIR/*.whl)

    VENV=/tmp/venv
    rm -rf $VENV
    mkdir -p $VENV
    python3 -m venv $VENV
    #PYTHONPATH=~/.local/lib/python3.8/site-packages
    #mkdir -p $PYTHONPATH
    $VENV/bin/python3 -m pip install "$WHEEL_FILENAME" --upgrade # --target $PYTHONPATH
    $VENV/bin/python3 -c "import datoviz; import datoviz; datoviz.demo()"
    # PYTHONPATH=$PYTHONPATH python3 -c "import datoviz; import datoviz; datoviz.demo()"
    EOF
#


# -------------------------------------------------------------------------------------------------
# Documentation
# -------------------------------------------------------------------------------------------------

doc: headers
    @python tools/generate_doc.py api
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

[windows]
runexample name="":
    ./build/example_{{name}}.exe
#

example name="":
    gcc -o build/example_{{name}} examples/{{name}}.c -Iinclude/ -Lbuild/ -Wl,-rpath,build -lm -ldatoviz
    just runexample {{name}}
#

runexamples:
    #!/usr/bin/env sh
    set -e
    mkdir -p data/screenshots/examples
    for file in examples/*.py; do
        bn=$(basename $file)
        name="${bn%.*}"
        DVZ_CAPTURE_PNG=data/screenshots/examples/$name.png python $file
    done
#

docexamples:
    @python tools/generate_doc.py examples
#

examples: runexamples docexamples
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
