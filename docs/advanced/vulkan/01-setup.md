# 1. Setup

**Your program at the end of this chapter: 9 lines.**

Nothing is drawn in this chapter. The goal is narrower and more important: a project directory, a
build you can run with one command, and proof that your compiler, the Datoviz headers, and the
Datoviz library agree with each other. Every later chapter assumes this works, and a build problem
discovered in chapter 12 is much harder to read than the same problem discovered now.

## Install Datoviz

The published release candidate ships the native library, the public C headers, a CMake package, and
a `datoviz-config` helper. Install it into a virtual environment:

=== "macOS / Linux"

    ```sh
    python -m venv .venv
    source .venv/bin/activate
    python -m pip install --upgrade pip
    python -m pip install --pre datoviz==0.4.0rc2
    ```

=== "Windows PowerShell"

    ```powershell
    py -m venv .venv
    .\.venv\Scripts\Activate.ps1
    py -m pip install --upgrade pip
    py -m pip install --pre datoviz==0.4.0rc2
    ```

It is a Python package, but you are here for the C library inside it. Keep the environment activated
whenever you build, so the compiler and linker can find it.

If you would rather build Datoviz yourself, follow
[Build from source](../../start/build-from-source.md) and then
[C/C++ integration](../../how-to/c-integration.md) for the flags; the rest of this course is
identical either way.

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

Configure once:

```sh
cmake -S . -B build -Ddatoviz_DIR="$(datoviz-config --cmake-dir)"
```

The `-Ddatoviz_DIR` argument points CMake at the package inside your virtual environment. If
Datoviz is installed system-wide, `cmake -S . -B build` alone is enough.

Then build and run — this is the loop you will repeat for the rest of the course:

```sh
cmake --build build
./build/vkcourse
```

```
Datoviz 0.4.0rc2
```

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
| `fatal error: 'datoviz.h' file not found` | The compiler has no include path. With CMake, `find_package` failed to locate the package — check that the virtual environment is activated and pass `-Ddatoviz_DIR`. |
| `Could not find a package configuration file provided by "datoviz"` | Same cause, seen at configure time. `datoviz-config --cmake-dir` prints the directory CMake wants. |
| Links fine, then `error while loading shared libraries` or `image not found` at startup | The dynamic loader cannot find the library at run time. Keep the environment activated; on Windows, ensure the wheel's DLL directory is on `PATH`. |
| `command not found: datoviz-config` | Either the environment is not activated, or you are on MSVC where the helper is not installed. Use the CMake path. |

A Vulkan driver is not needed yet — this program never touches the GPU. If your driver is missing,
you will find out in chapter 2, and the error message will say so clearly.

## Checkpoint

- Where do the Datoviz headers come from when you build, and which single CMake target provides
  them?
- Why does the course use CMake rather than a plain compiler invocation?
- What would you check first if the program compiled but failed to start?

Next: [Your first window](02-window.md).
