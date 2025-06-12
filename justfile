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
# Versioning and releasing
# -------------------------------------------------------------------------------------------------

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

tag version:
    git tag -a v{{version}} -m "v{{version}}"
#

runid:
    @echo $(gh run list --workflow=WHEELS --json conclusion,databaseId --jq '.[] | select(.conclusion == "success") | .databaseId' | head -n 1)
#

# Download the built wheel artifacts
download:
    #!/usr/bin/env sh
    tag=$(git describe --tags --abbrev=0)
    echo "Tag: $tag"

    run_id=$(just runid)
    echo "Workflow run: $run_id"

    if [ -z "$run_id" ]; then
        echo "No successful workflow run found for 'WHEELS'"
        exit 1
    fi

    artifacts_dir="release_artifacts/$tag"
    if ! ls $artifacts_dir/*.whl 1> /dev/null 2>&1; then
        gh run download "$run_id" --dir "$artifacts_dir"
        find "$artifacts_dir" -mindepth 2 -type f -exec mv -t "$artifacts_dir" {} +
        find "$artifacts_dir" -type d -empty -delete
    fi
#

draft:
    #!/usr/bin/env sh
    just download

    tag=$(git describe --tags --abbrev=0)
    artifacts_dir="release_artifacts/$tag"
    gh release create "$tag" --draft --title "$tag" --notes "" $artifacts_dir/*
    # gh release upload "$tag"
#

upload:
    #!/usr/bin/env sh

    # Put this in your ~/.pypirc:
    # [pypi]
    #     username = __token__
    #     password = pypi-YOUR_API_TOKEN_HERE

    tag=$(git describe --tags --abbrev=0)
    artifacts_dir="release_artifacts/$tag"
    twine upload $artifacts_dir/*.whl
#

wheels:
    #!/usr/bin/env sh
    gh workflow run wheels.yml -r dev
    sleep 2
    URL="https://github.com/datoviz/datoviz/actions"
    xdg-open "$URL" || open "$URL"
#

nightly arg='':
    #!/usr/bin/env sh
    set -e

    DATE=$(date +%Y%m%d)
    VERSION_TAG="dev${DATE}"
    OUTDIR="dist"

    echo "📦 Building nightly wheel with tag: $VERSION_TAG and arg: {{arg}}"

    # Optionally clean the dist directory
    rm -rf $OUTDIR/*
    mkdir -p $OUTDIR

    # Build the wheel
    just wheel {{arg}}

    # Find the built wheel
    WHEEL=$(ls $OUTDIR/datoviz-*.whl | head -n 1)

    if [ ! -f "$WHEEL" ]; then
        echo "❌ No wheel found in $OUTDIR/"
        exit 1
    fi

    # Only rename if not already tagged
    BASENAME=$(basename "$WHEEL")
    if echo "$BASENAME" | grep -q "$VERSION_TAG"; then
        echo "✅ Wheel already tagged with $VERSION_TAG: $BASENAME"
    else
        NEWNAME=$(echo "$BASENAME" | sed "s/dev0/$VERSION_TAG/")
        echo "Renaming $BASENAME → $NEWNAME"
        mv "$WHEEL" "$OUTDIR/$NEWNAME"
    fi

    echo "✅ Nightly wheel ready: $OUTDIR/$(ls $OUTDIR | grep $VERSION_TAG)"
#



# -------------------------------------------------------------------------------------------------
# Building
# -------------------------------------------------------------------------------------------------

checkstructs:
    #!/usr/bin/env python
    import ctypes
    import json

    # Insure local datoviz module is imported.
    import sys
    sys.path.insert(0, '.')

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

clang:
    export CC=/usr/bin/clang
    export CXX=/usr/bin/clang++
    just build
#

[linux]
build release="Debug":
    @set -e
    @unset CC
    @unset CXX
    @mkdir -p docs/images
    @mkdir -p build
    @cp -a libs/vulkan/linux/libvulkan* libs/shaderc/linux/libshaderc* build/
    @cd build/ && CMAKE_CXX_COMPILER_LAUNCHER=ccache cmake .. -GNinja -DCMAKE_BUILD_TYPE={{release}}
    @cd build/ && ninja
#

[macos]
build release="Debug": && bundledeps
    @set -e
    @unset CC
    @unset CXX
    @mkdir -p docs/images
    @mkdir -p build
    @cp -a libs/vulkan/macos/libvulkan.1.*dylib build/
    @cp -a libs/vulkan/macos/libMoltenVK.dylib build/
    @cp -a libs/vulkan/macos/MoltenVK_icd.json build/
    @cp -a libs/shaderc/macos_$([[ "$(arch)" == "aarch64" || "$(arch)" == "arm64" ]] && echo "arm64" || echo "x86_64")/libshaderc*dylib build/
    cd build/ && CMAKE_CXX_COMPILER_LAUNCHER=ccache cmake .. -GNinja -DCMAKE_BUILD_TYPE={{release}}
    cd build/ && ninja
#


[windows]
[linux]
release: headers symbols
    just build "Release" || just build "Release"
#

[macos]
release: headers symbols && bundledeps
    just build "Release" || just build "Release"
#

[linux]
manylinux release="Release":
    #!/usr/bin/env sh
    set -e
    DOCKER_IMAGE="rossant/datoviz_manylinux"
    BUILD_DIR="build_many"
    IMAGE_NAME="datoviz-manylinux"
    DISTDIR="dist"

    mkdir -p $BUILD_DIR
    # HACK: do NOT use the shipped Ubuntu libraries in the RedHat-based Docker container
    rsync -a -v \
        --exclude "libvulkan*" --exclude "glslc" --exclude "justfile" \
        --exclude "__pycache__" --exclude "Dockerfile" --exclude "data/mesh" \
        --exclude "data/misc" --exclude "data/volumes" --exclude "data/gallery" \
        bin cli cmake data datoviz external include libs src tests tools \
        *.toml *.json *.txt *.map *.md *.cff \
        CMakeLists.txt *.map "$BUILD_DIR/"

    # Create a temporary Dockerfile
    cat <<EOF > $BUILD_DIR/Dockerfile
    FROM $DOCKER_IMAGE

    # Copy source files into the container
    COPY . /workspace

    # Set the working directory
    WORKDIR /workspace

    # Build the project
    RUN ldd --version ldd
    # RUN ldd  libs/shaderc/linux/libshaderc_shared.so.1
    RUN mkdir -p build/ $DISTDIR wheel/ && \
        cd build/ && \
        CMAKE_CXX_COMPILER_LAUNCHER=ccache cmake .. -GNinja -DCMAKE_MESSAGE_LOG_LEVEL=INFO -DCMAKE_BUILD_TYPE=$release || true && \
        ninja || true
    RUN cd build/ && \
        CMAKE_CXX_COMPILER_LAUNCHER=ccache cmake .. -GNinja -DCMAKE_MESSAGE_LOG_LEVEL=INFO -DCMAKE_BUILD_TYPE=$release && \
        ninja

    # Copy files before building the wheel.
    RUN \
        mkdir -p wheel wheel/datoviz && \
        cp datoviz/*.py wheel/datoviz/ && \
        cp pyproject.toml wheel/ && \
        cp build/libdatoviz.so wheel/datoviz/ && \
        cp -a libs/shaderc/linux/*.so* wheel/datoviz/ && \
        cp /usr/lib64/libvulkan.so.1 wheel/datoviz/ && \
        cp /usr/bin/glslc wheel/datoviz/

    # Build the wheel.
    RUN /opt/python/cp38-cp38/bin/pip wheel wheel/ -w "$DISTDIR/" --no-deps

    # # Rename the wheel.
    # RUN \
    #     WHEELPATH=$(ls dist/*any.whl 2>/dev/null) && \
    #     PLATFORM_TAG=$(/opt/python/cp38-cp38/bin/python -c "from setuptools.wheel import get_platform; print(get_platform())") && \
    #     TAG="cp3-none-$PLATFORM_TAG" && \
    #     /opt/python/cp38-cp38/bin/python -m wheel tags --platform-tag $PLATFORM_TAG "$DISTDIR/*"

    EOF

    # Build the Docker image
    docker build -t $IMAGE_NAME $BUILD_DIR # --progress=plain

    # Copy the files from the container to the host, in dist/.
    container_id=$(docker create $IMAGE_NAME)
    files=$(docker run --rm $IMAGE_NAME sh -c 'ls /workspace/dist/datoviz*.whl')
    for file in $files; do
        docker cp "$container_id:$file" ./dist/
    done
    docker rm $container_id

    # Check the contents of dist/
    ls -lah $DISTDIR

    # Fix permissions.
    # sudo chown $(whoami):$(id -gn) $DISTDIR/*.whl

    # Rename the wheel
    just renamewheel "manylinux_2_34_x86_64"

    rm -rf wheel/
#

[windows]
build release="Debug":
    #!/usr/bin/env sh
    set -e
    unset CC
    unset CXX
    mkdir -p build
    cp libs/vulkan/windows/vulkan-1.dll build/
    cp libs/shaderc/windows/libshaderc_shared.dll build/

    # Copy mingw64 shared libraries.
    MINGW64_DIR="$(dirname $(which gcc))"
    cp "$MINGW64_DIR/libgcc_s_seh-1.dll" build/
    cp "$MINGW64_DIR/libstdc++-6.dll" build/
    cp "$MINGW64_DIR/libwinpthread-1.dll" build/

    pushd build/
    CMAKE_CXX_COMPILER_LAUNCHER=ccache cmake .. --preset=default -DCMAKE_BUILD_TYPE={{release}}
    cmake --build .

    # Copy vcpkg_installed dll's to datoviz.exe location.
    if [ "{{release}}" == "Debug" ]; then
        cp vcpkg_installed/x64-windows/debug/bin/*.dll .
    else
        cp vcpkg_installed/x64-windows/bin/*.dll .
    fi

    popd
#


# -------------------------------------------------------------------------------------------------
# Docker image and CI/CD
# -------------------------------------------------------------------------------------------------

dockerpush name:
    docker build -t rossant/datoviz_{{name}}:latest -f docker/Dockerfile_{{name}} .
    docker login
    docker push rossant/datoviz_{{name}}:latest
    # docker run -it rossant/datoviz_{{name}}:latest
#

# on macOS do
# export DOCKER_HOST=$(docker context inspect | jq -r '.[0].Endpoints.docker.Host')
[linux]
[macos]
act arg:
    act --bind --env USING_ACT=1 -j {{arg}}
#

[windows]
act arg:
    act --bind --env USING_ACT=1 -P windows-latest=-self-hosted -j {{arg}}
#


# -------------------------------------------------------------------------------------------------
# Linux packaging
# -------------------------------------------------------------------------------------------------

[linux]
deb: checkstructs && rpath
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
    cp -a libs/shaderc/linux/libshaderc*.so* $DEB$LIBDIR

    # Copy the Python files
    cp -a datoviz/ $DEB$LIBDIR/

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
    ldd "$TEMP_DIR/usr/local/lib/datoviz/libdatoviz.so" | sort -r
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
    echo "$(cat docker/Dockerfile_ubuntu)

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

[linux]
wheel almalinux="0":
    #!/usr/bin/env sh
    set -e

    # Create the wheel/ temporary directory
    mkdir -p wheel wheel/datoviz

    # Copy the Python projects files
    cp datoviz/*.py wheel/datoviz/
    cp pyproject.toml wheel/

    # Copy libdatoviz
    cp build/libdatoviz.so wheel/datoviz/

    # Copy the Vulkan shared libraries
    if [ "{{almalinux}}" != "0" ]; then
        cp /usr/lib64/libvulkan.so.1 wheel/datoviz/
        cp /usr/bin/glslc wheel/datoviz/
    else
        cp libs/vulkan/linux/libvulkan.so.1 wheel/datoviz/
        cp libs/shaderc/linux/*.so* wheel/datoviz/
        cp bin/vulkan/linux/glslc wheel/datoviz/
    fi

    # Build the wheel
    pip3 wheel wheel/ -w "dist/" --no-deps

    # Rename the wheel
    if [ "{{almalinux}}" != "0" ]; then
        just renamewheel "manylinux_2_34_x86_64"
    fi

    rm -rf wheel/
#

# Test the wheel in a Docker environment.
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
    echo "$(cat docker/Dockerfile_ubuntu)

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
# macOS packaging
# -------------------------------------------------------------------------------------------------

[macos]
bundledeps lib="build/libdatoviz.dylib":  # && rpath
    #!/usr/bin/env sh
    # Copy the dependencies and adjust their rpaths.

    # ALTERNATIVE:
    # cp -a $(otool -L build/libdatoviz.dylib | grep brew | awk '{print $1}') $PKGROOT$LIBDIR

    # List all Homebrew/Vulkan dependencies, copy them to the package tree, and update the rpath of
    # the library to point to these local copies.
    target=$(dirname {{lib}})
    otool -L {{lib}} | grep -E "brew|rpath|local/opt" | awk '{print $1}' | while read -r dep; do
        filename=$(basename "$dep")
        if [[ "$dep" != *"rpath"* ]]; then
            echo "Copying $dep to $target/"
            cp -a $dep $target
        fi
        # echo "Change $dep to @loader_path/$filename in {{lib}}"
        install_name_tool -change "$dep" "@loader_path/$filename" {{lib}}
    done

    # Remove the rpath that links to a build directory.
    target_rpath=$(otool -l {{lib}} | awk '/LC_RPATH/ {getline; getline; if ($2 ~ /libs\/vulkan\/macos/) print $2}')
    if [ -n "$target_rpath" ]; then
        install_name_tool -delete_rpath "$target_rpath" {{lib}}
    fi
    chmod 775 $target/*
#

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

    # Copy the built files.
    cp -a datoviz/*.py $PKGROOT$LIBDIR/
    cp -a build/MoltenVK_icd.json $PKGROOT$LIBDIR
    cp -a build/*dylib $PKGROOT$LIBDIR
    ls -lah $PKGROOT$LIBDIR

    # Post-install script for Python installation
    # Create a symlink from the local site-packages to /usr/local/lib/datoviz so that
    # one can do "import datoviz" in Python, it will load /usr/local/lib/datoviz/__init__.py
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
    LIB=$PKGROOT$LIBDIR/libdatoviz.dylib
    just bundledeps $LIB

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

[macos]
wheel arg='': checkstructs
    #!/usr/bin/env sh
    set -e
    PKGROOT="packaging/wheel"
    DVZDIR="$PKGROOT/datoviz"
    DISTDIR="dist"

    # Clean up and prepare the directory structure.
    mkdir -p $PKGROOT $DISTDIR
    mkdir -p $DVZDIR

    # Copy the header files.
    cp datoviz/*.py $DVZDIR
    cp pyproject.toml $PKGROOT/
    cp build/libdatoviz.dylib $DVZDIR
    cp build/libvulkan.1.dylib $DVZDIR
    cp build/libshaderc*.1.dylib $DVZDIR
    cp build/libfreetype.6.dylib $DVZDIR
    cp build/libpng16.16.dylib $DVZDIR
    cp build/libMoltenVK.dylib $DVZDIR
    cp build/MoltenVK_icd.json $DVZDIR

    # Copy the dependencies and adjust their rpaths.
    LIB=$DVZDIR/libdatoviz.dylib
    just bundledeps $LIB

    # Create the wheel.
    cd $PKGROOT
    pip wheel . -w "../../$DISTDIR" --no-deps

    # Cleanup.
    cd -
    rm -rf $PKGROOT

    # Rename the wheel.
    just renamewheel {{arg}}

    # Show the wheel contents.
    just showwheel
#

# Test the wheel in a virtual machine
[macos]
testwheel vm_ip_address="":
    #!/usr/bin/env sh
    set -e
    IP="{{vm_ip_address}}"
    TMPDIR=/tmp/datoviz_example

    if [ ! $IP]; then
        just checkwheel
        # # Create a new virtual environment
        # rm -rf test_env
        # python -m venv test_env --system-site-packages

        # # Activate the virtual environment
        # source test_env/bin/activate

        # # Install the wheel
        # pip install dist/datoviz-*.whl

        # # Run a test command
        # pushd test_env
        # python -c "import datoviz; datoviz.demo()"
        # popd

        # # Deactivate the virtual environment
        # deactivate

        # # Optionally clean up the environment
        # rm -rf test_env
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
# Python
# -------------------------------------------------------------------------------------------------

pydev: # install the Python binding on a development machine
    @pip install -r requirements-dev.txt
    @pip install -e .
#

ctypes:
    @python tools/build_ctypes.py
#

pytest:
    @pytest tests.py
#


# -------------------------------------------------------------------------------------------------
# Python packaging
# -------------------------------------------------------------------------------------------------

showwheel:
    @unzip -l dist/*.whl
#

renamewheel platform_tag='':
    #!/usr/bin/env sh
    set -e

    echo "just renamewheel {{platform_tag}}"

    # Rename the wheel depending on the current platform.
    if [ ! -f dist/*any.whl ]; then
        echo "No universal wheel to rename in dist/"
        exit 1
    fi

    WHEELPATH=$(ls dist/*any.whl 2>/dev/null)

    if [ -z "{{platform_tag}}" ]; then
        PLATFORM_TAG=$(python -c "from setuptools.wheel import get_platform; print(get_platform().replace('-', '_'))")
    else
        PLATFORM_TAG="{{platform_tag}}"
    fi
    echo $PLATFORM_TAG

    TAG="cp3-none-$PLATFORM_TAG"

    echo "Rename $WHEELPATH"
    python -m wheel tags --platform-tag $PLATFORM_TAG $WHEELPATH
    rm $WHEELPATH
#

[windows]
wheel: checkstructs && showwheel
    #!/usr/bin/env sh
    set -e
    PKGROOT="packaging/wheel"
    DVZDIR="$PKGROOT/datoviz"
    DISTDIR="dist"

    # Clean up and prepare the directory structure.
    rm -rf "$PKGROOT" "$DISTDIR"
    mkdir -p "$PKGROOT" "$DVZDIR" "$DISTDIR"

    # Copy the header files.
    cp datoviz/*.py "$DVZDIR"
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
    just renamewheel win_amd64

    # Clean up.
    rm -rf "$PKGROOT"
#

testpypi:
    #!/usr/bin/env bash

    # HACK: work around: ERROR: Can not perform a '--user' install. User site-packages are not
    # visible in this virtualenv
    # see https://github.com/gitpod-io/gitpod/issues/1997
    export PIP_USER=false

    case "$(uname -s)" in
    *CYGWIN*|*MINGW*|*MSYS*) BINDIR="Scripts" ;;
    *) BINDIR="bin" ;;
    esac

    # Create a temporary venv.
    rm -rf venv_pypi
    python -m venv venv_pypi
    pushd venv_pypi

    # Make sure Datoviz is not installed in the venv before we pip install it.
    $BINDIR/python -c "exec('try: import datoviz\nexcept: print(\"datoviz not yet installed\")\nelse: raise RuntimeError(\"datoviz already installed\")')"

    # Install datoviz from PyPI
    $BINDIR/pip install datoviz

    # Check the Datoviz demo.
    $BINDIR/python -c "import datoviz; datoviz.demo()"

    # Cleanup the venv.
    popd
    rm -rf venv_pypi
#

checkwheel path="":
    #!/usr/bin/env sh
    set -e

    # Temp directory
    TESTDIR=~/tmp/testwheel
    rm -rf $TESTDIR
    mkdir -p $TESTDIR

    case "$(uname -s)" in
    *CYGWIN*|*MINGW*|*MSYS*) BINDIR="Scripts" ;;
    *) BINDIR="bin" ;;
    esac

    # Copy the wheel
    [ -f "{{path}}" ] && cp {{path}} $TESTDIR || cp dist/datoviz-*.whl $TESTDIR

    # Virtual env
    python -m venv $TESTDIR/venv
    cd $TESTDIR
    $TESTDIR/venv/$BINDIR/python -m pip install --isolated --upgrade pip wheel
    # NOTE: --isolated fixes the pip error 'Can not perform a '--user' install'
    $TESTDIR/venv/$BINDIR/python -m pip install --isolated $TESTDIR/datoviz-*.whl

    # Run the demo from the wheel
    DVZ_CAPTURE_PNG="$TESTDIR/testwheel.png" $TESTDIR/venv/$BINDIR/python -c "import datoviz; datoviz.demo()"

    # Return 0 iff the file exists and if sufficiently large
    res=1
    if [ -f "$TESTDIR/testwheel.png" ]; then
        filesize=$($TESTDIR/venv/$BINDIR/python -c "from pathlib import Path; print(Path(r'testwheel.png').stat().st_size)")
        res=$(( $filesize > 100000 ? 0 : 1 ))
    fi
    rm -rf $TESTDIR
    exit $res
#

checkartifactversion temp_dir:
    #!/usr/bin/env sh
    set -e
    version=$(just version)
    wheel_file=$(find "{{temp_dir}}" -name 'datoviz*.whl' | head -n 1)
    if [ -z "$wheel_file" ]; then
        echo "❌ No wheel file found!"
        rm -rf "{{temp_dir}}"
        exit 1
    fi

    if ! echo "$wheel_file" | grep -q "$version"; then
        echo "❌ Version mismatch: wheel '$wheel_file' does not contain expected version '$version'"
        rm -rf "{{temp_dir}}"
        exit 1
    fi
#

[linux]
checkartifact RUN_ID="":
    #!/usr/bin/env sh
    set -e
    run_id={{RUN_ID}}
    if [ -z "$run_id" ]; then
        run_id=$(just runid)
    fi
    temp_dir=$(mktemp -d)
    gh run download $run_id -n wheel-linux_x86_64 -D $temp_dir
    just checkartifactversion $temp_dir
    just checkwheel $temp_dir/datoviz*.whl
    exit_code=$?
    rm -rf "${temp_dir}"
    exit $exit_code
#

[macos]
checkartifact RUN_ID="":
    #!/usr/bin/env sh
    set -e
    run_id={{RUN_ID}}
    if [ -z "$run_id" ]; then
        run_id=$(just runid)
    fi

    arch_str={{arch()}}
    echo $arch_str
    if [[ "$arch_str" == "aarch64" ]]; then
        platform="arm64"
    else
        platform="x86_64"
    fi

    temp_dir=$(mktemp -d)
    gh run download $run_id -n "wheel-macosx_$platform" -D $temp_dir
    just checkartifactversion $temp_dir
    ls $temp_dir/datoviz*.whl
    just checkwheel $temp_dir/datoviz*.whl
    exit_code=$?
    rm -rf "${temp_dir}"
    exit $exit_code
#

[windows]
checkartifact RUN_ID="":
    #!/usr/bin/env sh
    set -e
    run_id={{RUN_ID}}
    if [ -z "$run_id" ]; then
        run_id=$(just runid)
    fi
    temp_dir=$(mktemp -d)
    gh run download $run_id -n wheel-win_amd64 -D $temp_dir
    just checkartifactversion $temp_dir
    just checkwheel $temp_dir/datoviz*.whl
    exit_code=$?
    rm -rf "${temp_dir}"
    exit $exit_code
#

buildwheel args='':
    #!/usr/bin/env sh
    set -e

    if [ -n "$DVZ_NIGHTLY_TAG" ]; then
        echo "🔁 Detected DVZ_NIGHTLY_TAG=$DVZ_NIGHTLY_TAG — using just nightly"
        just nightly {{args}}
    else
        echo "🎯 No DVZ_NIGHTLY_TAG — using just wheel"
        just wheel {{args}}
    fi



# -------------------------------------------------------------------------------------------------
# Shared library
# -------------------------------------------------------------------------------------------------

headers:
    @python tools/parse_headers.py
#


[linux]
[macos]
symbols:
    @jq -r '.[] | .functions | keys[]' {{justfile_directory()}}/build/headers.json > {{justfile_directory()}}/symbols.map
#

[windows]
symbols:
    @jq -r ".[] | .functions | keys[]" "{{justfile_directory()}}\\build\\headers.json" > "{{justfile_directory()}}\\symbols.map"
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

[linux]
strip:
    @strip --strip-debug build/libdatoviz.so
#

[macos]
rpath:
    @echo "Printing RPATH:"
    @otool -l build/libdatoviz.dylib | grep -i "path"
#

[linux]
rpath:
    @echo "Printing RPATH:"
    @objdump -x build/libdatoviz.so | grep -i 'R.*PATH'
#

tryimport:
    @python -c "import datoviz"
#

api: headers symbols ctypes doc tryimport # after every API update
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

[windows]
swiftshader +args:
    VK_ICD_FILENAMES=data/swiftshader/windows/vk_swiftshader_icd.json \
    VK_LOADER_DEBUG=all \
    {{args}}
#


# -------------------------------------------------------------------------------------------------
# WebAssembly
# -------------------------------------------------------------------------------------------------
wasm:
    set -e
    python3 tools/generate_wasm.py
    ../emsdk/upstream/emscripten/emcc @wasm_sources.txt -o build/datoviz.js \
        -Iinclude/ -Iinclude/datoviz/ -Iexternal/ \
        -Ibuild/_deps/cglm-src/include/ \
        -s MODULARIZE=1 \
        -s EXPORT_NAME='datoviz' \
        -s EXPORT_ES6=1 \
        -s EXPORTED_FUNCTIONS=["$(sed 's/^/_/' wasm_functions.txt | paste -sd, -),_free"] \
        -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap'] \
        -s ALLOW_MEMORY_GROWTH=1 \
        -O3
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

copyright:
    #!/bin/bash

    # Define the copyright text
    COPYRIGHT_TEXT="/*
     * Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
     * Licensed under the MIT license. See LICENSE file in the project root for details.
     * SPDX-License-Identifier: MIT
     */
    "

    # Define the directories to search through
    DIRECTORIES=("tests" "src" "include" "cli")

    # Define the file extensions to look for
    EXTENSIONS=("comp" "vert" "frag" "glsl" "c" "h")

    # Function to prepend text to a file
    prepend_text() {
        local file="$1"
        # Check if the file already contains the copyright text
        if ! grep -q "SPDX-License-Identifier" "$file"; then
            # Prepend the copyright text to the file
            echo $file
            { echo "$COPYRIGHT_TEXT"; cat "$file"; } > temp_file && mv temp_file "$file"
        fi
    }

    # Loop through each directory
    for dir in "${DIRECTORIES[@]}"; do
        # Loop through each extension
        for ext in "${EXTENSIONS[@]}"; do
            # Find all files with the current extension in the current directory
            find "$dir" -type f -name "*.$ext" | while read -r file; do
                prepend_text "$file"
            done
        done
    done
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

[linux]
exec arg:
    @LD_LIBRARY_PATH=build/ python3 -c "import ctypes; print(ctypes.cdll.LoadLibrary('libdatoviz.so').{{arg}}())"
#



# -------------------------------------------------------------------------------------------------
# Demo
# -------------------------------------------------------------------------------------------------

demo:
    ./build/datoviz demo
#

[linux]
pydemo_dll:
    @LD_LIBRARY_PATH=build/ python3 -c "import ctypes; ctypes.cdll.LoadLibrary('libdatoviz.so').dvz_demo()"
#

[macos]
pydemo_dll:
    @DYLD_LIBRARY_PATH=build/ VK_DRIVER_FILES="$(pwd)/libs/vulkan/macos/MoltenVK_icd.json" python3 -c "import ctypes; ctypes.cdll.LoadLibrary('libdatoviz.dylib').dvz_demo()"
#

[windows]
pydemo_dll:
    python -c "import ctypes; ctypes.cdll.LoadLibrary('libdatoviz.dll').dvz_demo()"
#

pydemo:
    python -c "import datoviz; datoviz.demo()"
#

python *args:
    @PYTHONPATH=. python {{args}}
#


# -------------------------------------------------------------------------------------------------
# Examples
# -------------------------------------------------------------------------------------------------

# [linux]
# runexample name="":
#     ./build/example_{{name}}
# #

# [macos]
# runexample name="":
#     @VK_DRIVER_FILES="libs/vulkan/macos/MoltenVK_icd.json" ./build/example_{{name}}
# #

# [windows]
# runexample name="":
#     ./build/example_{{name}}.exe
# #

# example name="":
#     gcc -o build/example_{{name}} examples/c/{{name}}.c -Iinclude/ -Lbuild/ -Wl,-rpath,build -lm -ldatoviz
#     just runexample {{name}}
# #

# Run all Python examples and generate a screenshot in data/gallery/
examples filter="": && gallery
    @echo "Generating screenshots from examples..."
    @python tools/build_screenshots.py {{filter}}


# Build the gallery Markdown files
gallery:
    @echo "Generating the gallery Markdown files..."
    @python tools/build_gallery.py


# -------------------------------------------------------------------------------------------------
# Documentation
# -------------------------------------------------------------------------------------------------

doc: #gallery #headers
    @python tools/build_api_c.py
#

serve:
    @mkdocs serve -a localhost:8294
#

# Publish the mkdocs website on GitHub Pages.
publish:
    #!/usr/bin/env bash
    set -e

    # Get the current branch name
    current_branch=$(git rev-parse --abbrev-ref HEAD)

    # Check if the current branch is "main"
    if [ "$current_branch" != "main" ]; then
        echo "You can only publish the documentation from the main branch (current branch is '$current_branch')."
        exit 1
    fi

    pushd ../datoviz.github.io
    git pull
    mkdocs gh-deploy --config-file ../datoviz/mkdocs.yml --remote-branch main
    popd
#


# -------------------------------------------------------------------------------------------------
# Cleaning
# -------------------------------------------------------------------------------------------------

clean:
    @rm -rf build
#

rebuild:
    just rmbuild
    just build || just build
#

rmbuild:
    @rm -rf build/spirv build/artifacts build/struct_sizes* build/*.dylib* build/*.so* build/*.dll build/datoviz*
#
