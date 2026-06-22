# Install

Datoviz v0.4 is in active development and not yet available as a release package. The only
path today is building from source.

Release packages — pip wheel and system packages — will be published with the v0.4 release
candidate. The v0.3 stable release remains available on PyPI in the meantime.


## Prerequisites

| Requirement | Notes |
| --- | --- |
| Git | for cloning with submodules |
| CMake 3.21+ | build system |
| GCC 12+ or Clang 15+ | C/C++ compiler |
| Ninja | recommended build backend |
| [`just`](https://github.com/casey/just) | command runner used by all build targets |
| Python 3.10+ | for ctypes bindings and tools |
| NumPy | required by the Python package |
| Vulkan-capable GPU | integrated or discrete from the last ~10 years |
| Shader tools | `glslc` from the Vulkan SDK for shader precompilation; `glslangValidator` for WGSL-generation tooling |

On **Ubuntu 24.04**, install system dependencies with:

```bash
sudo apt install build-essential cmake curl gcc git ccache ninja-build \
  xorg-dev clang-format patchelf tree libfreetype-dev glslang-tools

# Install just
curl --proto '=https' --tlsv1.2 -sSf https://just.systems/install.sh | bash
```

On **macOS**, install with Homebrew:

```bash
xcode-select --install
brew install cmake just ninja glslang
```

On **Windows via WSL2**, use Ubuntu and then follow the Linux path:

```powershell
wsl --install
```

Install Ubuntu 24.04 from the Microsoft Store, open the Ubuntu shell, then run the Ubuntu
dependency commands above. On Windows 11 with current Intel, AMD, or NVIDIA drivers, WSLg provides
Vulkan GPU passthrough for normal desktop use.

On **native Windows with Visual Studio**, install:

| Requirement | Notes |
| --- | --- |
| Visual Studio 2022 | include the Desktop development with C++ workload |
| Vulkan SDK | from LunarG; ensure `glslc` and `glslangValidator` are on `PATH` |
| vcpkg | recommended dependency manager for Visual Studio projects |
| CMake and Ninja | available from Visual Studio, vcpkg, or standalone installers |

Then open the Datoviz folder in Visual Studio. Visual Studio detects `CMakePresets.json`; select
the `msvc` preset and build. Native Windows pip wheels are part of the release-candidate package
set; vcpkg and conda packages wait for release tags and package-manager review.


## Clone and Build

```bash
git clone https://github.com/datoviz/datoviz.git --recursive
cd datoviz
git checkout v0.4-dev
just build
just build   # a known first-build issue may require a second pass
```

Install as a Python package (editable):

```bash
pip install -e .
```


## Verify

```bash
just test
```

For focused test runs:

```bash
just test scene    # scene-layer tests only
just test drp2     # render-stream tests only
```

Some GPU and window tests need the repository runtime environment. On systems using `direnv`:

```bash
direnv exec . just test scene
```


## Run an Example

```bash
just example-c visuals/point
./build/examples/c/visuals/point --live
```

See [Quickstart](quickstart.md) for a walkthrough of this example.


## Package Status

The v0.4 release-candidate packaging work is active:

| Path | Status |
| --- | --- |
| `pip install datoviz` | planned primary Python install path for RC wheels |
| Windows MSVC/vcpkg wheel | active CI path; AMD64 and ARM64 artifacts passed hosted wheel CI |
| Windows MSYS2/MinGW source path | supported GCC-compatible path through `datoviz-config` |
| vcpkg overlay | active draft for C/C++ users; publication waits for a stable release tag |
| conda-forge | draft recipe builds locally; feedstock submission waits for release tag and platform proof |

Until those packages are published, build from source for reproducible v0.4 testing.
