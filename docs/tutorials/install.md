# Installation

## How to install Datoviz?

!!! note
    Binary packages are still experimental. They are currently uploaded on GitHub releases, but not yet on PyPI. They will, once they have been sufficiently tested. In the meantime, please open an issue on GitHub if you have any problem.

=== "Linux"

    1. If you don't already have a **Python distribution**, install one. For example, [download the miniconda installer](https://docs.conda.io/en/latest/miniconda.html#linux-installers) for Linux 64-bit and install it.
    2. Open a terminal.
    3. Type the following to install Datoviz:

        ```bash
        pip install http://dl.datoviz.org/v0.1.0-alpha.0/datoviz-0.1.0a0-cp38-cp38-manylinux_2_24_x86_64.whl
        ```

    4. Type the following to test Datoviz:

        ```bash
        python -c "import datoviz; datoviz.demo()"
        ```


=== "macOS"

    1. If you don't already have a **Python distribution**, install one. For example, [download the miniconda pkg installer](https://docs.conda.io/en/latest/miniconda.html#macosx-installers) for macOS and install it.
    2. Open a terminal.
    3. Type the following to install Datoviz:

        ```bash
        pip install http://dl.datoviz.org/v0.1.0-alpha.0/datoviz-0.1.0a0-cp38-cp38-macosx_10_9_x86_64.whl
        ```

    4. Type the following to test Datoviz:

        ```bash
        python -c "import datoviz; datoviz.demo()"
        ```


=== "Windows"

    1. If you don't already have a **Python distribution**, install one. For example, [download the miniconda installer](https://docs.conda.io/en/latest/miniconda.html#windows-installers) for Windows 64-bit and install it.
    2. Open an Anaconda prompt.
    3. Type the following to install Datoviz:

        ```bash
        pip install http://dl.datoviz.org/v0.1.0-alpha.0/datoviz-0.1.0a0-cp38-cp38-win_amd64.whl
        ```

    4. Type the following to test Datoviz:

        ```bash
        python -c "import datoviz; datoviz.demo()"
        ```



## How to build Datoviz from source?

Datoviz is made of:

* a **C library** (also called libdatoviz),
* a **Python wrapper** (written in Cython)

The philosophy of Datoviz is to **implement all the logic and functionality in C**, and provide minimal bindings in high-level languages. This will ensure that all wrappers share the same functionality.


=== "Linux"

    !!! note
        Only Ubuntu 20.04 has been tested so far.

    1. Install the latest graphics drivers.
    2. Install the build tools:

        ```bash
        sudo apt install build-essential cmake ninja-build \
            xcb libx11-xcb-dev libxcursor-dev libxi-dev patchelf
        ```

    3. Install the optional dependencies:

        ```bash
        sudo apt install libpng-dev libavcodec-dev libavformat-dev \
            libavfilter-dev libavutil-dev libswresample-dev \
            libqt5opengl5-dev libfreetype6-dev
        ```

    4. Install the latest [Lunarg Vulkan SDK](https://vulkan.lunarg.com/) (tarball SDK), for example in `~/vulkan`.

    5. Export the Vulkan environment variables:

        ```bash
        source ~/vulkan/setup-env.sh
        ```

    6. Add `source ~/vulkan/setup-env.sh` to your `~/.bashrc` so that the `$VULKAN_SDK` environment variable and other variables are properly set in your terminal.

    7. Copy the Vulkan headers and libraries to your system:

        ```bash
        sudo cp -r $VULKAN_SDK/include/vulkan/ /usr/local/include/
        sudo cp -P $VULKAN_SDK/lib/libvulkan.so* /usr/local/lib/
        sudo cp $VULKAN_SDK/lib/libVkLayer_*.so /usr/local/lib/
        sudo mkdir -p /usr/local/share/vulkan/explicit_layer.d
        sudo cp $VULKAN_SDK/etc/vulkan/explicit_layer.d/VkLayer_*.json /usr/local/share/vulkan/explicit_layer.d
        ```

    8. Clone the Datoviz repository:

        ```bash
        git clone --recursive https://github.com/datoviz/datoviz.git
        cd datoviz
        ```

    9. Build the C library:

        ```bash
        ./manage.sh build
        ```

    10. Check that the compilation worked by running an example:

        ```bash
        ./manage.sh demo scatter
        ```

    11. Once the C library is compiled, you need to compile the Cython module:

        ```bash
        ./manage.sh cython
        ```

    12. Export the shared library path to your environment:

        ```bash
        source setup-env.sh
        ```

    13. Try a Python example:

        ```bash
        python bindings/cython/examples/quickstart.py
        ```


=== "macOS"

    1. Open a terminal.
    2. Type `git` to install git.
    3. Install Xcode.
    4. Install [Homebrew](https://brew.sh/).
    5. Install the build dependencies:

        ```bash
        brew install cmake ninja
        ```

    6. Download [the latest Vulkan SDK](https://vulkan.lunarg.com/sdk/home#mac).
    7. Install it.
    8. Clone the Datoviz repository:

        ```bash
        git clone --recursive https://github.com/datoviz/datoviz.git
        cd datoviz
        ```

    9. Build the C library:

        ```bash
        ./manage.sh build
        ```

    10. Check that the compilation worked by running an example:

        ```bash
        ./manage.sh demo scatter
        ```

    11. Once the C library is compiled, you need to compile the Cython module:

        ```bash
        ./manage.sh cython
        ```

    12. Export the shared library path to your environment:

        ```bash
        source setup-env.sh
        ```

    13. Try a Python example:

        ```bash
        python bindings/cython/examples/quickstart.py
        ```


=== "Windows 10"

    !!! warning
        Only **mingw-w64** is supported at the moment. Microsoft Visual C++ is not yet supported.

    !!! note
        Help needed to add more details to the install instructions below.

    1. Install the latest graphics drivers for your system and hardware.
    2. Install [Winlibs](http://winlibs.com/), a Windows port of gcc, using mingw-w64.
    3. Make sure the mingw executable is in the PATH.
    4. Install [CMake for Windows](https://cmake.org/download/).
    5. Install the [Windows Universal C Runtime](https://www.microsoft.com/en-US/download/details.aspx?id=48234).
    6. Install the latest [Lunarg Vulkan SDK](https://vulkan.lunarg.com/) (`.exe` executable).
    7. Install git for Windows and open a Git-aware Windows terminal.
    8. Clone the Datoviz repository:

        ```bat
        git clone --recursive https://github.com/datoviz/datoviz.git
        cd datoviz
        ```

    9. Build the C library:

        ```bat
        manage.bat build
        ```

    10. Build the Cython module and create a wheel:

        ```bat
        manage.bat wheel
        ```

    11. Try a Python example:

        ```bat
        python bindings\cython\examples\quickstart.py
        ```


## How to update Datoviz when it was compiled from source?

```
git pull
./manage.sh build
./manage.sh parseheaders
./manage.sh cython
```


## What are the dependencies of Datoviz?

**Mandatory dependencies** are required for compilation:

* **LunarG Vulkan SDK 1.2.170+**
* **cmake 3.16+** (build)

**Optional dependencies**:

* **ninja** (build)
* **freetype** (font support)
* **libpng** (screenshot)
* **ffmpeg** (screencasts)
* **Qt5** (upcoming Qt backend (work in progress)

**Built-in dependencies**:

* **glfw3 3.3+**: cross-platform windowing system
* **cglm**: basic types and math computations on vectors and matrices
* **stb_image**: image file input and output
* **Dear ImGui**: rich graphical user interfaces
* **earcut.hpp**: triangulation of polygons
* **triangle**: triangulation of complex polygons and planar straight-line graphs (PSLG)
* **tiny_obj_loader**: loading of `.obj` mesh files


## CPU emulation with Swiftshader

!!! note
    Swiftshader support is still experimental.

Software emulation of Vulkan is useful on computers with no GPUs or on continuous integration servers, for testing purposes. Datoviz has preliminary support for [Swiftshader](https://github.com/google/swiftshader), an emulation library developed by Google.

1. Compile Datoviz with Swiftshader support.
2. Compile Swiftshader.
3. Temporarily override your native Vulkan driver with the SwiftShader one:

    * Linux: `export LD_LIBRARY_PATH=/path/to/swiftshader/build/Linux/:$LD_LIBRARY_PATH`

4. Run your Datoviz script or application.
