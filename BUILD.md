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

1. Install Git for Windows. You'll use the Git Bash console later.
2. Install [WinLibs](https://winlibs.com/). Download and install the latest gcc UCRT version with POSIX threads.
3. Download and decompress [just](https://github.com/casey/just/releases).
4. Copy the decompressed `just.exe` into `C:\mingw64\bin` (which should have been created by WinLibs).
5. Install the LunarG Vulkan SDK for Windows.
6. Open a Git Bash terminal, go to the Datoviz GitHub repository, and type `just build` twice.
7. Install the Vulkan Runtime for Windows, and copy the decompressed `vulkan-1.dll` into `build/`.
8. Open a bash terminal, go to `build/`, and type `./datoviz.exe demo`.