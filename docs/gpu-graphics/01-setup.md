# 1. Setup

*About 9 lines of C.*

![A terminal running the chapter 1 program and printing the Datoviz version.](../assets/gpu-graphics/01-setup.webp)

Before we draw anything, we need a build we can trust. A Datoviz program is ordinary C: you compile your source file against Datoviz's headers and link it with the Datoviz library. This chapter sets up that build once, for the whole course.

The program you write here prints the Datoviz version and draws nothing. That is deliberate. If your compiler, your Datoviz installation, or the library path is wrong, you find out now, in a nine-line program, rather than in chapter 12 among hundreds of lines of graphics code.

In this chapter you will:

1. install Datoviz, from source or from a package;
2. write a `main.c` that prints the Datoviz version;
3. write a `CMakeLists.txt` that finds Datoviz and links it;
4. configure, build, and run: the loop you will repeat in every chapter.

## What a build needs

A C program that uses a library needs three things from it, at three different moments:

- The **headers** declare the library's functions and types. The compiler reads them when it compiles `main.c`.
- The **library** holds the compiled code. The linker uses it when it builds the executable, and because Datoviz is a shared library, the system loads it again each time the program starts.
- The **CMake package**, `DatovizConfig.cmake`, tells CMake where the other two are, so your build file never names a path.

Each kind of failure points at one of these. A missing header is a compile error, a missing package is a configure error, and a library the system cannot find is a run-time error.

## Install Datoviz

!!! warning "This course needs a build newer than v0.4.0rc2"

    The course uses functions added after the RC2 package, such as `dvz_canvas_configure_gpu_ctx()`, so chapter 2 will not compile against `datoviz==0.4.0rc2`; build from source until a newer package is published. A source build of `main` still prints `0.4.0rc2`, so trust a successful chapter 2 build, not the printed version.

=== "From source (works today)"

    Follow the prerequisites and clone instructions in [Build from source](../start/build-from-source.md). For this course, build the Release configuration explicitly and install it into your own prefix:

    ```sh
    DVZ_CMAKE_ARGS="-DDVZ_ENABLE_CUDA=OFF" just build Release
    cmake --install build --config Release --prefix "$HOME/datoviz-prefix"
    ```

    The course never uses CUDA; disabling it keeps the build independent of CUDA tooling.

    On Windows, run the build commands from a Developer PowerShell for Visual Studio. The repository's `msvc` preset uses `build-msvc`; build its Release configuration and install into a prefix in your home directory:

    ```powershell
    cmake --build build-msvc --config Release
    cmake --install build-msvc --config Release --prefix "$HOME/datoviz-prefix"
    $env:PATH = "$HOME/datoviz-prefix/bin;$env:PATH"
    ```

    That prefix contains the public headers, library, and CMake package. A source install uses the Vulkan loader and driver from your system or SDK environment, so keep the environment you built Datoviz in.

=== "From a package (once published)"

    Once a package newer than RC2 is available, this is the shorter route. The package includes its own Vulkan loader, MoltenVK on macOS, and shader compiler:

    ```sh
    python -m venv .venv
    source .venv/bin/activate
    python -m pip install --upgrade pip
    python -m pip install --pre datoviz
    ```

    The Python package also contains the C library, headers, CMake package, and `datoviz-config` helper. Keep the environment activated whenever you build. On Windows, use a Developer PowerShell for Visual Studio, create the environment with `py -m venv .venv`, and activate it with `.\.venv\Scripts\Activate.ps1`. After installation, add the package directory to the DLL search path for your C program:

    ```powershell
    $coursePackage = datoviz-config --prefix
    $env:PATH = "$coursePackage;$env:PATH"
    ```

    Repeat that PATH setup in each new terminal. `--pre` lets pip select a release candidate; it does not make RC2 compatible with this course.

Both routes give CMake what it needs: `DatovizConfig.cmake`, the `datoviz/` headers, and the library. Later chapters do not depend on the route.

## Create the project

One directory, one source file:

```sh
mkdir vkcourse
cd vkcourse
```

Create `main.c` and type it out. It calls one Datoviz function, which needs the header at compile time and the library at link and run time, so it exercises the whole chain:

```c
#include <stdio.h>

#include <datoviz.h>

int main(void)
{
    printf("Datoviz %s\n", dvz_version());
    return 0;
}
```

`<datoviz.h>` is the umbrella header. Later chapters add narrower headers such as `<datoviz/canvas.h>` and `<datoviz/vklite.h>`. Include only public headers under `datoviz/`; anything in Datoviz's `src/` directory is private.

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

Read it as: find the Datoviz package or stop with an error, build one executable from `main.c`, and link it with Datoviz. This build file serves all sixteen chapters. The `datoviz::datoviz` target supplies the include paths, library, and dependencies, including the Vulkan headers. On Unix, `m` supplies the math functions used from chapter 3; MSVC needs no separate library.

Configure the project once and point CMake to your Datoviz installation:

=== "From source"

    ```sh
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$HOME/datoviz-prefix"
    ```

=== "From a package"

    ```sh
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -Ddatoviz_DIR="$(datoviz-config --cmake-dir)"
    ```

The Release build type matches the installed library on single-configuration generators such as Make and Ninja; Visual Studio selects it in the build command below. If Datoviz is installed system-wide, omit the package-location option but keep `-DCMAKE_BUILD_TYPE=Release`.

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

The Visual Studio generator puts each configuration in its own directory; keep `--config Release` and that executable path in later chapters. A source build prints:

```
Datoviz 0.4.0rc2
```

A package prints its own version. The important check is that the program prints one.

??? tip "A shorter loop on Linux and macOS"

    For a single-file program you can skip CMake entirely and let `datoviz-config` supply the flags:

    ```sh
    cc main.c $(datoviz-config --cflags --libs) -lm -o vkcourse && ./vkcourse
    ```

    This does not work with MSVC on Windows, so the course uses CMake.

!!! warning "When it goes wrong"

    | Symptom | Cause and fix |
    | --- | --- |
    | `fatal error: 'datoviz.h' file not found` | The compiler has no include path. With CMake, `find_package` failed to locate the package; check your `CMAKE_PREFIX_PATH` or `datoviz_DIR`. |
    | `Could not find a package configuration file provided by "datoviz"` | Same cause, seen at configure time. CMake wants the directory containing `DatovizConfig.cmake`; `datoviz-config --cmake-dir` prints it for a package install. |
    | `call to undeclared function 'dvz_canvas_configure_gpu_ctx'` in chapter 2 | Your Datoviz is older than the course; see the version warning at the top of this chapter. |
    | Links fine, then `error while loading shared libraries` or `image not found` at startup | The dynamic loader cannot find the library at run time. Keep the environment activated; on Windows, ensure the package's DLL directory is on `PATH`. |
    | `Datoviz 0.4.0rc2 (DEBUG)` or an unexpected version | Another Datoviz build is on the library path. Run `ldd ./build/vkcourse`, find the `libdatoviz` line, and fix `LD_LIBRARY_PATH`. |
    | `command not found: datoviz-config` | Either the environment is not activated, you built from source (where the helper is not installed), or the package installation did not provide its console scripts. For a source install, use `CMAKE_PREFIX_PATH`. |

    This program never touches the GPU, so it needs no Vulkan driver yet. If a later chapter prints `no Vulkan loader could be loaded`, Datoviz could not find the Vulkan *loader* at run time. Packages include one; source installs rely on your system or SDK. Check the loader, the installed driver (ICD), and `VULKAN_SDK`. [No Vulkan loader found](../how-to/diagnose-platform.md#no-vulkan-loader-found) lists every location Datoviz checks.

## What carries over

- A GPU program is still an ordinary program: headers at compile time, a library at link time, and shared libraries at run time. Knowing which step failed tells you where to look.
- Vulkan itself is reached the same way. A small **loader** library, found at run time, locates the driver installed for your GPU. Chapter 2 is the first program that needs it.

## Checkpoint

1. Where do the Datoviz headers come from when you build, and which single CMake target provides them?
2. Why does the course use CMake rather than a plain compiler invocation?
3. What would you check first if the program compiled but failed to start?

Next: [Your first window](02-window.md).
