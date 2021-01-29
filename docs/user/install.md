# Installation

!!! note
    Making easy-to-install packages for common systems is a work in progress. Help needed! In the meantime, you need to manually compile the library. The best supported platform at the moment is Ubuntu 20.04.

Visky should work on most systems. It has been developed on Linux (Ubuntu 20.04), tested on macOS (Intel). Windows support is preliminary, help needed.

## Dependencies

Mandatory dependencies that need to be installed before building the library:

* **LunarG Vulkan SDK**
* **cmake 3.16+** (build)
* **ninja** (build)

**The other dependencies below require no manual installation.**

Optional dependencies (used by the current version or an upcoming version of the library):

* **freetype** (optional)
* **libpng** (optional)
* **ffmpeg** (optional)
* **Qt5** (optional, not supported on Ubuntu *strictly* below 20.04)

Dependencies already included in the repository:

* **glfw3 3.3+**: cross-platform windowing system
* **cglm**: basic types and math computations on vectors and matrices
* **stb_image**: image file input and output
* **Dear ImGui**: rich graphical user interfaces
* **earcut.hpp**: triangulation of polygons
* **triangle**: triangulation of complex polygons and planar straight-line graphs (PSLG)
* **tiny_obj_loader**: loading of `.obj` mesh files


## Ubuntu 20.04

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

5. Clone the visky repository and build the library:

    1. `git clone --recursive git@github.com:viskydev/visky.git`
    2. `cd visky`
    3. `./manage.sh build`

6. Check that the compilation worked by running an example:

    1. (TODO) `./manage.sh example`

    Note: this will only work if Vulkan SDK's `setup-env.sh` file is source-ed in the terminal.

7. Compile the Cython module: `./manage.sh cython`
8. Export the shared library path in your environment: `source setup-env.sh`
9. Try a Python example: `python bindings/cython/examples/test.py`


## macOS

### Install the dependencies

1. Type `git` in a terminal to install it.
2. Install Xcode
3. Install [Homebrew](https://brew.sh/) if you don't have it already
4. Type `brew install cmake ninja`


### Install Vulkan

1. Install [the latest Vulkan SDK](https://vulkan.lunarg.com/sdk/home#mac)
2. `cd /Volumes/vulkansdk-macos-1.2.154.0` (replace by appropriate version)
3. `./install_vulkan.py`


### Install Visky

1. `git clone --recursive git@github.com:viskydev/visky.git`
2. `cd visky`
3. `./manage.sh build`
4. `./manage.sh example`


### Build the Cython module

1. Compile the Cython module: `./manage.sh cython`
2. Export the shared library path in your environment: `source setup-env.sh`
3. Try a Python example: `python bindings/cython/examples/test.py`


## Windows 10 (mingw-w64)

**Help needed to fill in the details**.

1. Install the latest graphics drivers for your system and hardware.
2. Install [Winlibs](http://winlibs.com/), a Windows port of gcc, using mingw-w64.
3. Install [CMake for Windows](https://cmake.org/download/)
4. Install the latest [Lunarg Vulkan SDK](https://vulkan.lunarg.com/) (`.exe` executable).
    Windows Universal C Runtime https://stackoverflow.com/a/52329698
5. Clone the repository.
6. Enter the following commands within the repository's directory:

    ```
    cd build
    cmake .. -G "MinGW Makefiles"
    mingw32-make
    cd ..
    ```
7. To run an example (the batch script will only work in cmd.exe, not Powershell):

    ```
    setup-env.bat
    build\visky.exe example (TODO)
    ```

**Note**: Visky does not yet compile with Microsoft Visual C++ compiler, help appreciated.



## Notes

### CPU emulation

CPU Vulkan emulation is useful on computers with no GPUs or on continuous integration servers, for testing purposes only. It is provided by the Swiftshader library developed by Google.

1. Install https://github.com/google/swiftshader
2. Temporarily override your native Vulkan driver with the SwiftShader one:

    1. Linux: `export LD_LIBRARY_PATH=/path/to/swiftshader/build/Linux/:$LD_LIBRARY_PATH`

3. Run the Visky tests as usual `./test.sh`
