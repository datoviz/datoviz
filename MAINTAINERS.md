# Maintainers instructions

## Release checklist for Datoviz maintainers

Development happens on `dev` whereas `main` is stable.

Release checklist from a Linux development machine:

1. `git branch`: check that you are on the `dev` branch.
1. Write the `CHANGELOG.md` for the new version.
1. `just api clean release`: rebuild in release mode.
1. `just test`: run the C testing suite.
1. `just pytest`: run the Python testing suite.
1. Run the last three commands on Windows and macOS too.
1. `just act test-linux`: simulate the GitHub Actions tests locally.
1. `version=x.y.z`: set up the new version.
1. `just bump $version`: bump the codebase to the new version.
1. `just release`: recompile with the new version.
1. `git diff`: check the changes to commit.
1. `git commit -am "Bump version to v$version" && git push`: commit the new version.
1. `just wheels`: build the wheels on GitHub Actions.
1. Wait until the [wheels have been successfully built on all supported platforms](https://github.com/datoviz/datoviz/actions/workflows/wheels.yml). **This will take about 15 minutes** (the Windows build is currently much longer than macOS and Linux builds because GitHub Actions do not support Windows Docker containers yet).
1. `just checkartifact`: once the wheels have been built, test them on different computers/operating systems (Linux, macOS, Windows if possible).
1. `git checkout main && git merge dev`: merge `dev` to `main` and switch to `main`.
1. `just tag $version`: once on `main`, tag with the new version.
1. `git push origin --tags`: push the tag.
1. `just draft`: create a new GitHub Release draft with the built wheels.
1. Edit and publish the [GitHub Release](https://github.com/datoviz/datoviz/releases).
1. `just upload`: upload the wheels to PyPI.
1. `just bump a.b.c-dev`: bump to the new development version (replace with the next expected version number).
1. `git commit -am "Bump to development version" && git push`: bump to the development version.
1. Announce the new release on the various communication channels.


## Packaging instructions (advanced users)

This section provides instructions for Datoviz maintainers who'd like to create binary packages and Python wheels.


### Ubuntu 24.04

Requirements:

* Docker
* [just](https://github.com/casey/just/releases)
* `sudo apt-get install dpkg-dev fakeroot nvidia-container-toolkit`

To build a release binary, see the [build instructions](BUILD.md):

```bash
just pydev
just release
```

To build a `.deb` Debian installable package for development (with C headers and shared libraries):

```bash
just deb
```

To test the `.deb` package in an isolated Docker container:

```bash
just testdeb
```

To build a `manylinux` wheel (using `manylinux_2_28_x86_64`, based on AlmaLinux 8):

```bash
# Build Datoviz in the manylinux container.
just buildmany

# Build a Python wheel in that container (saved in dist/).
just wheelmany
```

To test the `manylinux` wheel:

```bash
just testwheel
```


### macOS (arm64)

Requirements:

* Homebrew
* [just](https://github.com/casey/just/releases)

To build a release binary, see the [build instructions](BUILD.md):

```bash
just pydev
just release
```

To build a `.pkg` macOS installable package for development (with C headers and shared libraries):

```bash
just pkg
```

To build a macOS Python wheel:

```bash
just wheel
```

To test the macOS package in an isolated environment:

1. Install sshpass:

    ```bash
    brew install sshpass
    ```

2. Install [UTM](https://mac.getutm.app/).
3. Create a new macOS virtual machine (VM) with **at least 64 GB storage** (for Xcode).
4. Install macOS in the virtual machine. For simplicity, use your `$USER` as the login and password.
5. Once installed, find the IP address in the VM macOS system preferences and write it down (for example, `192.168.64.4`).
6. Set up remote access via SSH in the VM macOS system preferences to set up a SSH server.
7. Open a terminal in the VM and type:

    ```bash
    type: xcode-select --install
    ```

Go back to the host machine and type:

```bash
# Test the .pkg installation in an UTM virtual machine, using the IP address you wrote down earlier.
just testpkg 192.168.64.4
```

The virtual machine should show the Datoviz demo in a window.

To test the macOS wheel, you can either test in a virtual Python environment, or in a virtual machine using UTM.

To test the macOS wheel in a virtual Python environment:

```bash
just testwheel
```

To test the macOS wheel in a virtual machine, set up the virtual machine as indicated above, then run (replacing the IP address with your virtual machine's IP):

```bash
just testwheel 192.168.64.4
```


### macOS (Intel x86-64)

Requirements:

* Homebrew
* [just](https://github.com/casey/just/releases)

To build a release binary, see the [build instructions](BUILD.md):

```bash
just pydev
just release
```

To build a `.pkg` macOS installable package for development (with C headers and shared libraries):

```bash
just pkg
```

To build a macOS Python wheel:

```bash
just wheel
```


### Windows

Requirements:

* [Git for Windows](https://git-scm.com/download/win)
* [WinLibs](https://winlibs.com/)
* [just](https://github.com/casey/just/releases)
* [LunarG Vulkan SDK for Windows](https://vulkan.lunarg.com/sdk/home#windows)
* [WSL2](https://learn.microsoft.com/en-us/windows/wsl/install)
* [vcpkg](https://vcpkg.io/en/)

To build a release binary, see the [build instructions](BUILD.md):

```bash
just release
```

To build a Windows Python wheel, open a Git Bash and type:

```bash
# see https://stackoverflow.com/a/36530750/1595060
echo "alias python='winpty python.exe'" >> ~/.bash_profile
just pydev
just wheel
```

To test the wheel in a Python virtual environment:

```bash
just testwheel
```


## GitHub Actions notes

Datoviz relies on GitHub Actions for cross-platform automated testing and wheel building.

### Testing

[This workflow is defined in `test.yml`](.github/workflows/test.yml)

#### Linux

The `test-linux` job relies on the custom Docker image [`rossant/datoviz_ubuntu`](https://hub.docker.com/repository/docker/rossant/datoviz_ubuntu/) (see the [Dockerfile](docker/Dockerfile_ubuntu)).
This image has all build and run dependencies, as well as the Swiftshader software Vulkan renderer, and xvfb to run graphical applications on a headless server.

#### macOS

Docker seems to be not supported on GitHub Actions macOS servers. The `test_macos` job installs build dependencies with Homebrew, Python dependencies, it builds Datoviz and it run the test suite.

Note that there is a workaround to remove Mono's freetype library which conflicts with Homebrew's one.

#### Windows

There is a custom Docker image [`rossant/datoviz_windows`](https://hub.docker.com/repository/docker/rossant/datoviz_windows/) which is however unused at the moment because GitHub Actions Windows servers to not seem to support custom Docker images at the moment.

Instead, the job installs dependencies with Chocolatey, vcpkg, the Vulkan SDK, and it extracts the Swiftshader dynamic library.
Note that the Swiftshader Windows library is very large (>100 MB) so it is stored as a compressed zip file in the [`datoviz/data` submodule](https://github.com/datoviz/data).


### Wheel building

[This workflow is defined in `wheels.yml`](.github/workflows/wheels.yml)

For each supported platform, this workflow builds the library in release mode, builds the wheel, renames it for the current platform, and uploads it as a GitHub Actions build artifact.

Refer to the Testing workflow for more information about the building process, which is mostly replicated in this workflow.

#### Linux notes

For improved compatibility with Linux Python wheels uploaded to PyPI, it is necessary to build Datoviz on a particular Linux distribution based on [AlmaLinux](https://en.wikipedia.org/wiki/AlmaLinux) (based on Red Hat Enterprise Linux, REHL).

There is a custom Docker image [`rossant/datoviz_manylinux`](https://hub.docker.com/repository/docker/rossant/datoviz_manylinux/) based on `quay.io/pypa/manylinux_2_28_x86_64` (see the [Dockerfile](docker/Dockerfile_manylinux)) with all build dependencies.
It also has Swiftshader compiled for this platform.

#### macOS notes

There are two separate jobs for x86_64 and amd64 architectures.
