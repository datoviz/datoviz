# Build from source

Use the source path for the current development branch, C/C++ integration before packages are published, or Datoviz contribution work. For a published Python package, return to [Install](install.md).

<div class="dvz-context-strip">
  <span>Development checkout</span>
  <span>macOS, Linux, Windows</span>
  <span>Vulkan toolchain</span>
  <span>About 15–30 minutes</span>
</div>

The shortest route is: install the platform prerequisites below, clone with submodules, run `just build`, then run one focused test and the scatter example. Build commands on this page are complete shell commands; the CMake fragments linked from the integration guide are excerpts.

## Prerequisites

| Requirement | Why it is needed |
| --- | --- |
| Git | Downloads the source tree and submodules. |
| CMake 3.21+ | Configures the native build. |
| GCC 12+, Clang 15+, Apple Clang, or Visual Studio 2022 | Compiles native code. |
| Ninja | Recommended build backend. |
| `just` | Runs the project build commands used by the documentation. |
| Python 3.10+ and NumPy | Runs Python examples, binding tools, and documentation tools. |
| Vulkan-capable GPU and runtime | Renders native desktop and offscreen examples. |
| Shader path | The default canvas build requires `glslangValidator`. `glslc` is recommended for precompiled built-in scene SPIR-V; otherwise a usable shaderc runtime is required. |

## 1. Install platform prerequisites

Choose one platform section. WSL2 is a Linux environment and follows the Linux instructions; do not mix its libraries with a native Windows build.

### macOS

Install Apple's command-line tools and utilities for the normal vendored build:

```sh
xcode-select --install
brew install cmake glslang just ninja
```

The `glslang` package supplies `glslangValidator`. Install a Vulkan SDK providing `glslc`, MoltenVK, headers, and the loader when the selected environment does not already provide them. Current release wheels target macOS 15; source builds on other versions are development configurations, not wheel support claims.

### Linux

Ubuntu 24.04 is the reference Linux source-build environment:

```sh
sudo apt install build-essential cmake curl gcc git ccache ninja-build \
  xorg-dev clang-format patchelf tree libfreetype-dev glslang-tools
```

Install `just` if needed:

```sh
curl --proto '=https' --tlsv1.2 -sSf https://just.systems/install.sh | bash
```

Install a Vulkan SDK or distribution packages providing the loader, headers, and `glslc` for precompiled shaders. `glslang-tools` supplies `glslangValidator`. Desktop examples need a Vulkan-capable GPU and working drivers.

### Native Windows

A published native wheel is the primary Python release path. Source builds are for native development and C/C++ integration. Use Visual Studio 2022, the Vulkan SDK, CMake, Ninja, and vcpkg:

1. Install Visual Studio with the **Desktop development with C++** workload.
2. Install the LunarG Vulkan SDK and put `glslc` and `glslangValidator` on `PATH`.
3. Install CMake, Ninja, and vcpkg.
4. Open the Datoviz folder in Visual Studio.
5. Select the `msvc` CMake preset and build.

For a Linux development environment instead, install WSL2:

```powershell
wsl --install
```

Install Ubuntu, open its shell, and follow the Linux instructions above. On current Windows 11 systems with suitable Intel, AMD, or NVIDIA drivers, WSLg usually provides Vulkan GPU passthrough.

## 2. Clone and build

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

Run that command inside the virtual environment you intend to use. The editable installation points Python at this checkout; rebuilding the native library may still be necessary after source changes.

## 3. Verify the build

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

You should see colored points that pan and zoom. If the process cannot create a Vulkan instance or window, use [platform diagnostics](../how-to/diagnose-platform.md) before changing the example.

Continue with the complete [Quickstart](quickstart.md) or [First C Program](first-c-program.md).

## Dependency policy and troubleshooting context

The normal build prefers repository submodules for cglm, mimalloc, Kvazaar, GLFW, and msdf-atlas-gen where available. Distribution maintainers may set `DVZ_VENDORED_DEPS=OFF`; in that system-auto lane, CMake prefers installed dependencies while `AUTO` modes may fall back to vendored sources. See [Build options](../reference/build-options.md) for switches and packaging presets.

The default canvas build invokes `glslangValidator`. `glslc` is the recommended build-time compiler for embedded scene SPIR-V. Shaderc provides runtime GLSL compilation and is required by the release wheel configuration. Keep these roles separate when diagnosing a missing shader tool.

System-auto packaging additionally prefers installed GLFW, cglm, mimalloc, and Kvazaar development packages. Those are not required when the normal vendored sources are available.

## Package engineering status

Published RC2 wheels cover Linux x86_64/aarch64, macOS arm64/Intel, and Windows AMD64/ARM64. The release source bundle and checksums are attached to the GitHub prerelease. The vcpkg overlay and split conda-forge recipe remain engineering paths rather than published RC2 package channels. Use the [release notes index](../releases/index.md) and [project status](../reference/project-status.md) for the current posture.
