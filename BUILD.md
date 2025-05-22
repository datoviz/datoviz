# Building instructions

If precompiled pip wheels are unavailable or not working on your system—or if you need a custom build (e.g. to create a C/C++ application that depends on Datoviz)—you can build Datoviz manually.

!!! note

    Datoviz is currently built and tested with the **Vulkan LunarG SDK v1.3.280**.
    On Linux and macOS, this SDK is bundled automatically and does not need to be installed separately. On Windows, however, manual installation is required.
    We regularly update the supported Vulkan SDK version.


## Dependencies

Versions of the various dependencies, including the bundled ones.

* **Datoviz v0.2.2**
    * Dear ImGui: `v1.91.7-docking`
    * Vulkan SDK:
    * (TODO)

## Ubuntu 24.04

```bash
# Install the build and system dependencies.
sudo apt install build-essential cmake gcc ccache ninja-build xorg-dev clang-format patchelf tree libtinyxml2-dev libfreetype-dev

# Install just, see https://github.com/casey/just
curl --proto '=https' --tlsv1.2 -sSf https://just.systems/install.sh | bash

# Clone the Datoviz repo and build.
git clone https://github.com/datoviz/datoviz.git --recursive
cd datoviz

# Build Python requirements
pip install -r requirements-dev.txt

# NOTE: this call will fail, but the build will succeed the second time.
# Fix welcome (see https://github.com/Chlumsky/msdf-atlas-gen/issues/98)
just build

# That one should succeed.
just build

# Try a demo.
just demo

# Compile and run a C example.
just example scatter

# Run the demo from Python.
python -c "import datoviz; datoviz.demo()"
```


## macOS (arm64)

```bash
# Install brew if you don't have it already.
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
(echo; echo 'eval "$(/opt/homebrew/bin/brew shellenv)"') >> ~/.zprofile
    eval "$(/opt/homebrew/bin/brew shellenv)"

# Install build dependencies.
brew install just cmake ccache ninja freetype clang-format tree cloc jq

# Clone the Datoviz repo.
git clone https://github.com/datoviz/datoviz.git --recursive
cd datoviz

# Build Python requirements
pip install -r requirements-dev.txt

# NOTE: this call will fail, but the build will succeed the second time.
# Fix welcome (see https://github.com/Chlumsky/msdf-atlas-gen/issues/98)
just build

# That one should succeed.
just build

# Try a demo.
just demo

# Compile and run a C example.
just example scatter

# Run the demo from Python.
python -c "import datoviz; datoviz.demo()"
```


## macOS (Intel x86-64)

```bash
# Install brew if you don't have it already.
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
(echo; echo 'eval "$(/opt/homebrew/bin/brew shellenv)"') >> ~/.zprofile
    eval "$(/opt/homebrew/bin/brew shellenv)"

# Install build dependencies.
brew install just cmake ccache ninja freetype clang-format tree cloc jq

# Clone the Datoviz repo.
git clone https://github.com/datoviz/datoviz.git --recursive
cd datoviz

# Build Python requirements
pip install -r requirements-dev.txt

# NOTE: this call will fail, but the build will succeed the second time.
# Fix welcome (see https://github.com/Chlumsky/msdf-atlas-gen/issues/98)
just build

# That one should succeed.
just build

# Try a demo.
just demo

# Compile and run a C example.
just example scatter

# Run the demo from Python.
python -c "import datoviz; datoviz.demo()"
```


## Windows

Requirements:

* [Git for Windows](https://git-scm.com/download/win).
* [WinLibs](https://winlibs.com/): Download and install the latest gcc UCRT version with POSIX threads.
* [LunarG Vulkan SDK for Windows](https://vulkan.lunarg.com/sdk/home#windows).
* [vcpkg](https://vcpkg.io/en/): The `VCPKG_ROOT` environment variable should be set and should be in the `PATH`.
* [just](https://github.com/casey/just/releases): Extract the just.exe file into C:\mingw64\bin (created by WinLibs).`
* [jq](https://jqlang.github.io/jq/download/): For example, with winget, use `winget install jqlang.jq`
* [Python](https://www.python.org/downloads).


Instructions:

1. Install the above dependencies.
2. Open Windows git-bash terminal at the directory location for the datoviz build.

```bash
# Clone the Datoviz repo.
git clone https://github.com/datoviz/datoviz.git --recursive
cd datoviz

# Build Python requirements
pip install -r requirements-dev.txt

# NOTE: this call will fail, but the build will succeed the second time.
# Fix welcome (see https://github.com/Chlumsky/msdf-atlas-gen/issues/98)
just build

# That one should succeed.
just build

# Try a demo.
just demo

# Compile and run a C example.
just example scatter

# Run the demo from Python.
python -c "import datoviz; datoviz.demo()"
```
