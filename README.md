# Visky: scientific visualization with Vulkan

**Visky** is a **scientific visualization library written in C** and leveraging the low-level Vulkan API for GPUs. It is still highly experimental and the API/ABI changes a lot. The main goals of Visky are **performance, scalability, visual quality**, and ease of use. Longer-term goals are: integration with dynamic languages, easy and fast generation of static plots and videos, remote use.

Visky provides a high-level interface to create interactive plots, as well as a low-level API for those who want to deal with shaders, GPU buffers, etc. Visky is entirely implemented in C, except for thin C wrappers of C++ dependencies.

A longer-term goal is to provide a reasonably **stable ABI** that will allow dynamic languages to implement bindings via a foreign function interface. The `bindings/` subfolder provides binding proofs of concepts for **Python, Julia, and Rust** (help appreciated for other languages).

The main supported windowing library is **glfw** at the moment. The next step is to support Qt5 (note that Ubuntu 20.04 is required to have a distribution of Qt5 with Vulkan support), a proof of concept is provided in the examples. Offscreen backends are also provided, which are useful for testing, the video backend (using ffmpeg), and remote backends (there's a VNC backend proof of concept, and integration with Jupyter is planned via tornado and WebSockets).

Tighter integration with Dear ImGui is also planned to make it possible to build rich, interactive GUIs without the need for an external GUI system like Qt.


## Installation

### Dependencies

Mandatory dependencies:

* cmake
* ninja (for building)
* xcb
* glfw3
* vulkan

Optional dependencies:

* libpng (screenshot and offscreen backends)
* ffmpeg (video backend)
* libvncserver (VNC backend)
* Qt5 (Qt example)
* ASSIMP (mesh file loading)

The following libraries are statically bundled within the Visky library and they don't need to be installed separately:

* **cglm**: basic types and math computations on vectors and matrices
* **Dear ImGui**: rich graphical user interfaces
* **earcut**: triangulation of polygons
* **triangle**: triangulation of complex polygons and planar straight-line graphs (PSLG)
* **stb_image**: image file input and output
* **tiny_obj_loader**: loading of `.obj` mesh files


### Unix

The following instructions were tested on **Ubuntu 20.04**.

1. Install the latest graphics drivers for your system and hardware.
2. Install build tools and dependencies:

    `sudo apt install build-essential cmake ninja-build xcb libx11-xcb-dev libglfw3-dev`

    - If you prefer to install **all dependencies in one go**:

    `sudo apt install build-essential cmake ninja-build xcb libx11-xcb-dev libglfw3-dev libpng-dev libavcodec-dev libavformat-dev libavfilter-dev libavutil-dev libswresample-dev libvncserver-dev xtightvncviewer libqt5opengl5-dev libfreetype-dev libassimp-dev`

3. Install the latest [Lunarg Vulkan SDK](https://vulkan.lunarg.com/) (tarball SDK), for example in `~/vulkan`

    1. `cd ~/vulkan`
    2. `./vulkansdk samples` (build the Vulkan samples)
    3. `./samples/build/Sample-Programs/Hologram/Hologram` (test an example)
    4. **Important**: add `source ~/vulkan/setup-env.sh` to your `~/.bashrc` so that the `$VULKAN_SDK` environment variable and other variables are properly set in your terminal.

4. (Optional) Install optional dependencies to unlock all features.

    1. PNG support: `sudo apt install libpng-dev`
    2. FFmpeg support: `sudo apt install libavcodec-dev libavformat-dev libavfilter-dev libavutil-dev libswresample-dev`
    3. VNC support: `sudo apt install libvncserver-dev xtightvncviewer`
    4. Qt5 support: `sudo apt install libqt5opengl5-dev`
    5. ASSIMP support: `sudo apt install libassimp-dev`
    6. To create custom font textures from TTF files: `sudo apt install libfreetype-dev`

5. Clone the visky repository and build the library:

    1. `git clone git@github.com:viskydev/visky.git`
    2. `cd visky`
    3. `./manage.sh build`

6. Check that the compilation worked by running an example:

    1. `./manage.sh run app_triangle`

    Note: this will only work if Vulkan SDK's `setup-env.sh` file is source-ed in the terminal.


### macOS

**Help needed to fill in the details**. Note: the tests do not work yet because of a bug in the Vulkan SDK in the June 2020 release, which will be fixed in the next release.

1. Install the latest graphics drivers for your system and hardware.
2. Install xcode, brew, ninja, glfw3
3. Install the Vulkan SDK for macOS (it uses MoltenVK to map the Vulkan API to the native Apple Metal API).
4. Clone the repository and do `./manage.sh build` to build the library and the examples.
5. Use `./manage.sh run app_triangle` to run an example.



### Windows 10

**Not working yet, help needed**

1. Install the latest graphics drivers for your system and hardware.
2. Install Visual Studio Community 2019 (free).
3. Install Cmake for Windows, and put `C:\Program Files (x86)\GnuWin32\bin` in the PATH.
4. Install vcpkg

    git clone vcpkg
    double click on bootstrap-vcpkg.bat
    .\vcpkg.exe integrate install

5. Install the latest [Lunarg Vulkan SDK](https://vulkan.lunarg.com/) (`.exe` executable).
    Windows Universal C Runtime https://stackoverflow.com/a/52329698
6. To continue...


## Code organization

There are two sets of APIs:

* The **scene API**: this high-level API deals with visual elements such as markers, paths, images, text, subplots, axes...
* The **app API**: this low-level API deals with shaders, GPU buffers, pipelines, etc.

The scene API is built on top of the app API. The app API is written on top of a **thin Vulkan wrapper** called **vklite** and implemented in `src/vklite.c`. Almost all of Vulkan-specific code is found in this file. The app API could later support other modern GPU APIs such as WebGPU, DirectX 12 and Apple Metal since the abstractions provided by vklite (pipelines, shaders, buffers...) are roughly the same as those provided by these APIs.

There are some examples using either the app API or the scene API in `examples/`. There is also a testing suite that implements many other examples.


## Testing suite

There are no unit tests yet, but the library comes with an integration testing suite that runs a few examples (offscreen), makes screenshots, and compares them to presaved reference screenshots. A test passes if the two images are almost identical.

1. `./manage.sh test`
2. Screenshots are saved in `test/screenshots/` the first time you run the tests.
3. Afterwards, screenshots are no longer saved but compared to the reference screenshots, unless a test fails, in which case the failing screenshot it saved with the`*_fail.png` suffix. To see a diff:

    1. Install ImageMagick.
    2. Make a diff with `compare test/screenshots/test_11_image* -compose src diff.png`
    3. Open `diff.png`


## No GPU? Enable CPU Vulkan emulation with Google SwiftShader

This is useful on computers with no GPUs or on continuous integration servers, for testing purposes only. SwiftShader only works with **offscreen backends**.

1. Install https://github.com/google/swiftshader
2. Temporarily override your native Vulkan driver with the SwiftShader one:

    1. Linux: `export LD_LIBRARY_PATH=/path/to/swiftshader/build/Linux/:$LD_LIBRARY_PATH`

3. Run the Visky tests as usual `./test.sh`
