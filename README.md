# Visky: scientific visualization with Vulkan


## Installation

### Dependencies

Mandatory dependencies:

* cmake
* ninja (for building)
* xcb
* glfw3
* vulkan
* cglm

Optional dependencies:

* libpng (screenshot and offscreen backends)
* ffmpeg (video backend)
* libvncserver (VNC backend)
* Qt5 (Qt example)


### Unix / macOS

The following instructions were tested on **Ubuntu 20.04**.

1. Install the latest graphics drivers for your system and hardware.
2. Install build tools and dependencies:

    `sudo apt install build-essentials cmake ninja-build xcb libx11-xcb-dev libglfw3-dev`

    - If you prefer to install **all dependencies in one go**:

    `sudo apt install cmake xcb libx11-xcb-dev libglfw3-dev libpng-dev libavcodec-dev libavformat-dev libavfilter-dev libavutil-dev libswresample-dev libvncserver-dev xtightvncviewer libqt5opengl5-dev libfreetype-dev`

3. Install the latest [Lunarg Vulkan SDK](https://vulkan.lunarg.com/) (tarball SDK), for example in `~/vulkan`

    1. `cd ~/vulkan`
    2. `./vulkansdk samples` (build the Vulkan samples)
    3. `./samples/build/Sample-Programs/Hologram/Hologram` (test an example)
    4. Add `source ~/vulkan/setup-env.sh` to your `~/.bashrc` so that the `$VULKAN_SDK` environment variable and other variables are properly set in your terminal.

4. Install [cglm](https://cglm.readthedocs.io/en/latest/):

    1. `git clone git@github.com:recp/cglm.git`
    2. `cd cglm`
    3. `mkdir build`
    4. `cd build`
    5. `cmake ..`
    6. `make`
    7. `sudo make install`

5. (Optional) Install optional dependencies to unlock all features.

    1. PNG support: `sudo apt install libpng-dev`
    2. FFmpeg support: `sudo apt install libavcodec-dev libavformat-dev libavfilter-dev libavutil-dev libswresample-dev`
    3. VNC support: `sudo apt install libvncserver-dev xtightvncviewer`
    4. Qt5 support: `sudo apt install libqt5opengl5-dev`
    5. To create custom font textures from TTF files: `sudo apt install libfreetype-dev`

6. Clone the visky repository and build the library:

    1. `git clone git@github.com:viskydev/visky.git`
    2. `cd visky`
    3. `bash manage.sh compile`

7. Check the compilation worked by running an example:

    1. `export VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation`
    2. `./build/app_blank`



### Windows 10

[WIP] The following instructions were tested on **Windows 10 and Visual Studio Community 2019**.

1. Install the latest graphics drivers for your system and hardware.
2. Install Visual Studio Community 2019 (free).
3. Install Cmake for Windows, and put `C:\Program Files (x86)\GnuWin32\bin` in the PATH.
4. Install vcpkg

    git clone vcpkg
    double click on bootstrap-vcpkg.bat
    .\vcpkg.exe integrate install

5. Install the latest [Lunarg Vulkan SDK](https://vulkan.lunarg.com/) (`.exe` executable).
    Windows Universal C Runtime https://stackoverflow.com/a/52329698
6. ...



## Tests

The library comes with a work-in-progress testing suite that runs a few examples (offscreen), makes screenshots, and compares them to presaved reference screenshots. A test pass if the two images are not too different (using a simple mean square metrics for now).

1. `bash manage.sh test`
2. Screenshots are saved in `test/screenshots/` the first time you run the tests.
3. Afterwards, screenshots are no longer saved but compared to the reference screenshots, unless a test fails, in which case the failing screenshot it saved with the`*_fail.png` suffix. To see a diff:

    1. Install ImageMagick.
    2. Make a diff with `compare test/screenshots/test_11_1/plot2d_scatter1* -compose src diff.png`
    3. Open `diff.png`

### No GPU? Enable pure CPU Vulkan emulation with Google SwiftShader

This is useful on computers with no GPUs or on continuous integration servers, for testing purposes only. SwiftShader only works with **offscreen backends**.

1. Install https://github.com/google/swiftshader
2. Temporarily override your native Vulkan driver with the SwiftShader one:

    1. Linux: `export LD_LIBRARY_PATH=/path/to/swiftshader/build/Linux/:$LD_LIBRARY_PATH`

3. Run the Visky tests as usual `./test.sh`
