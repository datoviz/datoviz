# Install

Datoviz v0.4 is still pre-release. The current public path is to build it from source. Public
v0.4 Python wheels, vcpkg packages, and conda packages are planned, but they are not the stable
installation path yet.

If you run:

```sh
pip install datoviz
```

you currently get the older v0.3 package from PyPI, not the v0.4 documentation described here.


## Choose Your Path

| You want to... | Use this path |
| --- | --- |
| Try the v0.4 Python examples today | Build from source, then run `pip install -e .` |
| Use Datoviz from C or C++ today | Build from source and link against the local build |
| Use a published Python package | Wait for the v0.4 pre-release wheels, or explicitly install one once published |
| Use Windows with the fewest surprises | Use WSL2 with Ubuntu 24.04 for now |
| Package Datoviz for another project | Use the source build today; vcpkg and conda packaging are still pre-release work |


## What You Need

Datoviz needs a recent compiler, CMake, Python, and a GPU that supports Vulkan.

| Requirement | Why it is needed |
| --- | --- |
| Git | downloads the source tree and its submodules |
| CMake 3.21+ | configures the native build |
| GCC 12+ or Clang 15+ | compiles the C/C++ code |
| Ninja | recommended build backend |
| `just` | runs the project build commands used in this documentation |
| Python 3.10+ and NumPy | runs the Python direct-engine layer and documentation tools |
| Vulkan-capable GPU | renders the native desktop examples |
| Shader tools | `glslc` and `glslangValidator` compile shader assets |


## Linux

Ubuntu 24.04 is the easiest Linux path.

```bash
sudo apt install build-essential cmake curl gcc git ccache ninja-build \
  xorg-dev clang-format patchelf tree libfreetype-dev glslang-tools

curl --proto '=https' --tlsv1.2 -sSf https://just.systems/install.sh | bash
```

Then clone and build:

```bash
git clone https://github.com/datoviz/datoviz.git --recursive
cd datoviz
git checkout v0.4-dev
just build
just build   # run a second time if the first pass stops on generated assets
pip install -e .
```


## macOS

Install Apple's command-line tools and the Homebrew packages used by the build:

```bash
xcode-select --install
brew install cmake just ninja glslang
```

Then clone and build:

```bash
git clone https://github.com/datoviz/datoviz.git --recursive
cd datoviz
git checkout v0.4-dev
just build
just build   # run a second time if the first pass stops on generated assets
pip install -e .
```

Datoviz uses Vulkan through MoltenVK on macOS. The source build prepares the runtime path used by
the examples and tests.


## Windows

The recommended Windows path for v0.4 testing is WSL2 with Ubuntu 24.04.

```powershell
wsl --install
```

Install Ubuntu 24.04 from the Microsoft Store, open the Ubuntu shell, then follow the Linux
instructions above. On current Windows 11 systems with Intel, AMD, or NVIDIA drivers, WSLg usually
provides Vulkan GPU passthrough for desktop examples.

Native Windows with Visual Studio is supported by the build system, but it is a more advanced path:

1. Install Visual Studio 2022 with the "Desktop development with C++" workload.
2. Install the LunarG Vulkan SDK and make sure `glslc` and `glslangValidator` are on `PATH`.
3. Install CMake and Ninja.
4. Open the Datoviz folder in Visual Studio.
5. Select the `msvc` CMake preset and build.

Native Windows Python wheels are part of the v0.4 release-candidate package work, but they are not
the public install path until the pre-release artifacts are uploaded.


## Check the Build

Run the full test suite when you have time:

```bash
just test
```

For a quicker check while learning the library:

```bash
just test scene
```

Some GPU and window tests need the repository runtime environment. If you use `direnv`, run:

```bash
direnv exec . just test scene
```


## Run One Example

Build and open the quickstart scatter plot:

```bash
just example-c start/scatter
./build/examples/c/start/scatter --live
```

You should see a window with colored points that you can pan and zoom. Continue with the
[Quickstart](quickstart.md) for the Python version and a short explanation of the code.


## Package Status

| Package path | Current v0.4 status |
| --- | --- |
| `pip install datoviz` | installs v0.3.x from PyPI today |
| v0.4 Python wheels | release-candidate artifacts have been validated in CI; public upload is pending |
| Source build | current recommended v0.4 path |
| C/C++ local integration | available from a source build |
| vcpkg | draft overlay exists; publication waits for a stable release tag |
| conda-forge | draft recipe exists; feedstock submission waits for release tag and platform proof |

Until packages are published, use the source build for reproducible v0.4 testing.
