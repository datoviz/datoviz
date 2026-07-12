# Install

These instructions target Datoviz v0.4.

After v0.4 packages are published on PyPI, the normal Python install command will be:

```sh
pip install datoviz
```

No public RC is assumed by this page. Until release notes name a published artifact, build from
source. Once an RC exists, its release notes are authoritative and may specify `--pre`, an exact
version, or an artifact URL. This page will show the exact versioned command after publication; do
not use a generic pre-release command in the meantime.


## Choose your path

| If you want to... | Start here |
| --- | --- |
| Use Datoviz from Python | [Python package](#python-package) |
| Use Datoviz from C or C++ | [C and C++](#c-and-c) |
| Build Datoviz yourself | [Build from source](#build-from-source) |
| Work on macOS | [macOS notes](#macos-notes) |
| Work on Linux | [Linux notes](#linux-notes) |
| Work on Windows | [Windows notes](#windows-notes) |


## Python package

Use this section after v0.4 packages or release-candidate artifacts are available for your platform.
Until then, use [Build from source](#build-from-source).

Create a virtual environment first, then use the exact install command from the published release
notes. The abbreviated platform setup is:

=== "macOS / Linux"

    ```sh
    python -m venv .venv
    source .venv/bin/activate
    python -m pip install --upgrade pip
    ```

=== "Windows PowerShell"

    ```powershell
    py -m venv .venv
    .\.venv\Scripts\Activate.ps1
    python -m pip install --upgrade pip
    ```

Run the exact install command from the published release notes after activating the environment.
After the final v0.4 package is published, that command will normally be:

```sh
pip install datoviz
```

Check that Python can import Datoviz:

```sh
python -c "import datoviz as dvz; print('datoviz import ok')"
```

Then continue with the [Quickstart](quickstart.md). Before choosing optional providers or deployment
targets, review [platform support and known limitations](../reference/platform-support.md).


## C and C++

For C or C++ applications, you need the native Datoviz library, headers, and runtime assets.

The intended release path is to use an installed Datoviz package and link against the exported CMake
package or command-line configuration helper. A typical CMake project will look like this:

```cmake
find_package(datoviz CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE datoviz::datoviz)
```

If packaged C/C++ artifacts are not available yet for your platform, build Datoviz from source and
link against that local build.


## Build from source

Build from source when you want the current development version, need C/C++ integration before
packages are published, or want to contribute to Datoviz.

You need:

| Requirement | Why it is needed |
| --- | --- |
| Git | downloads the source tree and submodules |
| CMake 3.21+ | configures the native build |
| GCC 12+, Clang 15+, Apple Clang, or Visual Studio 2022 | compiles the native code |
| Ninja | recommended build backend |
| `just` | runs the project build commands used in this documentation |
| Python 3.10+ and NumPy | runs Python examples and documentation tools |
| Vulkan-capable GPU | renders native desktop examples |
| Shader path | The default canvas build requires `glslangValidator`. `glslc` is recommended for precompiled built-in scene SPIR-V; otherwise a usable shaderc runtime is required. |

Clone the development branch and initialize the submodules at the revisions recorded by that branch:

```sh
git clone --branch v0.4-dev https://github.com/datoviz/datoviz.git
cd datoviz
git submodule update --init --recursive
```

Build Datoviz:

```sh
just build
```

If the first build stops after generating assets, run the same command once more:

```sh
just build
```

Install the local Python package from the checkout:

```sh
pip install -e .
```


### macOS notes

Install Apple's command-line tools and the build utilities used by the normal vendored build:

```sh
xcode-select --install
brew install cmake glslang just ninja
```

The `glslang` package supplies `glslangValidator` for the default canvas build. Install a Vulkan SDK
providing `glslc`, MoltenVK, headers, and the loader when those are not already provided by the
selected package/build environment. Current release wheels target macOS 15; source builds on other
versions are development configurations, not wheel-support claims.


### Linux notes

Ubuntu 24.04 is the reference Linux source-build environment. The normal build prefers repository
submodules for cglm, mimalloc, Kvazaar, GLFW, and msdf-atlas-gen where available; the system packages
below provide the toolchain and common platform libraries:

```sh
sudo apt install build-essential cmake curl gcc git ccache ninja-build \
  xorg-dev clang-format patchelf tree libfreetype-dev glslang-tools
```

Install `just` if it is not already available:

```sh
curl --proto '=https' --tlsv1.2 -sSf https://just.systems/install.sh | bash
```

Install a Vulkan SDK or distribution packages that provide the loader, headers, and `glslc` for
precompiled shaders. `glslang-tools` supplies the `glslangValidator` required by the default canvas
build. Linux desktop examples need a Vulkan-capable GPU and working graphics drivers.

Distribution/package maintainers may instead set `DVZ_VENDORED_DEPS=OFF`. In that system-auto lane,
CMake prefers installed GLFW, cglm, mimalloc, and Kvazaar packages while `AUTO` modes may fall back
to vendored sources. See [Build options](../reference/build-options.md).


### Windows notes

For Python users, a native Windows wheel is the primary release path once the release notes publish
one. Current wheel validation covers Windows AMD64 and ARM64 and includes the DLL, MSVC import
library, CMake package, and required runtime DLLs. Do not use an unpublished `pip` command merely
because wheel validation has passed.

For a native source build, use Visual Studio 2022, the Vulkan SDK, CMake/Ninja, and vcpkg. This is a
developer/C++ integration path, not a prerequisite for consuming a published Python wheel.

WSL2 with Ubuntu 24.04 is a separate Linux development option:

```powershell
wsl --install
```

Install Ubuntu 24.04 from the Microsoft Store, open the Ubuntu shell, then follow the Linux source
build instructions. On current Windows 11 systems with Intel, AMD, or NVIDIA drivers, WSLg usually
provides Vulkan GPU passthrough for desktop examples.

Native Windows source-build outline:

1. Install Visual Studio 2022 with the "Desktop development with C++" workload.
2. Install the LunarG Vulkan SDK and ensure `glslc` and `glslangValidator` are on `PATH`.
3. Install CMake, Ninja, and vcpkg.
4. Open the Datoviz folder in Visual Studio.
5. Select the `msvc` CMake preset and build.

Native Windows Python wheels are part of the v0.4 packaging path. During RC testing, use the exact
package command from the release notes.


### Check your install

For a source checkout, run a small test target:

```sh
just test scene
```

Some GPU and window tests need the repository runtime environment. If you use `direnv`, run:

```sh
direnv exec . just test scene
```


### Run one example

From a source checkout, build and open the quickstart scatter plot:

```sh
just example-c start/scatter
./build/examples/c/start/scatter --live
```

You should see a window with colored points that you can pan and zoom. Continue with the
[Quickstart](quickstart.md) for the Python version and a short explanation of the code.


## Package status

| Package path | v0.4 status |
| --- | --- |
| `pip install datoviz` | intended normal Python install command after v0.4 packages are published |
| Published RC wheel | no public command is assumed here; use only the exact version or artifact named by release notes |
| Source build | available for development, C/C++ integration, and package validation |
| C/C++ local integration | available from a source build |
| Native Windows wheels | validated on AMD64/ARM64; install only from an artifact named by published release notes |
| vcpkg | draft overlay exists; Windows overlay validation, a stable source bundle, and its final checksum remain publication gates |
| conda-forge | draft split `libdatoviz`/`datoviz` recipe has local source-bundle proof; no public feedstock/package is claimed |
| Release source bundle | RC preparation item; use only after a release publishes the bundle and checksum |

Before v0.4 packages are published, the source build is the v0.4 path. If a package command changes
during the RC phase, the release notes should be treated as the source of truth.
