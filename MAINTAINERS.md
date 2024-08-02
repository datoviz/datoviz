# Maintainers instructions

## Packaging instructions

This section provides instructions for maintainers who need to create binary packages.

### Ubuntu 24.04

You need to install Docker to test the created deb package in an isolated virtual environment.

```bash
sudo apt-get install dpkg-dev fakeroot nvidia-container-toolkit

# Generate a .deb package in packaging/
just deb

# Test .deb installation in a Docker container
just testdeb
```

### macOS (arm64)

Building a `.pkg` package file with Datoviz and its dependencies is straightforward with the `just pkg` command on macOS (arm64).

#### Preparing a virtual machine for testing in an isolated environment

However, testing this package file in a virtual machine is currently more complicated that on Linux.
Before calling `just testpkg`, you need to follow several steps to prepare a virtual machine manually.

1.  Install sshpass:

    ```bash
    brew install sshpass
    ```

2. Install [UTM](https://mac.getutm.app/).
3. Create a new macOS virtual machine (VM) with **at least 64 GB storage** (for Xcode).
4. Install macOS in the virtual machine. For simplicity, use your $USER as the login and password.
5. Once installed, find the IP address in the VM macOS system preferences and write it down (for example, 192.168.64.4).
6. Set up remote access via SSH in the VM macOS system preferences to set up a SSH server.
7. Open a terminal in the VM and type:

    ```bash
    type: xcode-select --install
    ```

#### Build and test the macOS package

Go back to the host machine and type:

```bash
# Generate a .pkg package in packaging/
just pkg

# Test the .pkg installation in an UTM virtual machine, using the IP address you wrote down earlier.
just testpkg 192.168.64.4

# Go to the virtual machine, and run in a terminal `/tmp/datoviz_example/example_scatter`.
```


### Windows

1. Build Datoviz (see [build instructions](BUILD.md)).
2. Open a Git Bash.
3. Type: `echo "alias python='winpty python.exe'" >> ~/.bash_profile` ([see here](https://stackoverflow.com/a/36530750/1595060)).
4. Type: `pip install -r requirements-dev.txt`.
5. Type: `just wheel`.



<!-- PYTHON PACKAGING -->

## Python packaging instructions

This section provides instructions for maintainers who want to create Python wheel packages.

### Ubuntu 24.04

You need to install Docker to generate the wheels on a manylinux container. We have only used `manylinux_2_28_x86_64` so far (based on AlmaLinux 8).

```bash
# First, build Datoviz in the manylinux container.
just buildmany

# Then, build a Python wheel in a manylinux container (saved in dist/).
just wheelmany

# Show the contents of the wheel.
just showwheel

# Try installing and running the wheel in an Ubuntu container.
just testwheel
```

### macOS (arm46)

TODO.


### Windows

TODO.



## Release checklist

* Build in release mode with `just release`
* Run the C testing suite
* Run the Python testing suite
* Write the changelog
* Bump to the new version with `just bump x.y.z`
* Commit and tag
* Build packages
* Upload packages
* Bump to the new development version with `just bump a.b.c-dev`
* Announcement
