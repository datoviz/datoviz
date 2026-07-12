# Build from source

Use the source path for the current development branch, C/C++ integration before packages are
published, or Datoviz contribution work. For a published Python package, return to
[Install](install.md).


## Prerequisites

| Requirement | Why it is needed |
| --- | --- |
| Git | Downloads the source tree and submodules. |
| CMake 3.21+ | Configures the native build. |
| GCC 12+, Clang 15+, Apple Clang, or Visual Studio 2022 | Compiles native code. |
| Ninja | Recommended build backend. |
| `just` | Runs the project build commands used by the documentation. |
| Python 3.10+ and NumPy | Runs Python examples and documentation tools. |
| Vulkan-capable GPU and runtime | Renders native desktop and offscreen examples. |
| Shader path | The default canvas build requires `glslangValidator`. `glslc` is recommended for precompiled built-in scene SPIR-V; otherwise a usable shaderc runtime is required. |


## Clone and build

Clone the active branch and initialize its recorded submodule revisions:

```sh
git clone --branch v0.4-dev https://github.com/datoviz/datoviz.git
cd datoviz
git submodule update --init --recursive
just build
```

If the first build stops after generating assets, run `just build` once more.

To use the generated Python binding from the checkout:

```sh
python -m pip install -e .
```


## Dependency policy

The normal build prefers repository submodules for cglm, mimalloc, Kvazaar, GLFW, and
msdf-atlas-gen where available. Distribution maintainers may set `DVZ_VENDORED_DEPS=OFF`; in that
system-auto lane, CMake prefers installed dependencies while `AUTO` modes may fall back to vendored
sources. See [Build options](../reference/build-options.md) for switches and packaging presets.

The default canvas build invokes `glslangValidator`. `glslc` is the recommended build-time compiler
for embedded scene SPIR-V. Shaderc provides runtime GLSL compilation and is required by the release
wheel configuration. Keep these roles separate when diagnosing a missing shader tool.


## macOS

Install Apple's command-line tools and utilities for the normal vendored build:

```sh
xcode-select --install
brew install cmake glslang just ninja
```

The `glslang` package supplies `glslangValidator`. Install a Vulkan SDK providing `glslc`, MoltenVK,
headers, and the loader when the selected environment does not already provide them. Current release
wheels target macOS 15; source builds on other versions are development configurations, not wheel
support claims.


## Linux

Ubuntu 24.04 is the reference Linux source-build environment:

```sh
sudo apt install build-essential cmake curl gcc git ccache ninja-build \
  xorg-dev clang-format patchelf tree libfreetype-dev glslang-tools
```

Install `just` if needed:

```sh
curl --proto '=https' --tlsv1.2 -sSf https://just.systems/install.sh | bash
```

Install a Vulkan SDK or distribution packages providing the loader, headers, and `glslc` for
precompiled shaders. `glslang-tools` supplies `glslangValidator`. Desktop examples need a
Vulkan-capable GPU and working drivers.

System-auto packaging additionally prefers installed GLFW, cglm, mimalloc, and Kvazaar development
packages. Those are not required when the normal vendored sources are available.


## Windows

A published native wheel is the primary Python release path; source builds are for native development
and C++ integration. Native source builds use Visual Studio 2022, the Vulkan SDK, CMake, Ninja, and
vcpkg:

1. Install Visual Studio with the **Desktop development with C++** workload.
2. Install the LunarG Vulkan SDK and put `glslc` and `glslangValidator` on `PATH`.
3. Install CMake, Ninja, and vcpkg.
4. Open the Datoviz folder in Visual Studio.
5. Select the `msvc` CMake preset and build.

WSL2 with Ubuntu 24.04 is a separate Linux development option:

```powershell
wsl --install
```

Install Ubuntu, open its shell, and follow the Linux instructions above. On current Windows 11
systems with suitable Intel, AMD, or NVIDIA drivers, WSLg usually provides Vulkan GPU passthrough.


## Verify the build

Run a focused scene test from the checkout:

```sh
just test scene
```

Some GPU and window tests need the repository runtime environment. With `direnv`, use:

```sh
direnv exec . just test scene
```

Build and open the canonical scatter scenario:

```sh
just example-c start/scatter
./build/examples/c/start/scatter --live
```

You should see colored points that pan and zoom. Continue with the [Quickstart](quickstart.md), or
review [platform support and known limitations](../reference/platform-support.md) when a runtime
target fails.


## Package engineering status

Hosted wheel validation covers Linux x86_64/aarch64, macOS arm64/Intel, and Windows AMD64/ARM64, but
passing validation does not publish an artifact. The vcpkg overlay, split conda-forge recipe, and
release source bundle remain pre-publication engineering paths until release notes publish their
locations and checksums. Use the [draft release notes index](../releases/index.md) and
[project status](../reference/project-status.md) for the current posture.
