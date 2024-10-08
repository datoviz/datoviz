# Maintainers instructions

## Release checklist for Datoviz maintainers

Development happens on `dev` whereas `main` is stable.

Release checklist from a Linux development machine:

1. `git branch`: check that you are on the `dev` branch.
2. Write the `CHANGELOG.md` for the new version.
3. `just clean release api`: rebuild in release mode.
4. `just test`: run the C testing suite.
5. `just pytest`: run the Python testing suite.
6. `just act test-linux`: simulate the GitHub Actions tests locally.
7. `version=x.y.z`: set up the new version.
8. `just bump $version`: bump the codebase to the new version.
9. `just release`: recompile with the new version.
10. `git diff`: check the changes to commit.
11. `git commit -am "Bump version to v$version" && git push`: commit the new version.
12. `just wheels`: build the wheels on GitHub Actions.
13. Wait until the [wheels have been successfully built on all supported platforms](https://github.com/datoviz/datoviz/actions/workflows/wheels.yml). **This will take about 15 minutes** (the Windows build is currently much longer than macOS and Linux builds because GitHub Actions do not support Windows Docker containers yet).
14. `just checkartifact`: once the wheels have been built, test them on different computers/operating systems (Linux, macOS, Windows if possible).
15. `git checkout main && git merge dev`: merge `dev` to `main` and switch to `main`.
16. `just tag $version`: once on `main`, tag with the new version.
17. `git push origin --tags`: push the tag.
18. `just draft`: create a new GitHub Release draft with the built wheels.
19. Edit and publish the [GitHub Release](https://github.com/datoviz/datoviz/releases).
20. `just upload`: upload the wheels to PyPI.
21. `just bump a.b.c-dev`: bump to the new development version (replace with the next expected version number).
22. `git commit -am "Bump to development version" && git push`: bump to the development version.
23. Announce the new release on the various communication channels.


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

