# Building instructions

If packages are not available on your system, you can build Datoviz yourself.

## Ubuntu 24.04

```bash
# Install the build and system dependencies.
sudo apt install build-essential cmake gcc ccache ninja-build xorg-dev clang-format patchelf tree libtinyxml2-dev libfreetype-dev

# Install just, see https://github.com/casey/just
curl --proto '=https' --tlsv1.2 -sSf https://just.systems/install.sh | bash

# Clone the Datoviz repo and build.
git clone https://github.com/datoviz/datoviz.git@v0.2x --recursive
cd datoviz

# Build Python requirements
pip install -r requirements-dev.txt

# This call will fail, but the build will succeed the second time.
# Fix welcome (see https://github.com/Chlumsky/msdf-atlas-gen/issues/98)
just build

# That one should succeed.
just build

# Compile and run an example.
just example scatter
```


## macOS (arm64)

```bash
# Install brew if you don't have it already.
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
(echo; echo 'eval "$(/opt/homebrew/bin/brew shellenv)"') >> /Users/cyrille/.zprofile
    eval "$(/opt/homebrew/bin/brew shellenv)"

# Install dependencies.
brew install just cmake ccache ninja freetype clang-format tree

# Clone the Datoviz repo.
git clone https://github.com/datoviz/datoviz.git@v0.2x --recursive
cd datoviz

# Build Python requirements
pip install -r requirements-dev.txt

# This call will fail, but the build will succeed the second time.
# Fix welcome (see https://github.com/Chlumsky/msdf-atlas-gen/issues/98)
just build

# That one should succeed.
just build

# Compile and run an example.
just example scatter
```

## Windows

_Note_: we have less experience with Windows, improvements welcome.

Requirements:

* [Git for Windows](https://git-scm.com/download/win).
* [WinLibs](https://winlibs.com/): download and install the latest gcc UCRT version with POSIX threads..
* [just](https://github.com/casey/just/releases).
* [LunarG Vulkan SDK for Windows](https://vulkan.lunarg.com/sdk/home#windows).
* [WSL2](https://learn.microsoft.com/en-us/windows/wsl/install).
* [vcpkg](https://vcpkg.io/en/). The `VCPKG_ROOT` environment variable should be set and should be in the `PATH`.
* Python (e.g. conda).

Instructions:

1. Copy the decompressed `just.exe` into `C:\mingw64\bin` (which should have been created by WinLibs).
2. Open a Git Bash terminal.
3. Add this to your `~/.bash_profile`: `export VCPKG_ROOT=/path/to/vcpkg` after putting the path to vcpkg.
4. Clone the Datoviz GitHub repository in a folder.
5. Go to that folder in the terminal.
6. Type:

```bash
wsl.exe --install
wsl.exe --update
pip install -r requirements-dev.txt
just build  # this one may fail, try again below:
just build
build\datoviz.exe demo
```
