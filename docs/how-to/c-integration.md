# Use from C or C++

Use this page when you want to consume Datoviz from your own native C or C++ build. It focuses on
headers, linking, package discovery, and integration smoke tests. For the scene and visual call
sequence itself, start with [First C Program](../start/first-c-program.md) or
[Create a scene](create-a-scene.md).

!!! info "At a glance"

    - **Status:** Supported installed-package and CMake `FetchContent` integration paths.
    - **Languages:** C and C++ through the public C ABI.
    - **Prerequisites:** CMake 3.21+ or a compatible C compiler/linker, plus Datoviz runtime dependencies.
    - **Result:** An application linked to `datoviz::datoviz` with public headers and discoverable runtime libraries.


## Public interface

Application code should include Datoviz public headers only:

```c
#include <datoviz.h>
/* or narrower headers such as <datoviz/app.h> and <datoviz/scene.h> */
```

Do not include files from `src/`; they are private implementation files and can change without
public compatibility promises.

The snippets on this page are complete build-system fragments, but `main.c` must contain a complete
Datoviz program such as [First C Program](../start/first-c-program.md). Pin an exact release tag or
commit before shipping an application.

C++ applications should call the same C API. Wrap Datoviz pointers in your own C++ classes if that
helps your application, but keep Datoviz ownership rules intact: destroy the app before destroying
the scene, and do not manually destroy scene-owned objects unless a specific API grants that
ownership.

Do not mix headers and shared libraries from different Datoviz builds. That can compile successfully
and still fail through ABI or generated-structure drift at runtime.


## Installed package

Use this path when Datoviz is already installed into the environment where you build your
application. The installed package exports public headers, the Datoviz library, a CMake package,
and, on GCC-compatible shells, a `datoviz-config` helper.

For a simple C compile:

```sh
cc main.c $(datoviz-config --cflags --libs) -o my_datoviz_app
./my_datoviz_app
```

This helper is intended for GCC-compatible shells on Linux, macOS, and MSYS2/MinGW. On Windows with
MSVC, use CMake instead; `datoviz-config` does not emit `/I` or `/LIBPATH:` flags.

For CMake projects, use the exported package:

```cmake
cmake_minimum_required(VERSION 3.21)
project(my_datoviz_app C)

find_package(datoviz CONFIG REQUIRED)

add_executable(my_datoviz_app main.c)
target_link_libraries(my_datoviz_app PRIVATE datoviz::datoviz)
```

If the Datoviz package directory is not on CMake's normal prefix path, point CMake at it:

```sh
cmake -S . -B build -Ddatoviz_DIR="$(datoviz-config --cmake-dir)"
cmake --build build
```

The repository keeps a minimal installed-package consumer at
`examples/c/integration/cmake_package/`.


## Source checkout

Use this path when packages are not available yet, when you need the current development branch, or
when you want Datoviz to build as part of your project.

CMake `FetchContent` is the maintained embedding path:

```cmake
cmake_minimum_required(VERSION 3.21)
project(my_datoviz_app C)

include(FetchContent)
FetchContent_Declare(datoviz
    GIT_REPOSITORY https://github.com/datoviz/datoviz.git
    GIT_TAG main  # development branch only; pin a release tag or commit for applications
)
FetchContent_MakeAvailable(datoviz)

add_executable(my_datoviz_app main.c)
target_link_libraries(my_datoviz_app PRIVATE datoviz::datoviz)
```

`main` is a moving development branch, not a reproducible application dependency. During RC
testing, replace it with the exact RC tag or tested commit. The final RC documentation pass must
replace this development example with the published RC tag once that tag exists; do not guess a tag
before publication.

During local Datoviz development, prefer pointing `FetchContent` at a checkout instead of fetching
from Git:

```sh
cmake -S . -B build -DDATOVIZ_FETCHCONTENT_SOURCE_DIR=/path/to/datoviz
cmake --build build
```

The repository keeps the copy-pasteable source-checkout consumer at
`examples/c/integration/fetchcontent/`.


## Runtime libraries

Datoviz is linked as a native library. Your executable must be able to find the Datoviz shared
library at runtime.

On Linux and macOS, install Datoviz into a normal prefix or set the runtime search path used by your
application packaging. For local smoke tests, `LD_LIBRARY_PATH` or `DYLD_LIBRARY_PATH` can point at
the Datoviz library directory.

On Windows, copy the Datoviz DLL next to your executable after build:

```cmake
add_custom_command(TARGET my_datoviz_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:datoviz::datoviz>"
        "$<TARGET_FILE_DIR:my_datoviz_app>"
)
```


## Validation

From a Datoviz source checkout, run both maintained C integration consumers with:

```sh
tools/c_integration_smoke.sh
```

This script builds and installs Datoviz into a temporary prefix, builds the
`find_package(datoviz)` consumer, then builds the `FetchContent` consumer against the current
checkout.

When validating a wheel or installed Python package that includes C integration files, use:

```sh
tools/wheel_c_integration_smoke.sh
```

See [Build options](../reference/build-options.md) for package export options and related build
presets.


## Common mistakes

- Including private headers from `src/`.
- Linking to a build-tree library while including headers from a different install.
- Forgetting to make the Datoviz shared library discoverable at runtime.
- Using `datoviz-config` with MSVC instead of the CMake package.
- Shipping an application whose `FetchContent` dependency still follows the moving `main`
  branch instead of an exact release tag or tested commit.
- Copying generated gallery source from the built docs instead of using `examples/c/...`.
- Destroying `DvzScene` before `dvz_app_destroy()` in an application that created an app.


## See also

- [First C Program](../start/first-c-program.md)
- [Create a scene](create-a-scene.md)
- [Open an interactive window](create-a-window.md)
- [Render offscreen](render-offscreen.md)
- [Build options](../reference/build-options.md)

??? example "Complete and related examples"

    - Canonical complete example: `examples/c/integration/cmake_package/`
    - Source: `examples/c/integration/fetchcontent/`
    - Smoke script: `tools/c_integration_smoke.sh`
