# Building instructions

If packages are not available on your system, you can build Datoviz yourself.

**Note:** Datoviz is currently built and tested with the **Vulkan LunarG SDK v1.3.280** (you normally don't need to install it to build Datoviz on Linux and macOS, only on Windows). We'll regularly update this version.

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
* [vcpkg](https://vcpkg.io/en/) The `VCPKG_ROOT` environment variable should be set and should be in the `PATH`.
* [just](https://github.com/casey/just/releases) Extract the just.exe file into C:\mingw64\bin (created by WinLibs).
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
