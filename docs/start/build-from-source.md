# Build from source

Use the source path for the current development branch, C/C++ integration before packages are published, or Datoviz contribution work. For a published Python package, return to [Install](install.md).

The shortest route is: install the platform prerequisites below, clone with submodules, run `just build`, then run one focused test and the scatter example. Build commands on this page are complete shell commands; the CMake fragments linked from the integration guide are excerpts.

## Prerequisites

| Requirement | Why it is needed |
| --- | --- |
| Git | Downloads the source tree and submodules. |
| CMake 3.21+ | Configures the native build. |
| GCC 12+, Clang 15+, Apple Clang, or Visual Studio 2022 | Compiles native code. |
| Ninja | Recommended build backend. |
| `just` | Runs the project build commands used by the documentation. |
| Python 3.10+, NumPy, and PyYAML | Runs build-time profile checks, Python examples, binding tools, and documentation tools. |
| Vulkan-capable GPU and runtime | Renders native desktop and offscreen examples. |
| Shader path | Native scene and test shaders use `glslc`; Canvas has no built-in shader compiler dependency; external GLSL uses the optional runtime shaderc provider; CI and release builds also require `spirv-val`. |

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

### FreeBSD 14 amd64

FreeBSD is an initial, unverified community source-build target. Datoviz does not publish FreeBSD binaries or include FreeBSD in its release validation matrix. These conservative instructions deliberately disable unverified optional integrations; report successful builds or complete failure logs in [issue #65](https://github.com/datoviz/datoviz/issues/65).

As root, install the native compiler and system dependencies:

```sh
pkg install git cmake ninja pkgconf python3 \
  vulkan-headers vulkan-loader vulkan-tools shaderc \
  glfw cglm freetype2
```

Install and configure a Vulkan driver appropriate for the machine before attempting GPU or window tests. The [FreeBSD graphics documentation](https://docs.freebsd.org/en/books/handbook/x11/#x-config) describes the Intel, AMD, and NVIDIA driver paths. Follow the clone step below, then use the dedicated FreeBSD configuration instead of `just build`.

### Native Windows

A published native wheel is the primary Python release path. For source development, install Visual Studio 2022 with **Desktop development with C++**, the LunarG Vulkan SDK, CMake, Ninja, `just`, Python, Git for Windows, and vcpkg. Put CMake, Ninja, `just`, Git Bash, and the SDK's `glslc` on `PATH`.

The default Windows route uses MSVC x64 and Ninja in `build`. `just build` initializes the Visual Studio developer environment when needed and uses the Python interpreter selected by `python`. It finds vcpkg through `VCPKG_ROOT` or the selected Visual Studio installation's bundled vcpkg. Set `VCPKG_ROOT` if you use a separate checkout.

After cloning, prepare the Python environment in PowerShell:

```powershell
py -3 -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install numpy PyYAML
just build
just test planning
```

Use `just build Release` for an optimized build. The helper copies runtime DLLs beside the library, tests, and examples. An existing `build` directory configured with another compiler, generator, or source checkout is rejected; preserve its artifacts and move it aside before configuring the new route. The helper never deletes it.

For extra CMake options, set `DVZ_CMAKE_ARGS`. Use forward slashes in Windows paths and quote values containing spaces. The default `msvc-ninja` preset disables CUDA; enable it explicitly only when working on that optional integration. The separate `msvc` preset keeps the Visual Studio generator in `build-msvc`; its consumers need configuration-specific paths. `just mingw` uses a separate `build-mingw` tree and remains an unvalidated optional route.

For a Linux development environment instead, install WSL2:

```powershell
wsl --install
```

Install Ubuntu, open its shell, and follow the Linux instructions above. On current Windows 11 systems with suitable Intel, AMD, or NVIDIA drivers, WSLg usually provides Vulkan GPU passthrough.

## 2. Clone and build

Clone the active branch and initialize its recorded submodule revisions:

```sh
git clone --branch main https://github.com/datoviz/datoviz.git
cd datoviz
git submodule update --init --recursive
```

On macOS, Linux, and Windows, run the normal repository build:

```sh
just build
```

### Initial FreeBSD build

On FreeBSD, start with the native library and system dependencies while the broader feature surface remains unverified:

```sh
cmake -S . -B build-freebsd -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$PWD/build-freebsd/install" \
  -DDVZ_VENDORED_DEPS=OFF \
  -DDVZ_CGLM_SOURCE=SYSTEM \
  -DDVZ_ENABLE_KVAZAAR=OFF \
  -DDVZ_BUILD_GUI=OFF \
  -DDVZ_WITH_MSDF_ATLAS=OFF \
  -DDVZ_WITH_MSDF_SVG=OFF \
  -DDVZ_ENABLE_CUDA=OFF \
  -DDVZ_ENABLE_QT_BRIDGE=OFF \
  -DDVZ_ENABLE_SHADERC=AUTO \
  -DDVZ_BUILD_TESTING=OFF \
  -DDVZ_BUILD_EXAMPLES=OFF

cmake --build build-freebsd --parallel
cmake --build build-freebsd --target install
```

This first pass covers the native core, Vulkan, vklite, Canvas, DRP2, scene, app, GLFW, FreeType, installation metadata, and runtime shaderc discovery. It does not claim support for ImGui, MSDF text, Kvazaar, CUDA/NVENC, Qt, Python bindings, examples, or the test suite. After the native build succeeds, enable tests in the same build directory and report the complete result:

```sh
cmake -S . -B build-freebsd \
  -DDVZ_BUILD_TESTING=ON
cmake --build build-freebsd --parallel
./build-freebsd/testing/dvztest
```

Include the following diagnostics with a FreeBSD report:

```sh
freebsd-version -ku
uname -m
clang --version
cmake --version
pkg info
vulkaninfo --summary
```

Use [platform diagnostics](../how-to/diagnose-platform.md) for loader, device, window, and shader failures, and [Build options](../reference/build-options.md) before enabling additional dependencies. Python installation remains pending until the native library and generated ctypes path have been verified on FreeBSD.

On the currently validated macOS, Linux, and Windows source-build paths, use the generated Python binding from the checkout with:

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

In PowerShell, run the Windows executable as `.\build\examples\c\start\scatter.exe --live`. The Python binding finds the default source library in `build/src`; the Windows build stages its dependency DLLs there.

You should see colored points that pan and zoom. If the process cannot create a Vulkan instance or window, use [platform diagnostics](../how-to/diagnose-platform.md) before changing the example.

Continue with the complete [Quickstart](quickstart.md) or [First C Program](first-c-program.md).

## Check production stack usage

For GCC builds, enable `DVZ_STACK_USAGE` in a fresh build directory, build all targets plus the atlas generator, and check the compiler reports:

```sh
cmake -S . -B build-stack -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DPython3_EXECUTABLE="$(command -v python3)" \
  -DDVZ_ENABLE_CUDA=OFF \
  -DDVZ_STACK_USAGE=ON
cmake --build build-stack --parallel
cmake --build build-stack --target datoviz_text_atlas_generate
python tools/check_stack_usage.py --compile-commands build-stack/compile_commands.json --check
```

Repeat with `CMAKE_BUILD_TYPE=Release` in another fresh directory. The checker reports missing or stale evidence, uncompiled production sources, unbounded frames, and frames above 16 KiB. Its JSON output retains every frame from the analyzed compilation units. Per-function measurements do not establish total call-stack depth or coverage of disabled platform paths; the bounded worker regression and native Windows runs provide separate evidence. With MSVC, `DVZ_STACK_USAGE` enables `/analyze` and warning C6262 at the same threshold.

## Dependency policy and troubleshooting context

The normal build prefers repository submodules for cglm, Kvazaar, GLFW, and msdf-atlas-gen where available. Distribution maintainers may set `DVZ_VENDORED_DEPS=OFF`; in that system-auto lane, CMake prefers installed dependencies while `AUTO` modes may fall back to vendored sources. See [Build options](../reference/build-options.md) for switches and packaging presets.

The normal native build uses `glslc` for scene and test SPIR-V and does not require `glslangValidator`; Canvas has no built-in shader compiler dependency. Shaderc provides the separate runtime GLSL API for external shaders, while CI and release builds use `spirv-val` to validate generated SPIR-V. Keep these roles separate when diagnosing a missing shader tool.

System-auto packaging additionally prefers installed GLFW, cglm, and Kvazaar development packages. Those are not required when the normal vendored sources are available.

## Package engineering status

Published RC2 wheels cover Linux x86_64/aarch64, macOS arm64/Intel, and Windows AMD64/ARM64. The release source bundle and checksums are attached to the GitHub prerelease. The vcpkg overlay and split conda-forge recipe remain engineering paths rather than published RC2 package channels. Use the [release notes index](../releases/index.md) and [project status](../reference/project-status.md) for the current posture.
