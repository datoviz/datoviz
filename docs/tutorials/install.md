# Installation

!!! warning
    Datoviz is at an early stage of development and doesn't yet provide precompiled packages. Manual compilation is required.

Datoviz should work on most systems. It has been developed on Linux (Ubuntu 20.04), tested on macOS (Intel). Windows support is preliminary, help appreciated on this platform.

This section describes how to compile Datoviz on different operating systems.

## Python bindings

Datoviz comprises two parts:

* A full featured **C library**,
* Light **Python bindings** written in Cython.

The philosophy of Datoviz is to **implement all the logic and functionality in C**, and provide minimal bindings in high-level languages around this functionality. This design choice ensures that future bindings that might be developed in different languages would all share the same functionality (Julia, R, MATLAB, Rust, C#, and so on).

!!! note
    Most efforts were so far dedicated to the C library, whereas the Python bindings are at still an early stage of development.

**Dependencies of the Python bindings** are:

* NumPy
* IPython

**Building the Cython bindings** manually (which is mandatory at the moment) requires the following additional dependencies:

* cython
* pyparsing
* colorcet
* imageio


## Dependencies of the C library

**Mandatory dependencies** that need to be installed before compiling the C library are:

* **LunarG Vulkan SDK 1.2.170+**
* **cmake 3.16+** (build)

**Optional dependencies** are:

* **ninja** (build)
* **freetype** (optional)
* **libpng** (optional)
* **ffmpeg** (optional)
* **Qt5** (optional, not supported on Ubuntu *strictly* below 20.04)


The other dependencies below are already included in the library, so **you don't need to install them manually**:

* **glfw3 3.3+**: cross-platform windowing system
* **cglm**: basic types and math computations on vectors and matrices
* **stb_image**: image file input and output
* **Dear ImGui**: rich graphical user interfaces
* **earcut.hpp**: triangulation of polygons
* **triangle**: triangulation of complex polygons and planar straight-line graphs (PSLG)
* **tiny_obj_loader**: loading of `.obj` mesh files


## Manual compilation

### Ubuntu 20.04+

1. Install the latest graphics drivers.
2. Install the build tools:

    `sudo apt install build-essential cmake ninja-build xcb libx11-xcb-dev libglfw3-dev`

3. Install the optional dependencies:

    `sudo apt install libpng-dev libavcodec-dev libavformat-dev libavfilter-dev libavutil-dev libswresample-dev libvncserver-dev xtightvncviewer libqt5opengl5-dev libfreetype6-dev libassimp-dev`

4. Install the latest [Lunarg Vulkan SDK](https://vulkan.lunarg.com/) (tarball SDK), for example in `~/vulkan`

    1. `cd ~/vulkan`
    2. `./vulkansdk samples` (build the Vulkan samples)
    3. `./samples/build/Sample-Programs/Hologram/Hologram` (test an example)
    4. **Important**: add `source ~/vulkan/setup-env.sh` to your `~/.bashrc` so that the `$VULKAN_SDK` environment variable and other variables are properly set in your terminal.

5. Clone the Datoviz repository and build the library:

    1. `git clone --recursive git@github.com:datoviz/datoviz.git`
    2. `cd datoviz`
    3. `./manage.sh build`

6. Check that the compilation worked by running an example:

    1. `./manage.sh demo`

    Note: this will only work if Vulkan SDK's `setup-env.sh` file is source-ed in the terminal.

7. Compile the Cython module: `./manage.sh cython`
8. Export the shared library path in your environment: `source setup-env.sh`
9. Try a Python example: `python bindings/cython/examples/quickstart.py`



### macOS

#### Install the dependencies

1. Type `git` in a terminal to install it.
2. Install Xcode
3. Install [git-lfs](https://git-lfs.github.com/) (to download large test/example datasets)
4. Install [Homebrew](https://brew.sh/) if you don't have it already
5. Type `brew install cmake ninja`


#### Install Vulkan

1. Install [the latest Vulkan SDK](https://vulkan.lunarg.com/sdk/home#mac)
2. `cd /Volumes/vulkansdk-macos-1.2.170.0` (replace by appropriate version)
3. `./install_vulkan.py`


#### Install Datoviz

1. `git clone --recursive git@github.com:datoviz/datoviz.git`
2. `cd datoviz`
3. `./manage.sh build`
4. `./manage.sh demo`


#### Build the Cython module

1. Compile the Cython module: `./manage.sh cython`
2. Export the shared library path in your environment: `source setup-env.sh`
3. Try a Python example: `python bindings/cython/examples/quickstart.py`


### Windows 10

!!! warning
    Only **mingw-w64** is supported at the moment. Microsoft Visual C++ is not yet supported.

**Help needed to fill in the details below**:

1. Install the latest graphics drivers for your system and hardware.
2. Install [Winlibs](http://winlibs.com/), a Windows port of gcc, using mingw-w64. Make sure the mingw executable is in the PATH.
3. Install [CMake for Windows](https://cmake.org/download/)
4. Install the latest [Lunarg Vulkan SDK](https://vulkan.lunarg.com/) (`.exe` executable).
    * Install the Windows Universal C Runtime https://stackoverflow.com/a/52329698
5. Clone the repository: `git clone --recursive git@github.com:datoviz/datoviz.git`
6. Enter the following commands within the repository's directory (using the Windows terminal, **not** in Windows for Linux subsystem):

    ```
    mkdir build
    cd build
    cmake .. -G "MinGW Makefiles"
    mingw32-make
    cd ..
    ```

7. To run an example (the batch script will only work in cmd.exe, not Powershell):

    ```
    setup-env.bat
    set DVZ_INTERACT=1
    build\datoviz.exe test test_scene_ax
    ```

8. To build the Cython package:

    ```
    setup-env.bat
    cd bindings\cython
    pip install -r requirements.txt
    python setup.py build_ext -i -c mingw32
    python setup.py develop
    ```

9. Copy `build\libdatoviz.dll` to `bindings\cython\datoviz\` (there has to be a better way)

10. `python bindings\cython\examples\quickstart.py`


## Notes

### CPU emulation

CPU Vulkan emulation is useful on computers with no GPUs or on continuous integration servers, for testing purposes only. It is provided by the Swiftshader library developed by Google.

1. Install https://github.com/google/swiftshader
2. Temporarily override your native Vulkan driver with the SwiftShader one:

    1. Linux: `export LD_LIBRARY_PATH=/path/to/swiftshader/build/Linux/:$LD_LIBRARY_PATH`

3. Run the Datoviz tests as usual `./test.sh`
