# 1. Setup

**Your program at the end of this chapter: 9 lines.**

![A terminal running the chapter 1 program and printing the Datoviz version.](../assets/gpu-graphics/01-setup.webp)

You will not draw anything in this chapter. You will create a project directory, set up a repeatable build, and verify that your compiler, the Datoviz headers, and the Datoviz library agree. Every later chapter depends on this setup, so diagnose problems here rather than in chapter 12.

## Install Datoviz

!!! warning "This course needs a build newer than v0.4.0rc2"

    The course uses functions added to the low-level layers after the RC2 package was published, including `dvz_canvas_configure_gpu_ctx`, `dvz_commands_unwrap`, and `dvz_cmd_set_viewport_scissor`. Chapter 2 will not compile against `datoviz==0.4.0rc2`. Until a newer package is available, build from source.

=== "From source (works today)"

    Follow [Build from source](../start/build-from-source.md), then install the result into your own prefix:

    ```sh
    cmake --install build --config Release --prefix "$HOME/datoviz-prefix"
    ```

    On Windows, run the build commands from a Developer PowerShell for Visual Studio. The repository's `msvc` preset uses `build-msvc`; build its Release configuration and install into a prefix in your home directory:

    ```powershell
    cmake --build build-msvc --config Release
    cmake --install build-msvc --config Release --prefix "$HOME/datoviz-prefix"
    $env:PATH = "$HOME/datoviz-prefix/bin;$env:PATH"
    ```

    That prefix contains the public headers, library, and CMake package used in this chapter. A source install does not include its own Vulkan runtime; at run time, it uses the Vulkan loader and driver available through your system or configured SDK environment. If you keep the same environment you used to build Datoviz, the course needs no additional configuration.

=== "From a package (once published)"

    Once a package newer than RC2 is available, this is the shorter route. The package includes its own Vulkan loader, MoltenVK on macOS, and shader compiler, so you do not need to install those components separately:

    ```sh
    python -m venv .venv
    source .venv/bin/activate
    python -m pip install --upgrade pip
    python -m pip install --pre datoviz
    ```

    Although `datoviz` is installed as a Python package, it also contains the C library, headers, CMake package, and `datoviz-config` helper used by this course. Keep the environment activated whenever you build. On Windows, use a Developer PowerShell for Visual Studio, create the environment with `py -m venv .venv`, and activate it with `.\.venv\Scripts\Activate.ps1`. After installation, add the package directory to the DLL search path for your C program:

    ```powershell
    $coursePackage = datoviz-config --prefix
    $env:PATH = "$coursePackage;$env:PATH"
    ```

    Repeat that PATH setup in each new terminal. `--pre` lets pip select a release candidate; it does not make the older RC2 package compatible with this course.

Both installation routes give CMake the three things this course needs: `DatovizConfig.cmake`, the `datoviz/` headers, and the library. The remaining chapters do not depend on which route you choose.

## Create the project

One directory, one source file:

```sh
mkdir vkcourse
cd vkcourse
```

Create `main.c` and type it out:

```c
#include <stdio.h>

#include <datoviz.h>

int main(void)
{
    printf("Datoviz %s\n", dvz_version());
    return 0;
}
```

`<datoviz.h>` is the umbrella header. Later chapters add narrower headers such as `<datoviz/canvas.h>` and `<datoviz/vklite.h>` when they need them. Include only public headers under `datoviz/`; anything from Datoviz's own `src/` directory is private and may change.

## Write the build file

CMake works on every supported platform, including Windows with MSVC. Create `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.21)
project(vkcourse C)

find_package(datoviz CONFIG REQUIRED)

add_executable(vkcourse main.c)
target_link_libraries(vkcourse PRIVATE datoviz::datoviz)
if(NOT WIN32)
    target_link_libraries(vkcourse PRIVATE m)
endif()
```

This build file will serve all fifteen chapters. The `datoviz::datoviz` target supplies the include paths, library, and dependencies, including the Vulkan headers you will use from chapter 2 onward. On Unix, the separate `m` link supplies the standard math functions needed from chapter 3 onward; MSVC provides them without a separate library.

Configure the project once and point CMake to your Datoviz installation:

=== "From source"

    ```sh
    cmake -S . -B build -DCMAKE_PREFIX_PATH="$HOME/datoviz-prefix"
    ```

=== "From a package"

    ```sh
    cmake -S . -B build -Ddatoviz_DIR="$(datoviz-config --cmake-dir)"
    ```

If Datoviz is installed system-wide, `cmake -S . -B build` on its own is enough.

Then build and run. You will repeat this loop throughout the course:

=== "Linux and macOS"

    ```sh
    cmake --build build
    ./build/vkcourse
    ```

=== "Windows (Visual Studio)"

    ```powershell
    cmake --build build --config Release
    .\build\Release\vkcourse.exe
    ```

The Visual Studio generator puts each configuration in its own directory. Use the same `--config Release` and executable path in the following chapters.

```
Datoviz 0.4.0
```

The exact version depends on what you installed. At this point, the important check is that the program prints it.

??? tip "A shorter loop on Linux and macOS"

    For a single-file program you can skip CMake entirely and let `datoviz-config` supply the flags:

    ```sh
    cc main.c $(datoviz-config --cflags --libs) -lm -o vkcourse && ./vkcourse
    ```

    This is convenient while iterating, but it is not available with MSVC on Windows. The course therefore uses CMake by default.

## When it goes wrong

| Symptom | Cause and fix |
| --- | --- |
| `fatal error: 'datoviz.h' file not found` | The compiler has no include path. With CMake, `find_package` failed to locate the package; check your `CMAKE_PREFIX_PATH` or `datoviz_DIR`. |
| `Could not find a package configuration file provided by "datoviz"` | Same cause, seen at configure time. CMake wants the directory containing `DatovizConfig.cmake`; `datoviz-config --cmake-dir` prints it for a package install. |
| `call to undeclared function 'dvz_canvas_configure_gpu_ctx'` in chapter 2 | Your Datoviz is older than the course; see the version warning at the top of this chapter. |
| Links fine, then `error while loading shared libraries` or `image not found` at startup | The dynamic loader cannot find the library at run time. Keep the environment activated; on Windows, ensure the package's DLL directory is on `PATH`. |
| `command not found: datoviz-config` | Either the environment is not activated, you built from source (where the helper is not installed), or the package installation did not provide its console scripts. For a source install, use `CMAKE_PREFIX_PATH`. |

You do not need a Vulkan driver yet because this program never touches the GPU. Chapter 2 will detect a missing driver and report it. If you see `no Vulkan loader could be loaded`, Datoviz could not find the Vulkan *loader* at run time. Package installs include one; source installs rely on the loader and driver search paths provided by your system or SDK environment. Check the loader installation, the installed driver or ICD, and `VULKAN_SDK` if you use the SDK. [No Vulkan loader found](../how-to/diagnose-platform.md#no-vulkan-loader-found) lists every location Datoviz checks.

## Checkpoint

- Where do the Datoviz headers come from when you build, and which single CMake target provides them?
- Why does the course use CMake rather than a plain compiler invocation?
- What would you check first if the program compiled but failed to start?

Next: [Your first window](02-window.md).
