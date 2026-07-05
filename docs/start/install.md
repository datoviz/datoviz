# Install

These instructions target Datoviz v0.4.

Before v0.4 packages are published, build Datoviz from source for v0.4 testing. The current stable
PyPI package is still the v0.3 line and should not be used for these v0.4 docs.

After v0.4 packages are published, the normal Python install command will be:

```sh
pip install datoviz
```

During the release-candidate phase, use the exact command from the release notes. It may temporarily
need a pre-release flag or an explicit version, for example:

```sh
pip install --pre datoviz
```


## Choose Your Path

| If you want to... | Start here |
| --- | --- |
| Use Datoviz from Python | [Python package](#python-package) |
| Use Datoviz from C or C++ | [C and C++](#c-and-c) |
| Build Datoviz yourself | [Build from source](#build-from-source) |
| Work on macOS | [macOS notes](#macos-notes) |
| Work on Linux | [Linux notes](#linux-notes) |
| Work on Windows | [Windows notes](#windows-notes) |


## Python Package

Use this section after v0.4 packages or release-candidate artifacts are available. Until then, use
[Build from source](#build-from-source).

Create a virtual environment first. This keeps Datoviz and its Python dependencies separate from your
system Python.

=== "macOS / Linux"

    ```sh
    python -m venv .venv
    source .venv/bin/activate
    python -m pip install --upgrade pip
    pip install --pre datoviz
    ```

=== "Windows PowerShell"

    ```powershell
    py -m venv .venv
    .\.venv\Scripts\Activate.ps1
    python -m pip install --upgrade pip
    pip install --pre datoviz
    ```

After the final v0.4 package is published, replace the last command with:

```sh
pip install datoviz
```

Check that Python can import Datoviz:

```sh
python -c "import datoviz as dvz; print('datoviz import ok')"
```

Then continue with the [Quickstart](quickstart.md).


## C And C++

For C or C++ applications, you need the native Datoviz library, headers, and runtime assets.

The intended release path is to use an installed Datoviz package and link against the exported CMake
package or command-line configuration helper. A typical CMake project will look like this:

```cmake
find_package(datoviz CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE datoviz::datoviz)
```

If packaged C/C++ artifacts are not available yet for your platform, build Datoviz from source and
link against that local build.


## Build From Source

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
| Shader tools | `glslc` and `glslangValidator` compile shader assets |

Clone the repository with submodules:

```sh
git clone https://github.com/datoviz/datoviz.git --recursive
cd datoviz
```

For the v0.4 development branch:

```sh
git checkout v0.4-dev
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


## macOS Notes

Install Apple's command-line tools and the Homebrew packages used by the build:

```sh
xcode-select --install
brew install cmake just ninja glslang
```

Datoviz uses Vulkan through MoltenVK on macOS. The source build prepares the runtime path used by
the examples and tests.


## Linux Notes

Ubuntu 24.04 is the easiest Linux path.

```sh
sudo apt install build-essential cmake curl gcc git ccache ninja-build \
  xorg-dev clang-format patchelf tree libfreetype-dev glslang-tools
```

Install `just` if it is not already available:

```sh
curl --proto '=https' --tlsv1.2 -sSf https://just.systems/install.sh | bash
```

Linux desktop examples need a Vulkan-capable GPU and working graphics drivers.


## Windows Notes

For the fewest surprises during v0.4 testing, use WSL2 with Ubuntu 24.04:

```powershell
wsl --install
```

Install Ubuntu 24.04 from the Microsoft Store, open the Ubuntu shell, then follow the Linux source
build instructions. On current Windows 11 systems with Intel, AMD, or NVIDIA drivers, WSLg usually
provides Vulkan GPU passthrough for desktop examples.

Native Windows with Visual Studio is a more advanced path:

1. Install Visual Studio 2022 with the "Desktop development with C++" workload.
2. Install the LunarG Vulkan SDK and make sure `glslc` and `glslangValidator` are on `PATH`.
3. Install CMake and Ninja.
4. Open the Datoviz folder in Visual Studio.
5. Select the `msvc` CMake preset and build.

Native Windows Python wheels are part of the v0.4 packaging path. During RC testing, use the exact
package command from the release notes.


## Check Your Install

For Python, run:

```sh
python -c "import datoviz as dvz; print('datoviz import ok')"
```

For a source checkout, run a small test target:

```sh
just test scene
```

Some GPU and window tests need the repository runtime environment. If you use `direnv`, run:

```sh
direnv exec . just test scene
```


## Run One Example

From a source checkout, build and open the quickstart scatter plot:

```sh
just example-c start/scatter
./build/examples/c/start/scatter --live
```

You should see a window with colored points that you can pan and zoom. Continue with the
[Quickstart](quickstart.md) for the Python version and a short explanation of the code.


## Package Status

| Package path | v0.4 status |
| --- | --- |
| `pip install datoviz` | intended normal Python install command after v0.4 packages are published |
| `pip install --pre datoviz` | possible RC command if the v0.4 package is published as a pre-release |
| Source build | available for development, C/C++ integration, and package validation |
| C/C++ local integration | available from a source build |
| vcpkg | planned package path after a stable release tag |
| conda-forge | planned package path after release tag and platform validation |

Before v0.4 packages are published, the source build is the v0.4 path. If a package command changes
during the RC phase, the release notes should be treated as the source of truth.
