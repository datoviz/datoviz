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
