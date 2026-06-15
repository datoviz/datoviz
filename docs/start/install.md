# Install

Datoviz v0.4 is in active development and not yet available as a release package. The only
path today is building from source.

Release packages — pip wheel and system packages — will be published with the v0.4 release
candidate. The v0.3 stable release remains available on PyPI in the meantime.


## Prerequisites

| Requirement | Notes |
| --- | --- |
| Git | for cloning with submodules |
| CMake 3.20+ | build system |
| GCC 12+ or Clang 15+ | C/C++ compiler |
| Ninja | recommended build backend |
| [`just`](https://github.com/casey/just) | command runner used by all build targets |
| Python 3.8+ | for ctypes bindings and tools |
| NumPy | required by the Python package |
| Vulkan-capable GPU | integrated or discrete from the last ~10 years |

On **Ubuntu 24.04**, install system dependencies with:

```bash
sudo apt install build-essential cmake curl gcc git ccache ninja-build \
  xorg-dev clang-format patchelf tree libfreetype-dev

# Install just
curl --proto '=https' --tlsv1.2 -sSf https://just.systems/install.sh | bash
```

On **macOS**, install with Homebrew:

```bash
xcode-select --install
brew install cmake just ninja
```


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
