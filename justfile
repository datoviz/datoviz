# -------------------------------------------------------------------------------------------------
# Constants
# -------------------------------------------------------------------------------------------------

MAINTAINER := "Cyrille Rossant <cyrille.rossant@gmail.com>"
DESCRIPTION := "A C library for high-performance GPU scientific visualization"
TEMPLATES_DIR := "templates"

# Use a non-system shell so macOS keeps DYLD_* variables from direnv/.envrc.
set shell := ["bash", "-cu"]



# -------------------------------------------------------------------------------------------------
# Management script
# -------------------------------------------------------------------------------------------------

default:
    @echo "No arguments supplied"
    @exit 1
#

check-staged:
    python3 tools/check_staged_payloads.py
#

install-git-hooks:
    #!/usr/bin/env sh
    set -e
    mkdir -p .git/hooks
    {
        printf '%s\n' '#!/usr/bin/env sh'
        printf '%s\n' 'set -e'
        printf '%s\n' 'python3 tools/check_staged_payloads.py'
    } > .git/hooks/pre-commit
    chmod +x .git/hooks/pre-commit
    echo "Installed .git/hooks/pre-commit"
#



# -------------------------------------------------------------------------------------------------
# Templates
# -------------------------------------------------------------------------------------------------

stub path:
    #!/usr/bin/env sh
    path="{{path}}"

    # Determine extension and base name
    ext="${path##*.}"
    base="$(basename "$path")"

    # Pick template based on name pattern or extension
    template=""
    case "$base" in
        *_structs.h) template="{{TEMPLATES_DIR}}/structs.h" ;;
        test*.c)      template="{{TEMPLATES_DIR}}/test.c" ;;
        test*.h)      template="{{TEMPLATES_DIR}}/test.h" ;;
        *.h)          template="{{TEMPLATES_DIR}}/header.h" ;;
        *.c)          template="{{TEMPLATES_DIR}}/source.c" ;;
        *) echo "error: no template defined for $path" >&2; exit 1 ;;
    esac

    mkdir -p "$(dirname "$path")"

    # Extract filename without extension
    title="$(basename "$path" ".$ext")"

    # Split by '_' and take the last part
    title="${title##*_}"

    # Lowercase (POSIX way via tr)
    title="$(printf '%s' "$title" | tr '[:upper:]' '[:lower:]')"

    # Replace <title> placeholder
    sed "s/<title>/$title/g" "$template" > "$path"

    echo "Created stub: $path"
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
    @echo "Use the manual .github/workflows/wheels.yml workflow for v0.4 RC wheel builds." >&2
    @echo "Query the selected GitHub Actions run ID explicitly before downloading artifacts." >&2
    @exit 1
#

# Download the built wheel artifacts
download:
    #!/usr/bin/env sh
    tag=$(git describe --tags --abbrev=0)
    echo "Tag: $tag"

    run_id=$(just runid)
    echo "Workflow run: $run_id"

    if [ -z "$run_id" ]; then
        echo "No successful wheel workflow run found"
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
    echo "The v0.4 wheel workflow is manual: .github/workflows/wheels.yml" >&2
    echo "Use local validation first: just wheel-ci-local <platform-tag>" >&2
    echo "Then dispatch the GitHub Actions workflow from the Actions tab or gh workflow run." >&2
    exit 1
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
        if [ "$BASENAME" = "$NEWNAME" ]; then
            echo "ℹ️  No rename needed: $BASENAME == $NEWNAME"
        else
            echo "Renaming $BASENAME → $NEWNAME"
            mv "$WHEEL" "$OUTDIR/$NEWNAME"
        fi
    fi

    echo "✅ Nightly wheel ready: $OUTDIR/$(ls $OUTDIR | grep $VERSION_TAG)"
#

# Display the list of commits since the last tag.
commits since='' until='':
    #!/usr/bin/env sh
    set -e
    tag=""
    if [ -z "{{since}}" ]; then
        tag=$(git describe --tags --abbrev=0)
        since=$(git log -1 --date=format:'%Y-%m-%d' --format=%ad "${tag}")
    else
        since="{{since}}"
    fi
    if [ -z "{{until}}" ]; then
        until=$(date +%Y-%m-%d)
    else
        until="{{until}}"
    fi
    echo "commits between ${since} (tag: ${tag}) and ${until}:\n"
    git log --since="$since" --until="$until" --pretty=format:"%s" | sort | uniq
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
        import datoviz.raw as dvz
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
    @cmake_args="${DVZ_CMAKE_ARGS:-}"; cd build/ && CMAKE_CXX_COMPILER_LAUNCHER=ccache cmake .. -GNinja -DCMAKE_BUILD_TYPE={{release}} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ${cmake_args}
    @cd build/ && ninja
#

[linux]
build-gprof release="RelWithDebInfo":
    @set -e
    @unset CC
    @unset CXX
    @mkdir -p docs/images
    @mkdir -p build-gprof
    @cd build-gprof && CMAKE_CXX_COMPILER_LAUNCHER=ccache cmake .. -GNinja -DCMAKE_BUILD_TYPE={{release}} -DDVZ_ENABLE_GPROF=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    @cd build-gprof && ninja dvztest
#

[linux]
build-profile:
    @set -e
    @unset CC
    @unset CXX
    @mkdir -p docs/images
    @mkdir -p build-profile
    @cd build-profile/ && CMAKE_CXX_COMPILER_LAUNCHER=ccache cmake .. -GNinja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DDVZ_USE_VALIDATION=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    @cd build-profile/ && ninja dvz_live_canvas
#

[linux]
run-gprof *args:
    @set -e
    @if [ ! -x "build-gprof/testing/dvztest" ]; then echo "build-gprof/testing/dvztest missing; run 'just build-gprof' first"; exit 1; fi
    @cd build-gprof/testing && ./dvztest {{args}}
    @if [ -f build-gprof/testing/gmon.out ]; then \
        echo "Profile stored in build-gprof/testing/gmon.out"; \
    else \
        echo "warning: gmon.out not produced"; \
    fi
#

[linux]
gprof-report gmon="build-gprof/testing/gmon.out" out="build-gprof/testing/gprof.txt":
    @set -e
    @bin="build-gprof/testing/dvztest"; \
    if [ ! -x "$bin" ]; then echo "$bin missing; run 'just build-gprof' first"; exit 1; fi; \
    gmon_file="{{gmon}}"; \
    if [ ! -f "$gmon_file" ]; then echo "profile '$gmon_file' not found"; exit 1; fi; \
    out_file="{{out}}"; \
    mkdir -p "$(dirname "$out_file")"; \
    gprof "$bin" "$gmon_file" > "$out_file"; \
    echo "Report written to $out_file"
#



[linux]
_sanitizer-build name:
    @set -e
    @mkdir -p docs/images build-{{name}}
    @cd build-{{name}}/ && \
      CC=/usr/bin/clang CXX=/usr/bin/clang++ cmake .. -GNinja \
        -DCMAKE_BUILD_TYPE=Debug \
        -DDVZ_ENABLE_ASAN_IN_DEBUG=$([ "{{name}}" = "asan" ] && echo ON || echo OFF) \
        -DDVZ_ENABLE_CUDA=OFF \
        -DDVZ_SANITIZER={{name}} \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    @cd build-{{name}}/ && ninja
#

[linux]
msan:
    just _sanitizer-build msan
#

[linux]
asan:
    just _sanitizer-build asan
#

[linux]
tsan:
    just _sanitizer-build tsan
#

[macos]
build release="Debug": # && bundledeps
    @set -e
    @unset CC
    @unset CXX
    @mkdir -p docs/images
    @mkdir -p build/src
    @mkdir -p build/testing
    cmake_args="${DVZ_CMAKE_ARGS:-}"; cd build/ && CMAKE_CXX_COMPILER_LAUNCHER=ccache cmake .. -GNinja -DCMAKE_BUILD_TYPE={{release}} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ${cmake_args}
    cd build/ && ninja
#

[windows]
[linux]
release: symbols
    just build "Release" || just build "Release"
#

[macos]
release: symbols && bundledeps
    just build "Release" || just build "Release"
#

[windows]
build release="Debug":
    #!/usr/bin/env sh
    set -e
    unset CC
    unset CXX
    cmake_args="${DVZ_CMAKE_ARGS:-}"
    BUILD_DIR="build"
    mkdir -p "$BUILD_DIR"

    # Copy MinGW runtime libraries next to the built artifacts.
    MINGW64_DIR="$(dirname "$(which gcc)")"
    cp "$MINGW64_DIR/libgcc_s_seh-1.dll" "$BUILD_DIR"/
    cp "$MINGW64_DIR/libstdc++-6.dll" "$BUILD_DIR"/
    cp "$MINGW64_DIR/libwinpthread-1.dll" "$BUILD_DIR"/

    CMAKE_CXX_COMPILER_LAUNCHER=ccache cmake --preset=mingw -DCMAKE_BUILD_TYPE={{release}} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ${cmake_args}
    cmake --build --preset mingw

    # Copy vcpkg_installed dll's to datoviz.exe location.
    if [ "{{release}}" == "Debug" ]; then
        cp "$BUILD_DIR/vcpkg_installed/x64-windows/debug/bin/"*.dll "$BUILD_DIR"/
    else
        cp "$BUILD_DIR/vcpkg_installed/x64-windows/bin/"*.dll "$BUILD_DIR"/
    fi
#

[windows]
msvc release="Debug":
    #!/usr/bin/env sh
    set -e
    unset CC
    unset CXX
    cmake_args="${DVZ_CMAKE_ARGS:-}"
    BUILD_DIR="build-msvc"
    mkdir -p "$BUILD_DIR"

    CMAKE_CXX_COMPILER_LAUNCHER=ccache cmake --preset=msvc -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ${cmake_args}
    cmake --build --preset msvc --config {{release}}

    # Copy vcpkg_installed dll's next to datoviz.exe.
    if [ "{{release}}" == "Debug" ]; then
        cp "$BUILD_DIR/vcpkg_installed/x64-windows/debug/bin/"*.dll "$BUILD_DIR"/
    else
        cp "$BUILD_DIR/vcpkg_installed/x64-windows/bin/"*.dll "$BUILD_DIR"/
    fi
#

# Cross-compile Datoviz for Windows using MinGW-w64 (GNU ABI)
[linux]
mingw release="Debug":
    @set -e
    @unset CC
    @unset CXX
    mkdir -p "build-mingw"
    mkdir -p docs/images
    cd "build-mingw" && \
    CMAKE_CXX_COMPILER_LAUNCHER=ccache \
    cmake .. -GNinja \
        -DCMAKE_BUILD_TYPE={{release}} \
        -DCMAKE_SYSTEM_NAME=Windows \
        -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
        -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
        -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres \
        -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
        -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
        -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
        -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    cd "build-mingw" && ninja
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
    if ls build/libshaderc*.so* >/dev/null 2>&1; then cp -L build/libshaderc*.so* $DEB$LIBDIR; fi

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

# Test the wheel in a virtual machine
[macos]
testwheel vm_ip_address="":
    #!/usr/bin/env sh
    set -e
    IP="{{vm_ip_address}}"
    TMPDIR=/tmp/datoviz_example

    if [ -z "$IP" ]; then
        just wheel-check
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

api-json:
    @python tools/bindings/extract_api.py
#

ctypes: api-json
    @python tools/bindings/generate_ctypes.py
    @python tools/bindings/generate_array_facade.py
#

ctypes-abi: api-json
    @python tools/bindings/generate_ctypes_abi.py
#

ctypes-check: api-json ctypes-abi
    @python tools/bindings/generate_ctypes.py --check
    @python tools/bindings/generate_array_facade.py --check
    @python tools/bindings/validate_ctypes_policy.py
    @python tools/bindings/validate_array_facade.py
    @PYTHONPATH=. python tools/bindings/validate_ctypes_abi.py
#

ctypes-smoke:
    @PYTHONPATH=. python tools/bindings/ctypes_smoke.py
#

ctypes-facade-smoke:
    @PYTHONPATH=. python tools/bindings/array_facade_smoke.py
#

ctypes-python-smoke:
    @PYTHONPATH=. pytest -q testing/test_python_async_helpers.py testing/test_ctypes_raw_smoke.py testing/test_array_facade.py
#

ctypes-render-smoke:
    @PYTHONPATH=. python tools/bindings/ctypes_render_smoke.py
#

ctypes-editable-smoke: build ctypes
    @python tools/bindings/ctypes_package_smoke.py editable
#

ctypes-wheel-smoke: build ctypes
    @python tools/bindings/ctypes_package_smoke.py wheel
#

ctypes-package-smoke: ctypes-editable-smoke ctypes-wheel-smoke
#

c-integration-smoke:
    @tools/c_integration_smoke.sh
#

bindings: build ctypes ctypes-check ctypes-smoke ctypes-facade-smoke ctypes-python-smoke ctypes-render-smoke ctypes-package-smoke
#

pytest:
    @pytest tests.py
#

drp2-fixtures *args='':
    @python3 tools/drp2_fixture_runner.py {{args}}
#

webgpu-fixture-preflight *args='':
    @python3 tools/webgpu_fixture_preflight.py {{args}}
#

webgpu-runner-smoke:
    @node tools/webgpu_runner_smoke.mjs
#

webgpu-browser-smoke: wasm-scene-build
    @node tools/webgpu_browser_smoke.mjs
#

webgpu-gallery-check: wasm-scene-smoke webgpu-browser-smoke
#

wasm-env-check:
    #!/usr/bin/env bash
    set -euo pipefail
    export EMSDK_QUIET=1
    if ! command -v emcmake >/dev/null 2>&1; then
        if [ -n "${EMSDK_ENV_SH:-}" ] && [ -f "$EMSDK_ENV_SH" ]; then
            source "$EMSDK_ENV_SH"
        elif [ -n "${EMSDK:-}" ] && [ -f "$EMSDK/emsdk_env.sh" ]; then
            source "$EMSDK/emsdk_env.sh"
        elif [ -f "$PWD/../emsdk/emsdk_env.sh" ]; then
            source "$PWD/../emsdk/emsdk_env.sh"
        elif [ -f "$HOME/SDK/emsdk/emsdk_env.sh" ]; then
            source "$HOME/SDK/emsdk/emsdk_env.sh"
        elif [ -f "$HOME/emsdk/emsdk_env.sh" ]; then
            source "$HOME/emsdk/emsdk_env.sh"
        else
            echo "Emscripten not found."
            echo "Install/activate emsdk, put emcmake on PATH, or set EMSDK_ENV_SH=/path/to/emsdk_env.sh."
            exit 1
        fi
    fi
    echo "emcc: $(command -v emcc)"
    echo "emcmake: $(command -v emcmake)"
    emcc --version | head -n 1
#

_wasm-scene-build-mode build_dir build_type flag_config c_flags='' cxx_flags='' linker_flags='':
    #!/usr/bin/env bash
    set -euo pipefail
    export EMSDK_QUIET=1
    if ! command -v emcmake >/dev/null 2>&1; then
        if [ -n "${EMSDK_ENV_SH:-}" ] && [ -f "$EMSDK_ENV_SH" ]; then
            source "$EMSDK_ENV_SH"
        elif [ -n "${EMSDK:-}" ] && [ -f "$EMSDK/emsdk_env.sh" ]; then
            source "$EMSDK/emsdk_env.sh"
        elif [ -f "$PWD/../emsdk/emsdk_env.sh" ]; then
            source "$PWD/../emsdk/emsdk_env.sh"
        elif [ -f "$HOME/SDK/emsdk/emsdk_env.sh" ]; then
            source "$HOME/SDK/emsdk/emsdk_env.sh"
        elif [ -f "$HOME/emsdk/emsdk_env.sh" ]; then
            source "$HOME/emsdk/emsdk_env.sh"
        else
            echo "Emscripten not found."
            echo "Install/activate emsdk, put emcmake on PATH, or set EMSDK_ENV_SH=/path/to/emsdk_env.sh."
            exit 1
        fi
    fi
    cmake_args=(
        -S .
        -B "{{build_dir}}"
        -DCMAKE_BUILD_TYPE="{{build_type}}"
        -DCMAKE_SKIP_INSTALL_RULES=ON
        -DDVZ_BUILD_VK=OFF
        -DDVZ_BUILD_CANVAS=OFF
        -DDVZ_BUILD_APP=OFF
        -DDVZ_BUILD_GUI=OFF
        -DDVZ_WITH_FREETYPE=OFF
        -DDVZ_WITH_MSDF_ATLAS=OFF
        -DDVZ_ENABLE_KVAZAAR=OFF
        -DDVZ_WITH_GLFW=OFF
        -DDVZ_WITH_ZLIB=OFF
        -DDVZ_USE_MIMALLOC_RELEASE_DEFAULT=OFF
    )
    if [ -n "{{c_flags}}" ]; then
        cmake_args+=("-DCMAKE_C_FLAGS_{{flag_config}}={{c_flags}}")
    fi
    if [ -n "{{cxx_flags}}" ]; then
        cmake_args+=("-DCMAKE_CXX_FLAGS_{{flag_config}}={{cxx_flags}}")
    fi
    if [ -n "{{linker_flags}}" ]; then
        cmake_args+=("-DCMAKE_EXE_LINKER_FLAGS_{{flag_config}}={{linker_flags}}")
    fi
    emcmake cmake "${cmake_args[@]}"
    cmake --build "{{build_dir}}" --target datoviz_wasm_scene -j 8
#

wasm-scene-build:
    @just _wasm-scene-build-mode build-wasm-scene Release RELEASE
#

wasm-scene-build-debug:
    @just _wasm-scene-build-mode build-wasm-scene-debug Debug DEBUG \
        "-O0 -g3 -fno-omit-frame-pointer" \
        "-O0 -g3 -fno-omit-frame-pointer" \
        "-sASSERTIONS=2 -sSTACK_OVERFLOW_CHECK=2 -sSTACK_SIZE=1048576 -gsource-map --source-map-base=/"
#

wasm-scene-build-asan:
    @just _wasm-scene-build-mode build-wasm-scene-asan Debug DEBUG \
        "-O1 -g -fsanitize=address -fno-omit-frame-pointer" \
        "-O1 -g -fsanitize=address -fno-omit-frame-pointer" \
        "-fsanitize=address -sASSERTIONS=2 -sSTACK_OVERFLOW_CHECK=2 -sSTACK_SIZE=1048576 -gsource-map --source-map-base=/"
#

wasm-scene-build-safeheap:
    @just _wasm-scene-build-mode build-wasm-scene-safeheap Debug DEBUG \
        "-O1 -g -fno-omit-frame-pointer" \
        "-O1 -g -fno-omit-frame-pointer" \
        "-sASSERTIONS=2 -sSAFE_HEAP=1 -sSTACK_OVERFLOW_CHECK=2 -sSTACK_SIZE=1048576 -gsource-map --source-map-base=/"
#

wasm-scene-stack-usage:
    @just _wasm-scene-build-mode build-wasm-scene-stackusage Release RELEASE \
        "-O3 -fstack-usage -Wframe-larger-than=32768" \
        "-O3 -fstack-usage -Wframe-larger-than=32768"
    @find build-wasm-scene-stackusage -name '*.su' -print0 | \
        xargs -0 awk '{print $2 "\t" $1}' | sort -nr | head -n 30
#

wasm-scene-smoke: wasm-scene-build
    @node tools/wasm_scene_smoke.mjs
    @python3 tools/webgpu_fixture_preflight.py build-wasm-scene/wasm/wasm_api_scene_point_pixel_marker_segment_path_primitive_image_mesh_panzoom.json
    @python3 tools/webgpu_fixture_preflight.py build-wasm-scene/wasm/wasm_api_scene_sphere_textured_mesh3d_arcball.json
    @python3 tools/webgpu_fixture_preflight.py build-wasm-scene/wasm/wasm_api_scenario_timer_animation.json
    @node tools/webgpu_runner_smoke.mjs --streams-only \
        build-wasm-scene/wasm/wasm_api_scene_point_pixel_marker_segment_path_primitive_image_mesh_panzoom.json \
        build-wasm-scene/wasm/wasm_api_scene_sphere_textured_mesh3d_arcball.json \
        build-wasm-scene/wasm/wasm_api_scenario_timer_animation.json
#

shader-abi-check:
    @python3 tools/check_scene_shader_abi.py
#

spec-check: shader-abi-check
    #!/usr/bin/env bash
    set -euo pipefail

    tmpdir="$(mktemp -d)"
    trap 'rm -rf "$tmpdir"' EXIT

    names=()
    pids=()
    logs=()

    run_check() {
        local name="$1"
        shift
        local log="$tmpdir/${#names[@]}.log"
        names+=("$name")
        logs+=("$log")
        "$@" >"$log" 2>&1 &
        pids+=("$!")
    }

    run_check "drp2 fixture runner" .venv/bin/python tools/drp2_fixture_runner.py
    run_check "webgpu fixture preflight" .venv/bin/python tools/webgpu_fixture_preflight.py
    run_check "webgpu runner smoke" node tools/webgpu_runner_smoke.mjs
    run_check "drp2 fixture tests" .venv/bin/pytest -q testing/test_drp2_fixture_runner.py
    run_check "webgpu preflight tests" .venv/bin/pytest -q testing/test_webgpu_fixture_preflight.py
    run_check "scheduler tests" .venv/bin/pytest -q testing/test_dvztest_scheduler.py
    run_check "scene query source guard" .venv/bin/pytest -q testing/test_scene_query_source_guard.py
    run_check "scene architecture source guard" .venv/bin/pytest -q testing/test_scene_architecture_source_guard.py
    run_check "scene visual boundary guard" .venv/bin/python tools/check_scene_visual_boundaries.py

    status=0
    for i in "${!pids[@]}"; do
        if wait "${pids[$i]}"; then
            printf 'PASS %s\n' "${names[$i]}"
        else
            printf 'FAIL %s\n' "${names[$i]}"
            status=1
        fi
        cat "${logs[$i]}"
    done

    exit "$status"
#


# -------------------------------------------------------------------------------------------------
# Python packaging
# -------------------------------------------------------------------------------------------------

showwheel:
    @just wheel-inspect
#

wheel-matrix:
    @python tools/release_wheels/wheel_matrix.py
#

wheel-validate *args:
    @python tools/release_wheels/wheel_matrix.py --validate-dist {{args}}
#

wheel-stage *args:
    @python tools/release_wheels/stage_wheel.py {{args}}
#

wheel-build *args:
    @python tools/release_wheels/build_wheel.py {{args}}
#

wheel-inspect *args:
    @python tools/release_wheels/inspect_wheel.py {{args}}
#

wheel-check *args:
    @python tools/release_wheels/check_wheel.py {{args}}
#

release-source-bundle version:
    @python tools/release_source_bundle.py {{version}}
#

distribution-validate-local mode='all':
    @tools/validate_distribution_local.sh {{mode}}
#

wheel-ci-local platform_tag='' rebuild='0' render='0':
    #!/usr/bin/env sh
    set -e
    just wheel-matrix
    if [ "{{rebuild}}" = "1" ]; then
        just build
    fi
    just wheel-stage --clean
    if [ -n "{{platform_tag}}" ]; then
        just wheel-build --platform-tag "{{platform_tag}}"
    else
        just wheel-build
    fi
    just wheel-inspect
    if [ -n "{{platform_tag}}" ]; then
        just wheel-validate --platform-tag "{{platform_tag}}"
    fi
    just wheel-inspect --native-deps
    if [ "{{render}}" = "1" ]; then
        just wheel-check --shaderc --cmake-consumer --render --qt-probe optional
    else
        just wheel-check --shaderc --cmake-consumer --qt-probe optional
    fi
#

wheel-manylinux-docker arch='x86_64':
    #!/usr/bin/env sh
    set -e
    image="${DATOVIZ_MANYLINUX_IMAGE:-quay.io/pypa/manylinux_2_34_x86_64:latest}"
    case "{{arch}}" in
        x86_64) ;;
        *)
            echo "unsupported local manylinux Docker arch: {{arch}}" >&2
            echo "set DATOVIZ_MANYLINUX_IMAGE and extend this recipe after a native or emulated {{arch}} builder is proven" >&2
            exit 2
            ;;
    esac
    docker run --rm \
        -e DVZ_CMAKE_ARGS="${DVZ_CMAKE_ARGS:-}" \
        -e DVZ_WHEEL_RUNTIME_DIRS="${DVZ_WHEEL_RUNTIME_DIRS:-}" \
        -e DATOVIZ_MANYLINUX_PYTHON="${DATOVIZ_MANYLINUX_PYTHON:-}" \
        -e DATOVIZ_MANYLINUX_GENERATE_CTYPES="${DATOVIZ_MANYLINUX_GENERATE_CTYPES:-}" \
        -e DATOVIZ_HOST_UID="$(id -u)" \
        -e DATOVIZ_HOST_GID="$(id -g)" \
        -v "$PWD:/workspace" \
        -w /workspace \
        "$image" \
        bash tools/release_wheels/manylinux_build_inside.sh "{{arch}}"
#

wheel platform_tag='': build
    #!/usr/bin/env sh
    set -e
    just wheel-stage --clean
    if [ -n "{{platform_tag}}" ]; then
        just wheel-build --platform-tag "{{platform_tag}}"
    else
        just wheel-build
    fi
    just wheel-inspect
    if [ -n "{{platform_tag}}" ]; then
        just wheel-validate --platform-tag "{{platform_tag}}"
    fi
#

testpypi-check platform_tag dist_dir='dist':
    #!/usr/bin/env sh
    set -e
    just wheel-validate --dist-dir "{{dist_dir}}" --platform-tag "{{platform_tag}}"
    python -c "import twine" 2>/dev/null || {
        echo "twine is required: python -m pip install --upgrade twine" >&2
        exit 1
    }
    python -m twine check "{{dist_dir}}"/datoviz-*.whl
#

testpypi-check-all dist_dir='dist':
    #!/usr/bin/env sh
    set -e
    just wheel-validate --dist-dir "{{dist_dir}}"
    python -c "import twine" 2>/dev/null || {
        echo "twine is required: python -m pip install --upgrade twine" >&2
        exit 1
    }
    python -m twine check "{{dist_dir}}"/datoviz-*.whl
#

testpypi-upload platform_tag dist_dir='dist' confirm='no':
    #!/usr/bin/env sh
    set -e
    if [ "{{confirm}}" != "yes" ]; then
        echo "Refusing to upload to TestPyPI without confirm=yes" >&2
        echo "Run: just testpypi-upload {{platform_tag}} {{dist_dir}} yes" >&2
        exit 1
    fi
    just testpypi-check "{{platform_tag}}" "{{dist_dir}}"
    python -m twine upload --repository testpypi "{{dist_dir}}"/datoviz-*.whl
#

testpypi-upload-all dist_dir='dist' confirm='no':
    #!/usr/bin/env sh
    set -e
    if [ "{{confirm}}" != "yes" ]; then
        echo "Refusing to upload the full wheelhouse to TestPyPI without confirm=yes" >&2
        echo "Run: just testpypi-upload-all {{dist_dir}} yes" >&2
        exit 1
    fi
    just testpypi-check-all "{{dist_dir}}"
    python -m twine upload --repository testpypi "{{dist_dir}}"/datoviz-*.whl
#

pypi-install-smoke:
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
    just wheel-check $temp_dir/datoviz*.whl
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
    just wheel-check $temp_dir/datoviz*.whl
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
    just wheel-check $temp_dir/datoviz*.whl
    exit_code=$?
    rm -rf "${temp_dir}"
    exit $exit_code
#

[linux]
[macos]
symbols: api-json
    @jq -r '.functions[].name' {{justfile_directory()}}/build/bindings/datoviz_api.json > {{justfile_directory()}}/symbols.map
#

[windows]
symbols: api-json
    @jq -r ".functions[].name" "{{justfile_directory()}}\\build\\bindings\\datoviz_api.json" > "{{justfile_directory()}}\\symbols.map"
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

api: symbols ctypes doc tryimport # after every API update
#


# -------------------------------------------------------------------------------------------------
# Swiftshader
# -------------------------------------------------------------------------------------------------

[linux]
swiftshader +args:
    @icd="${VK_ICD_FILENAMES:-/usr/local/share/vulkan/icd.d/swiftshader_icd.json}"; \
    if [ ! -f "$icd" ]; then echo "SwiftShader ICD not found; set VK_ICD_FILENAMES or install SwiftShader outside data/." >&2; exit 1; fi; \
    VK_ICD_FILENAMES="$icd" {{args}}
#

[macos]
swiftshader +args:
    @icd="${VK_ICD_FILENAMES:-$HOME/.cache/datoviz/swiftshader/macos/vk_swiftshader_icd.json}"; \
    if [ ! -f "$icd" ]; then echo "SwiftShader ICD not found; set VK_ICD_FILENAMES or install SwiftShader outside data/." >&2; exit 1; fi; \
    VK_ICD_FILENAMES="$icd" {{args}}
#

[windows]
swiftshader +args:
    VK_ICD_FILENAMES=%VK_ICD_FILENAMES% \
    VK_LOADER_DEBUG=all \
    {{args}}
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
    tree -P '*.c' -P '*.h' -P '*.py' -P '*.cpp' -P '*.glsl' -P '*.vert' -P '*.frag' include src datoviz tests pytests examples cli
#

cloc:
    cloc include src datoviz tests pytests examples cli --quiet
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
# Static analysis
# -------------------------------------------------------------------------------------------------

analyze:
    @if [ ! -f build/compile_commands.json ]; then \
        echo "compile_commands.json not found. Run 'just build' first."; \
        exit 1; \
    fi
    @if command -v run-clang-tidy >/dev/null 2>&1; then \
        ROOT=$(pwd); \
        FILES=$(jq -r '.[].file' build/compile_commands.json \
            | grep -E "^$ROOT/(include|src|testing)/" \
            | grep -v '\.cu$' \
            | grep -v 'dvz_public_header_probe\.c$' \
            | tr '\n' ' '); \
        if [ -z "$FILES" ]; then \
            echo "No files matched for static analysis."; \
        else \
            run-clang-tidy -p build -quiet -header-filter="^$ROOT/(include|src|testing)/" $FILES; \
        fi; \
    else \
        echo "run-clang-tidy not found. Install clang-tidy or add it to PATH."; \
        exit 1; \
    fi
#

# -------------------------------------------------------------------------------------------------
# Examples
# -------------------------------------------------------------------------------------------------

example-c name *args: build
    #!/usr/bin/env bash
    set -euo pipefail
    if [[ "$(uname)" == "Darwin" ]]; then
        vk_lib="${VULKAN_SDK:-}/lib"
        if [[ -d "${vk_lib}" ]]; then
            fallback_path="${vk_lib}${DYLD_FALLBACK_LIBRARY_PATH:+:${DYLD_FALLBACK_LIBRARY_PATH}}"
            export DYLD_FALLBACK_LIBRARY_PATH="${fallback_path}"
        fi
    fi
    exe="./build/examples/c/{{name}}"
    if [[ ! -x "${exe}" && "{{name}}" != */* ]]; then
        matches=(./build/examples/c/*/{{name}})
        if [[ ${#matches[@]} -eq 1 && -x "${matches[0]}" ]]; then
            exe="${matches[0]}"
        fi
    fi
    dvzr_path=""
    if [[ "{{name}}" == "visuals/mesh" ]]; then
        if [[ " {{args}} " =~ (^|[[:space:]])record=([^[:space:]]+) ]]; then
            dvzr_path="${BASH_REMATCH[2]}"
        elif [[ " {{args}} " =~ (^|[[:space:]])--record[[:space:]]+([^[:space:]]+) ]]; then
            dvzr_path="${BASH_REMATCH[2]}"
        elif [[ " {{args}} " == *" record "* || " {{args}} " == "record" ]]; then
            dvzr_path="./build/examples/c/visuals/mesh.dvzr"
        fi
        if [[ -n "${dvzr_path}" ]]; then
            rm -rf -- "${dvzr_path}"
        fi
    fi
    "${exe}" {{args}}
    if [[ "{{name}}" == "visuals/mesh" ]]; then
        if [[ -n "${dvzr_path}" ]]; then
            python3 tools/dvzr_to_webgpu_stream.py \
                "${dvzr_path}" \
                examples/webgpu/streams/mesh_dvzr_wgsl.json
        fi
    fi
#

# Tests
# -------------------------------------------------------------------------------------------------

[linux]
test test_name="":
    #!/usr/bin/env bash
    set -euo pipefail
    ./build/testing/dvztest {{test_name}}
#

[linux]
atest test_name="": asan
    #!/usr/bin/env bash
    set -euo pipefail
    build_dir="$(pwd)/build-asan"
    # NOTE: we create build-asan/lsan.supp by copying parts of sanitizers/asan.ignore into it
    suppressions_file="${build_dir}/lsan.supp"
    mkdir -p "${build_dir}"
    grep '^leak:' sanitizers/asan.ignore > "${suppressions_file}" || true
    ASAN_OPTIONS="halt_on_error=1:detect_stack_use_after_return=1:strict_init_order=1:alloc_dealloc_mismatch=1:detect_invalid_pointer_pairs=1:malloc_context_size=20:verbosity=1" LSAN_OPTIONS="suppressions=${suppressions_file}${LSAN_OPTIONS:+:${LSAN_OPTIONS}}" ./build-asan/testing/dvztest {{test_name}}
#

[linux]
coverage filter="":
    @set -e
    @if ! command -v gcovr >/dev/null 2>&1; then \
        echo "gcovr is required for coverage reporting. Install it with 'pip install gcovr'."; \
        exit 1; \
    fi
    @rm -rf build-coverage
    @mkdir -p docs/images
    @mkdir -p build-coverage/coverage
    @ROOT=$(pwd) && cd build-coverage && cmake .. -GNinja -DCMAKE_BUILD_TYPE=Debug -DDVZ_ENABLE_COVERAGE=ON -DDVZ_ENABLE_ASAN_IN_DEBUG=OFF -DDVZ_ENABLE_CUDA=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_C_COMPILER_LAUNCHER= -DCMAKE_CXX_COMPILER_LAUNCHER=
    @cd build-coverage && ninja
    @cd build-coverage && if [ -n "{{filter}}" ]; then ./testing/dvztest "{{filter}}"; else ./testing/dvztest; fi
    @ROOT=$(pwd) && cd build-coverage && gcovr --root "$ROOT" --filter "$ROOT/src" --filter "$ROOT/include" --exclude "$ROOT/external" --exclude "$ROOT/testing" --exclude "$ROOT/v0.3" --exclude "$ROOT/build" --txt-metric branch --print-summary
    @ROOT=$(pwd) && cd build-coverage && gcovr --root "$ROOT" --filter "$ROOT/src" --filter "$ROOT/include" --exclude "$ROOT/external" --exclude "$ROOT/testing" --exclude "$ROOT/v0.3" --exclude "$ROOT/build" --html --html-details -o coverage/index.html
    @echo "Coverage HTML report: build-coverage/coverage/index.html"
#

[linux]
mtest test_name="": msan
    #!/usr/bin/env bash
    set -euo pipefail
    MSAN_OPTIONS="halt_on_error=0:exit_code=0:print_stats=0:symbolize=1:abort_on_error=0" \
    ./build-msan/testing/dvztest {{test_name}} 2> >(awk -f tools/hide-msan.awk >&2)
#

[linux]
ttest test_name="": tsan
    #!/usr/bin/env bash
    set -euo pipefail
    TSAN_OPTIONS="ignore_noninstrumented_modules=1:verbosity=0" ./build-tsan/testing/dvztest {{test_name}}
#

[macos]
test test_name="":
    ./build/testing/dvztest {{test_name}}
#

[macos]
test-inventory lane="":
    #!/usr/bin/env bash
    set -euo pipefail
    if [[ -n "{{lane}}" ]]; then
        python3 tools/test_inventory.py \
            --lane "{{lane}}" \
            --output "build/testing/test_inventory_{{lane}}.json" \
            --markdown "build/testing/test_inventory_{{lane}}.md"
    else
        python3 tools/test_inventory.py \
            --output build/testing/test_inventory.json \
            --markdown build/testing/test_inventory.md
    fi
#

[macos]
test-lane lane *args:
    #!/usr/bin/env bash
    set -euo pipefail
    case_list="build/testing/test_lane_{{lane}}.txt"
    python3 tools/test_inventory.py \
        --lane "{{lane}}" \
        --output "build/testing/test_inventory_{{lane}}.json" \
        --markdown "build/testing/test_inventory_{{lane}}.md" \
        --case-list "${case_list}"
    ./build/testing/dvztest --case-list "${case_list}" {{args}}
#

[macos]
test-fast *args:
    just test-lane fast-cpu {{args}}
#

[macos]
test-scene-cpu *args:
    just test-lane scene-semantic {{args}}
#

[macos]
test-drp2-contract *args:
    just test-lane drp2-contract {{args}}
#

[macos]
test-runtime-vklite *args:
    just test-lane runtime-vklite {{args}}
#

[macos]
test-render-smoke *args:
    just test-lane render-smoke {{args}}
#

[macos]
test-render-conformance *args:
    just test-lane render-conformance {{args}}
#

[macos]
test-slow *args:
    just test-lane slow-churn {{args}}
#

[linux]
[macos]
shaderc-smoke:
    #!/usr/bin/env bash
    set -euo pipefail
    cmake --build build --target dvz_shaderc_smoke
    ./build/testing/dvz_shaderc_smoke
#

[windows]
test test_name="":
    ./build/testing/dvztest.exe {{test_name}}
#

[linux]
canvas *args:
    ./build/testing/dvz_live_canvas {{args}}
#

[linux]
profile-canvas *args:
    just build-profile
    scripts/profile_live_canvas.sh --bin ./build-profile/testing/dvz_live_canvas {{args}}
#

[linux]
profile-canvas-release *args:
    just build-profile
    scripts/profile_live_canvas.sh --bin ./build-profile/testing/dvz_live_canvas {{args}}
#

[linux]
memory-canvas-release *args:
    just build-profile
    scripts/memory_live_canvas.sh --bin ./build-profile/testing/dvz_live_canvas {{args}}
#

[macos]
canvas *args:
    ./build/testing/dvz_live_canvas {{args}}
#

[windows]
canvas *args:
    ./build/testing/dvz_live_canvas.exe {{args}}
#


# -------------------------------------------------------------------------------------------------
# Info
# -------------------------------------------------------------------------------------------------

# [unix]
# info:
#     @./build/datoviz info
# #

# [windows]
# info:
#     @build/datoviz.exe info
# #

[linux]
exec arg:
    @LD_LIBRARY_PATH=build/ python3 -c "import ctypes; print(ctypes.cdll.LoadLibrary('libdatoviz.so').{{arg}}())"
#



# -------------------------------------------------------------------------------------------------
# Demo
# -------------------------------------------------------------------------------------------------

# demo:
#     ./build/datoviz demo
# #

[linux]
pydemo_dll:
    @LD_LIBRARY_PATH=build/ python3 -c "import ctypes; ctypes.cdll.LoadLibrary('libdatoviz.so').dvz_demo()"
#

[macos]
pydemo_dll:
    @DYLD_LIBRARY_PATH=build/ python3 -c "import ctypes; ctypes.cdll.LoadLibrary('libdatoviz.dylib').dvz_demo()"
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

# [windows]
# runexample name="":
#     ./build/example_{{name}}.exe
# #

# example name="":
#     gcc -o build/example_{{name}} examples/c/<group>/{{name}}.c -Iinclude/ -Lbuild/ -Wl,-rpath,build -lm -ldatoviz
#     just runexample {{name}}
# #

# Run C examples sequentially for manual regression checks.
[positional-arguments]
examples *args: build
    @python3 tools/run_c_examples.py "$@"

# Smoke-test fenced code blocks in documentation markdown files.
# Prerequisites: build (for C) and ctypes (for Python array facade).
doctest lang="both":
    @python3 tools/doctest.py --lang {{lang}} docs/index.md docs/start/quickstart.md

# Run all Python examples and generate screenshots in data/gallery/.
gallery-screenshots filter="": && gallery
    @echo "Generating screenshots from examples..."
    @python tools/build_screenshots.py {{filter}}


# Capture v0.4 C gallery screenshots into data/gallery/v0.4/.
[positional-arguments]
capture-gallery *args: build
    @python3 tools/capture_gallery.py "$@"


# Refresh v0.4 gallery screenshots and generated docs with cached native captures.
[positional-arguments]
gallery-refresh *args: build
    @python3 tools/capture_gallery.py --all-screenshot --cache --jobs auto "$@"
    @python tools/build_gallery.py
    @python3 tools/build_examples_manifest.py
    @python3 tools/build_capabilities.py
    @python3 tools/check_gallery_media.py
    @git diff --check


# Check that gallery screenshots exist, are nonblank, and match cached fingerprints.
[positional-arguments]
check-gallery-media *args:
    @python3 tools/check_gallery_media.py "$@"


# Generate build-local WebP derivatives for gallery screenshots.
[positional-arguments]
gallery-webp *args:
    @python3 tools/build_gallery_webp.py "$@"


# Capture screenshots for the Start Here / Get Started documentation pages.
capture-start: build
    @python3 tools/capture_gallery.py --lane start
    @python3 tools/build_gallery_webp.py --lane start


# Capture the native C screenshots used by the local v0.4 landing prototype.
capture-landing: build
    @python3 tools/capture_gallery.py --landing


# Build the gallery Markdown files and machine-readable example manifests.
gallery:
    @echo "Generating build-local gallery WebP assets..."
    @python3 tools/build_gallery_webp.py
    @echo "Generating the gallery Markdown files..."
    @python tools/build_gallery.py
    @echo "Generating the public examples manifest..."
    @python3 tools/build_examples_manifest.py
    @echo "Generating the public capabilities manifest..."
    @python3 tools/build_capabilities.py

check-example-manifests:
    @python3 tools/check_example_manifests.py


# -------------------------------------------------------------------------------------------------
# Documentation
# -------------------------------------------------------------------------------------------------

doc: api-docs #gallery #headers
#

api-docs:
    @python3 tools/build_api_c.py
#

check-api-docs:
    @python3 tools/build_api_c.py --check
#

docs-assets:
    @python3 tools/build_gallery_webp.py --require-image-dir --quiet-missing
#

serve: docs-assets
    #!/usr/bin/env bash
    set -euo pipefail

    host="${DATOVIZ_DOCS_HOST:-localhost}"
    start_port="${DATOVIZ_DOCS_PORT:-8294}"
    end_port=$((start_port + 20))

    port=$(python3 - "$host" "$start_port" "$end_port" <<'PY'
    import socket
    import sys

    host = sys.argv[1]
    start_port = int(sys.argv[2])
    end_port = int(sys.argv[3])

    for port in range(start_port, end_port + 1):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            try:
                sock.bind((host, port))
            except OSError:
                continue
        print(port)
        break
    else:
        sys.exit(f"no free port found on {host}:{start_port}-{end_port}")
    PY
    )

    if [ "$port" != "$start_port" ]; then
        echo "Port ${host}:${start_port} is busy; serving docs on ${host}:${port}."
    fi

    uv run --with mkdocs-material --with 'mkdocstrings[python]' --with pillow mkdocs serve -a "${host}:${port}"
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
    @rm -rf build*
#

rebuild:
    just clean
    just build || just build
#

rmbuild:
    @rm -rf build/spirv build/artifacts build/struct_sizes* build/*.dylib* build/*.so* build/*.dll build/datoviz*
#
