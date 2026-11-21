# 1. Setup

*About 9 lines of C.*

![A terminal running the chapter 1 program and printing the Datoviz version.](../assets/gpu-graphics/01-setup.webp)

Before we draw anything, we need a build we can trust. A Datoviz program is ordinary C: you compile your source file against Datoviz's headers and link it with the Datoviz library. This chapter sets up that build once, for the whole course.

The program you write here prints the Datoviz version and draws nothing. That is deliberate. If your compiler, your Datoviz installation, or the library path is wrong, you find out now, in a nine-line program, rather than in chapter 12 among hundreds of lines of graphics code.

Follow these steps to build and run your first Datoviz program. Check the result at the end of each step before continuing.

In this chapter you will:

1. choose your operating system, then install Datoviz from source or from a package;
2. create a course project of your own, outside any Datoviz checkout;
3. write a `main.c` that prints the Datoviz version, and a `CMakeLists.txt` that finds Datoviz and links it;
4. configure, build, and run: the loop you will repeat in every chapter.

## What a build needs

A C program that uses a library needs three things from it, at three different moments:

- The **headers** declare the library's functions and types. The compiler reads them when it compiles `main.c`.
- The **library** holds the compiled code. The linker uses it when it builds the executable, and because Datoviz is a shared library, the system loads it again each time the program starts.
- The **CMake package**, `DatovizConfig.cmake`, tells CMake where the other two are, so your build file never names a path.

Each kind of failure points at one of these. A missing header is a compile error, a missing package is a configure error, and a library the system cannot find is a run-time error.

## Step 1: Choose your system and tools

The commands in this course depend on your operating system: which shell you type them in, how paths are written, and how a program finds shared libraries when it starts. The C code and the CMake build file do not; you write them once, whatever your system.

Select your system below. When you pick a tab, every tab with the same label on this page switches with it, and this browser remembers your choice for the other course pages that use the same tabs.

<noscript><p><strong>JavaScript is off:</strong> each set of tabs works on its own, so select your system again in every set of tabs on this page.</p></noscript>

<div id="course-platform" markdown="1">

=== "Linux"

    Type the commands in a terminal. They are written for a POSIX shell such as bash or zsh.

    You need:

    - a C compiler, GCC 12 or newer or Clang 15 or newer, and CMake 3.21 or newer;
    - a Vulkan-capable GPU with a working Vulkan driver, which the programs use from chapter 2;
    - for the source route in step 2, the rest of the [Linux prerequisites](../start/build-from-source.md#linux): Git, Ninja, `just`, and a Vulkan SDK or distribution packages that provide the Vulkan loader, the headers, and `glslc`; Ubuntu 24.04 is the reference environment;
    - for the package route in step 2, Python 3.10 or newer with its `venv` module.

=== "macOS"

    Type the commands in Terminal. They are written for a POSIX shell such as zsh, the default macOS shell, or bash.

    You need:

    - Apple's command-line tools, which provide Apple Clang (`xcode-select --install`), and CMake 3.21 or newer;
    - Vulkan support through **MoltenVK**, a library that runs Vulkan on top of Apple's Metal; where it comes from depends on the route in step 2;
    - for the source route, the rest of the [macOS prerequisites](../start/build-from-source.md#macos): Git, Ninja, `just`, and a Vulkan SDK that provides MoltenVK, the Vulkan loader, the headers, and `glslc`;
    - for the package route, Python 3.10 or newer; current release packages target macOS 15.

=== "Windows"

    Type the commands in a **Developer PowerShell for Visual Studio 2022**, which you open from the Start menu. It is PowerShell with the MSVC compiler and its tools already on `PATH`. Executables end in `.exe`, and you run one from the current directory with a `.\` prefix.

    You need:

    - Visual Studio 2022 with the **Desktop development with C++** workload, and CMake 3.21 or newer;
    - a Vulkan-capable GPU with a working Vulkan driver, which the programs use from chapter 2;
    - for the source route in step 2, the rest of the [Windows prerequisites](../start/build-from-source.md#native-windows): Git, the LunarG Vulkan SDK with `glslc` and `glslangValidator` on `PATH`, Ninja, and vcpkg, with the `VCPKG_ROOT` environment variable set to your vcpkg directory;
    - for the package route, Python 3.10 or newer, started with the `py` launcher.

    If you work inside WSL2, select **Linux** instead. WSL2 is a Linux environment, and its libraries must not mix with a native Windows build.

</div>

## Step 2: Install Datoviz

Datoviz reaches your machine by one of two routes. **From source** is the route that works for this course today. **From a package** is shorter, but only once a package newer than RC2 is published:

!!! warning "This course needs a build newer than v0.4.0rc2"

    The course uses functions added after the RC2 package, such as `dvz_canvas_configure_gpu_ctx()`, so chapter 2 will not compile against `datoviz==0.4.0rc2`; build from source until a newer package is published. A source build of `main` still prints `0.4.0rc2`, so trust a successful chapter 2 build, not the printed version.

Under your system, select the route. The route tabs are linked and remembered in the same way as the system tabs.

=== "Linux"

    === "From source"

        Get the source with the clone instructions in [Build from source](../start/build-from-source.md#2-clone-and-build). Then, from the root of the checkout, build the Release configuration explicitly and install it into a prefix in your home directory:

        ```sh
        DVZ_CMAKE_ARGS="-DDVZ_ENABLE_CUDA=OFF" just build Release
        cmake --install build --config Release --prefix "$HOME/datoviz-prefix"
        ```

        The course never uses CUDA; disabling it keeps the build independent of CUDA tooling. The prefix `$HOME/datoviz-prefix` now contains the public headers, the library, and the CMake package.

        A source install has no Vulkan loader of its own: it uses the loader and driver from your system or Vulkan SDK environment. Keep the environment you built Datoviz in active whenever you configure and run the course project. CMake must find Vulkan, and chapter 4 onward also needs the shaderc runtime library to compile GLSL.

    === "From a package"

        **Use this route only once a package newer than 0.4.0rc2 is published.** With RC2, this chapter works but chapter 2 does not compile.

        Create a Python virtual environment in your home directory, activate it, and install Datoviz into it:

        ```sh
        python3 -m venv "$HOME/datoviz-course-env"
        source "$HOME/datoviz-course-env/bin/activate"
        python -m pip install --upgrade pip
        python -m pip install --pre datoviz
        ```

        The Python package also contains the C library, the headers, the CMake package, and the `datoviz-config` helper, together with its own Vulkan loader and shader compiler. The environment lives at a fixed place in your home directory, not in the directory you are working in, so the same `source` line activates it from any directory, including the course project you create in step 3. Keep the environment activated whenever you build or run the course project; in a new terminal, run the `source` line again. `--pre` lets pip select a release candidate; it does not make RC2 compatible with this course.

=== "macOS"

    === "From source"

        Get the source with the clone instructions in [Build from source](../start/build-from-source.md#2-clone-and-build). Then, from the root of the checkout, build the Release configuration explicitly and install it into a prefix in your home directory:

        ```sh
        DVZ_CMAKE_ARGS="-DDVZ_ENABLE_CUDA=OFF" just build Release
        cmake --install build --config Release --prefix "$HOME/datoviz-prefix"
        ```

        The course never uses CUDA; disabling it keeps the build independent of CUDA tooling. The prefix `$HOME/datoviz-prefix` now contains the public headers, the library, and the CMake package.

        A source install has no Vulkan loader of its own: it uses the loader and MoltenVK from your Vulkan SDK environment. Keep the environment you built Datoviz in active whenever you configure and run the course project. CMake must find Vulkan, and chapter 4 onward also needs the shaderc runtime library to compile GLSL.

    === "From a package"

        **Use this route only once a package newer than 0.4.0rc2 is published.** With RC2, this chapter works but chapter 2 does not compile.

        Create a Python virtual environment in your home directory, activate it, and install Datoviz into it:

        ```sh
        python3 -m venv "$HOME/datoviz-course-env"
        source "$HOME/datoviz-course-env/bin/activate"
        python -m pip install --upgrade pip
        python -m pip install --pre datoviz
        ```

        The Python package also contains the C library, the headers, the CMake package, and the `datoviz-config` helper, together with its own Vulkan loader, MoltenVK, and shader compiler. The environment lives at a fixed place in your home directory, not in the directory you are working in, so the same `source` line activates it from any directory, including the course project you create in step 3. Keep the environment activated whenever you build or run the course project; in a new terminal, run the `source` line again. `--pre` lets pip select a release candidate; it does not make RC2 compatible with this course.

=== "Windows"

    === "From source"

        Get the source with the clone instructions in [Build from source](../start/build-from-source.md#2-clone-and-build), and work from the root of the checkout. The repository's `msvc` preset configures a Visual Studio 2022 x64 build in `build-msvc` and finds vcpkg's toolchain file through the `VCPKG_ROOT` environment variable, so check that this variable is set to your vcpkg directory. Then configure with CUDA disabled, build the Release configuration, install it into a prefix in your home directory, and put the runtime DLLs on `PATH`:

        ```powershell
        cmake --preset msvc -DDVZ_ENABLE_CUDA=OFF
        cmake --build build-msvc --config Release
        cmake --install build-msvc --config Release --prefix "$HOME/datoviz-prefix"
        $courseVcpkgBin = (Resolve-Path "build-msvc/vcpkg_installed/x64-windows/bin").Path
        $env:PATH = "$HOME/datoviz-prefix/bin;$courseVcpkgBin;$env:PATH"
        ```

        The course never uses CUDA; disabling it keeps the build independent of CUDA tooling. The prefix `$HOME/datoviz-prefix` now contains the public headers, the library, and the CMake package.

        The last two lines matter at run time. Windows finds DLLs through `PATH`, and the source installation still needs the native dependency DLLs in the vcpkg build tree, in addition to the Datoviz DLL in the prefix's `bin` directory. The vcpkg path above matches the preset's default `x64-windows` triplet; if you changed the triplet or `VCPKG_INSTALLED_DIR`, use its Release `bin` directory instead.

        Keep that build tree. A `PATH` change lasts only for the current terminal, so in each new terminal, run the two `PATH` lines again from the root of the checkout, where the vcpkg path is resolved, before moving into your course project.

        A source install has no Vulkan loader of its own: it uses the loader and driver from your system or Vulkan SDK environment. Keep the environment you built Datoviz in active whenever you configure and run the course project. CMake must find Vulkan, and chapter 4 onward also needs the shaderc runtime library to compile GLSL.

    === "From a package"

        **Use this route only once a package newer than 0.4.0rc2 is published.** With RC2, this chapter works but chapter 2 does not compile.

        In the Developer PowerShell, create a Python virtual environment in your home directory, activate it, install Datoviz into it, and add the package directory to `PATH`:

        ```powershell
        py -m venv "$HOME/datoviz-course-env"
        & "$HOME/datoviz-course-env/Scripts/Activate.ps1"
        python -m pip install --upgrade pip
        python -m pip install --pre datoviz
        $coursePackage = datoviz-config --prefix
        $env:PATH = "$coursePackage;$env:PATH"
        ```

        The Python package also contains the C library, the headers, the CMake package, and the `datoviz-config` helper, together with its own Vulkan loader and shader compiler. The environment lives at a fixed place in your home directory, not in the directory you are working in, so the same activation line works from any directory, including the course project you create in step 3. Activating the environment does not tell Windows where the Datoviz DLL is; the last two lines add the package directory to `PATH`, the DLL search path your C program uses. In each new terminal, run the activation line and the two `PATH` lines again. `--pre` lets pip select a release candidate; it does not make RC2 compatible with this course.

Both routes give CMake what it needs: `DatovizConfig.cmake`, the `datoviz/` headers, and the library. Later chapters do not depend on the route.

## Step 3: Create the course project

The course project is a directory of your own, outside the Datoviz checkout. It uses Datoviz only through what you installed in step 2, so it builds the same way whichever route you chose, and its build directory never mixes with Datoviz's own build directories.

If you built from source, first leave the checkout, for example with `cd ..` from its root. Then create the project directory and move into it; these two commands are the same in every shell:

```sh
mkdir vkcourse
cd vkcourse
```

The project needs two files: one source file and one build file.

## Step 4: Write `main.c`

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

`<datoviz.h>` is the umbrella header for ordinary Datoviz applications. From chapter 2, the course programs also include `<datoviz/advanced.h>`, the public opt-in header for low-level APIs such as the canvas and vklite, which ordinary applications do not need. These two public entry points cover the APIs we use in the course. Include only public headers under `datoviz/`; anything in Datoviz's `src/` directory is private.

## Step 5: Write `CMakeLists.txt`

The build file tells CMake how to turn `main.c` into a program. CMake works on every supported platform, including Windows with MSVC, so this one file serves every system. In the same directory, create `CMakeLists.txt`:

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

## Step 6: Configure the project

Configuring means letting CMake read `CMakeLists.txt`, find the Datoviz package, and write a build system into the `build` directory. You do it once per project: when `CMakeLists.txt` changes later, the build step reruns the configuration by itself. Configure again by hand only if you switch to another Datoviz installation. Point CMake to the installation from step 2:

=== "Linux"

    === "From source"

        ```sh
        cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$HOME/datoviz-prefix"
        ```

    === "From a package"

        ```sh
        cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -Ddatoviz_DIR="$(datoviz-config --cmake-dir)"
        ```

    CMake uses a single-configuration generator here, such as Make or Ninja, which builds one configuration per build directory. `-DCMAKE_BUILD_TYPE=Release` chooses that configuration now, to match the installed Release library.

=== "macOS"

    === "From source"

        ```sh
        cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$HOME/datoviz-prefix"
        ```

    === "From a package"

        ```sh
        cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -Ddatoviz_DIR="$(datoviz-config --cmake-dir)"
        ```

    CMake uses a single-configuration generator here, such as Make or Ninja, which builds one configuration per build directory. `-DCMAKE_BUILD_TYPE=Release` chooses that configuration now, to match the installed Release library.

=== "Windows"

    === "From source"

        ```powershell
        cmake -S . -B build -G "Visual Studio 17 2022" -DCMAKE_PREFIX_PATH="$HOME/datoviz-prefix"
        ```

    === "From a package"

        ```powershell
        cmake -S . -B build -G "Visual Studio 17 2022" -Ddatoviz_DIR="$(datoviz-config --cmake-dir)"
        ```

    `-G "Visual Studio 17 2022"` names the generator explicitly, so the build uses the Visual Studio 2022 toolchain from step 1 whatever CMake would otherwise pick. The Visual Studio generator is multi-configuration: it keeps every configuration in the same build directory, and you select Release in the build command of step 7, so the configure command needs no `-DCMAKE_BUILD_TYPE`.

If Datoviz is installed system-wide, omit the package-location option but keep the rest of your system's command, so that Release is still selected the same way. A successful configure ends with a line saying that the build files have been written; an error here usually means CMake did not find Datoviz or Vulkan (see "When it goes wrong" below).

## Step 7: Build, run, and check

Building compiles `main.c` and links it with Datoviz; running it proves the system also finds the library at run time. You will repeat this loop in every chapter:

=== "Linux"

    ```sh
    cmake --build build
    ./build/vkcourse
    ```

=== "macOS"

    ```sh
    cmake --build build
    ./build/vkcourse
    ```

=== "Windows"

    ```powershell
    cmake --build build --config Release
    .\build\Release\vkcourse.exe
    ```

    The Visual Studio generator puts each configuration in its own directory; keep `--config Release` and that executable path in later chapters.

A source build prints:

```
Datoviz 0.4.0rc2
```

A package prints its own version. The important check is that the program prints one, without a `(DEBUG)` suffix: a suffix, or a version you did not expect, means the program loaded another Datoviz build than the one you installed. "When it goes wrong" below shows how to find which one.

??? tip "Optional: compile without CMake"

    With a package install, you can skip CMake for a single-file program and let `datoviz-config` supply the compiler and linker flags. A source install has no `datoviz-config`, so keep using CMake there. The rest of the course always uses CMake.

    === "Linux"

        ```sh
        cc main.c $(datoviz-config --cflags --libs) -lm -o vkcourse && ./vkcourse
        ```

        `-lm` links the math library, as the `if(NOT WIN32)` block does in `CMakeLists.txt`.

    === "macOS"

        ```sh
        cc main.c $(datoviz-config --cflags --libs) -lm -o vkcourse && ./vkcourse
        ```

        `-lm` links the math library, as the `if(NOT WIN32)` block does in `CMakeLists.txt`.

    === "Windows"

        This shortcut does not work with MSVC: `datoviz-config` prints flags in the GCC and Clang style. Use CMake, as in steps 6 and 7.

??? warning "When it goes wrong"

    | Symptom | Cause and fix |
    | --- | --- |
    | The compiler cannot find `datoviz.h` | The compiler has no include path. With CMake, `find_package` failed to locate the package; check your `CMAKE_PREFIX_PATH` or `datoviz_DIR`. |
    | `Could not find a package configuration file provided by "datoviz"` | Same cause, seen at configure time. CMake wants the directory containing `DatovizConfig.cmake`; `datoviz-config --cmake-dir` prints it for a package install. |
    | `Could NOT find Vulkan` at configure time with a source install | CMake cannot find the Vulkan headers or loader library. Activate the SDK environment you built Datoviz in, or set `VULKAN_SDK` to your installed SDK directory, then configure again. |
    | In chapter 2, the compiler reports `dvz_canvas_configure_gpu_ctx` as undeclared | Your Datoviz is older than the course; see the version warning in step 2. |
    | `datoviz-config` is not found | Either the environment is not activated, you built from source (where the helper is not installed), or the package installation did not provide its console scripts. For a source install, use `CMAKE_PREFIX_PATH`. |

    The remaining problems appear when the program starts, and the fix depends on your system:

    === "Linux"

        | Symptom | Cause and fix |
        | --- | --- |
        | Links fine, then `error while loading shared libraries` at startup | The dynamic loader cannot find the library at run time. Keep the environment from step 2 activated. |
        | `Datoviz 0.4.0rc2 (DEBUG)` or an unexpected version | Another Datoviz build is on the library path. Run `ldd ./build/vkcourse`, find the `libdatoviz` line, and fix `LD_LIBRARY_PATH`. |

    === "macOS"

        | Symptom | Cause and fix |
        | --- | --- |
        | Links fine, then `Library not loaded` or `image not found` at startup | The dynamic loader cannot find the library at run time. Keep the environment from step 2 activated. |
        | `Datoviz 0.4.0rc2 (DEBUG)` or an unexpected version | Another Datoviz build is on the library path. Run `otool -L ./build/vkcourse` to see which `libdatoviz` the program was linked against, often as an `@rpath/` name; check that no inherited `DYLD_LIBRARY_PATH` points at another build, and reconfigure with the intended prefix rather than adding `DYLD_*` overrides. |

    === "Windows"

        | Symptom | Cause and fix |
        | --- | --- |
        | Links fine, then reports a missing DLL or exits at startup without printing | Windows cannot find a DLL at run time. Run the `PATH` lines from step 2 again in this terminal: the package directory for a package install, or the prefix's `bin` directory and the vcpkg `bin` directory for a source install. |
        | `Datoviz 0.4.0rc2 (DEBUG)` or an unexpected version | Another Datoviz DLL comes first on `PATH`. Open a new terminal and run only the `PATH` lines from step 2 for the installation you want, which put it first. |

    This program never touches the GPU, so it needs no Vulkan driver yet. If a later chapter prints `no Vulkan loader could be loaded`, Datoviz could not find the Vulkan *loader* at run time. Packages include one; source installs rely on your system or SDK. Check the loader, the installed driver (ICD), and `VULKAN_SDK`. [No Vulkan loader found](../how-to/diagnose-platform.md#no-vulkan-loader-found) lists every location Datoviz checks.

## What carries over

- A GPU program is still an ordinary program: headers at compile time, a library at link time, and shared libraries at run time. Knowing which step failed tells you where to look.
- Vulkan itself is reached the same way. A small **loader** library, found at run time, locates the driver installed for your GPU. Chapter 2 is the first program that needs it.

## Checkpoint

1. Where do the Datoviz headers come from when you build, and which single CMake target provides them?
2. Why does the course use CMake rather than a plain compiler invocation?
3. What would you check first if the program compiled but failed to start?

Next: [Your first window](02-window.md).
