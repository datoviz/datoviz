# 1. Setup

**Your program at the end of this chapter: 9 lines.**

Nothing is drawn in this chapter. The goal is narrower and more important: a project directory, a
build you can run with one command, and proof that your compiler, the Datoviz headers, and the
Datoviz library agree with each other. Every later chapter assumes this works, and a build problem
discovered in chapter 12 is much harder to read than the same problem discovered now.

## Install Datoviz

!!! warning "This course needs a build newer than v0.4.0rc2"

    The course uses five functions that were added to the low-level layers after the RC2 package was
    published — among them `dvz_canvas_configure_gpu_ctx`, `dvz_commands_unwrap`, and
    `dvz_cmd_set_viewport_scissor`. Chapter 2 will not compile against `datoviz==0.4.0rc2`. Until the
    next package is out, build from source.

=== "From source (works today)"

    Follow [Build from source](../../start/build-from-source.md), then install the result into a
    prefix of your own:

    ```sh
    cmake --install build --prefix ~/datoviz-prefix
    ```

    That prefix contains the public headers, the library, and the CMake package this chapter uses.
    Note that a source install does not bring a Vulkan runtime with it: on macOS you need the Vulkan
    SDK's loader discoverable at run time, which is what the SDK's own setup script arranges.

=== "From a package (once published)"

    When a package newer than RC2 is available, this is the shorter road, and it carries its own
    Vulkan loader, MoltenVK on macOS, and shader compiler — nothing else to install:

    ```sh
    python -m venv .venv
    source .venv/bin/activate
    python -m pip install --upgrade pip
    python -m pip install datoviz
    ```

    It is a Python package, but you are here for the C library inside it: it ships the headers, a
    CMake package, and a `datoviz-config` helper. Keep the environment activated whenever you build.
    On Windows, use `py -m venv .venv` and `.\.venv\Scripts\Activate.ps1`.

Either way, what the rest of the course needs from you is one directory that CMake can find:
`DatovizConfig.cmake`, the `datoviz/` headers, and the library. Nothing below depends on which path
you took.

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

`<datoviz.h>` is the umbrella header. Later chapters include narrower headers alongside it —
`<datoviz/canvas.h>`, `<datoviz/vklite.h>` — as they need them. Include only headers under
`datoviz/`; anything from Datoviz's own `src/` directory is private and may change.

## Write the build file

CMake is the path that works on every platform, including Windows with MSVC. Create
`CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.21)
project(vkcourse C)

find_package(datoviz CONFIG REQUIRED)

add_executable(vkcourse main.c)
target_link_libraries(vkcourse PRIVATE datoviz::datoviz)
```

Those six lines are the whole build, for all fifteen chapters. `datoviz::datoviz` carries the
include paths, the library, and its dependencies — including the Vulkan headers you will start using
in chapter 2 — so you never add them by hand.

Configure once, pointing CMake at wherever you installed Datoviz:

=== "From source"

    ```sh
    cmake -S . -B build -DCMAKE_PREFIX_PATH=~/datoviz-prefix
    ```

=== "From a package"

    ```sh
    cmake -S . -B build -Ddatoviz_DIR="$(datoviz-config --cmake-dir)"
    ```

If Datoviz is installed system-wide, `cmake -S . -B build` on its own is enough.

Then build and run — this is the loop you will repeat for the rest of the course:

```sh
cmake --build build
./build/vkcourse
```

```
Datoviz 0.4.0
```

The exact version string is whatever you installed; what matters is that it printed one.

??? tip "A shorter loop on Linux and macOS"

    For a single-file program you can skip CMake entirely and let `datoviz-config` supply the flags:

    ```sh
    cc main.c $(datoviz-config --cflags --libs) -o vkcourse && ./vkcourse
    ```

    Convenient while iterating. It is not available for MSVC on Windows, which is why CMake is the
    course's default.

## When it goes wrong

| Symptom | Cause and fix |
| --- | --- |
| `fatal error: 'datoviz.h' file not found` | The compiler has no include path. With CMake, `find_package` failed to locate the package — check your `CMAKE_PREFIX_PATH` or `datoviz_DIR`. |
| `Could not find a package configuration file provided by "datoviz"` | Same cause, seen at configure time. CMake wants the directory containing `DatovizConfig.cmake`; `datoviz-config --cmake-dir` prints it for a package install. |
| `call to undeclared function 'dvz_canvas_configure_gpu_ctx'` in chapter 2 | Your Datoviz is older than the course — see the version warning at the top of this chapter. |
| Links fine, then `error while loading shared libraries` or `image not found` at startup | The dynamic loader cannot find the library at run time. Keep the environment activated; on Windows, ensure the package's DLL directory is on `PATH`. |
| `command not found: datoviz-config` | Either the environment is not activated, you built from source (where the helper is not installed), or you are on MSVC. Use `CMAKE_PREFIX_PATH`. |

A Vulkan driver is not needed yet — this program never touches the GPU. If your driver is missing,
you will find out in chapter 2, and the error message will say so clearly. One symptom worth knowing
in advance, because it looks alarming and is not: `cannot create Vulkan instance because Volk
initialization failed` means the Vulkan *loader* was not found at run time. Package installs bundle
one; source installs expect the Vulkan SDK's loader to be discoverable.

## Checkpoint

- Where do the Datoviz headers come from when you build, and which single CMake target provides
  them?
- Why does the course use CMake rather than a plain compiler invocation?
- What would you check first if the program compiled but failed to start?

Next: [Your first window](02-window.md).
