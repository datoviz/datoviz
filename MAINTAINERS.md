# Maintainers instructions

## Packaging

This section provides instructions for maintainers who need to create binary packages and Python wheels.


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


## Release checklist

Development happens on `dev` whereas `main` is stable.

1. While on `dev`, build in release mode with `just release`.
2. Run the C testing suite with `just test`.
3. Run the Python testing suite with `just pytest`.
4. Run GitHub Actions tests locally with `just act`.
5. Write the `CHANGELOG.md`.
6. Bump to the new version with `just bump x.y.z`.
7. Merge `dev` to `main` and switch to `main`.
8. Once on `main`, tag with the new version.
9. Build and test packages (until this is done by CI/CD):
   1. On a Linux computer:
      * `just release`
      * `just deb`
      * `just testdeb`
      * `just manylinux`
      * `just testwheel`
      * Wheel is in `dist/`
   2. On macOS ARM & Intel:
      * `just release`
      * `just pkg`
      * `just wheel`
      * `just testwheel`
      * Wheel is in `dist/`
   3. On: Windows
      * `just release`
      * `just wheel`
      * `just testwheel`
      * Wheel is in `dist/`
10. Upload these packages to GitHub and PyPI.
11. Bump to the new development version with `just bump a.b.c-dev`.
12. New release announcement.
