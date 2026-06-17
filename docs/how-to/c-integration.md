# Use Datoviz From C Or C++

Datoviz v0.4 wheels include the C library, public headers, and CMake package metadata. After
installing the wheel, C and C++ consumers can build against the installed package without cloning
the repository.


## Compiler Flags

Use `datoviz-config` for direct compiler invocations on Linux, macOS, and MSYS2:

```bash
cc scatter.c $(datoviz-config --cflags --libs) -o scatter
./scatter
```

The command emits the installed include path, library path, and runtime rpath for the wheel
location.


## CMake

Use the installed package config for CMake projects:

```cmake
cmake_minimum_required(VERSION 3.21)
project(my_datoviz_app C)

find_package(datoviz REQUIRED)

add_executable(my_datoviz_app main.c)
target_link_libraries(my_datoviz_app PRIVATE datoviz::datoviz)
```

Configure with:

```bash
cmake -B build -Ddatoviz_DIR="$(datoviz-config --cmake-dir)"
cmake --build build
```

The maintained copy-pasteable example is in
`examples/c/integration/cmake_package/`.


## FetchContent

Projects that intentionally build Datoviz from source can embed it as a CMake subproject:

```cmake
include(FetchContent)

FetchContent_Declare(datoviz
    GIT_REPOSITORY https://github.com/datoviz/datoviz.git
    GIT_TAG v0.4.0
)

FetchContent_MakeAvailable(datoviz)

add_executable(my_datoviz_app main.c)
target_link_libraries(my_datoviz_app PRIVATE datoviz::datoviz)
```

When Datoviz is added this way, tests, examples, and install/package export rules are disabled by
default. Use the installed package path above for normal consumers; FetchContent is slower and
requires the full native build environment.

The maintained copy-pasteable example is in
`examples/c/integration/fetchcontent/`.


## Headers

Use the umbrella header for application code:

```c
#include <datoviz.h>
```

The wheel bundles Datoviz public headers and the public third-party headers required by those
headers, including Vulkan and volk headers.


## Local Validation

Repository developers can validate public header parseability in the normal build through
`dvz_public_header_probe` and `dvz_public_header_cpp_probe`.

The wheel C integration path is validated separately with:

```bash
just build
tools/wheel_c_integration_smoke.sh
```

The maintained CMake examples are validated with:

```bash
just c-integration-smoke
```

That smoke installs Datoviz into a temporary prefix, builds the `find_package()` example against
that prefix, and builds the FetchContent example against the local checkout.

The wheel smoke builds a temporary wheel, installs it into a temporary target directory, compiles a
C consumer with `datoviz-config`, and builds an out-of-tree CMake consumer with
`find_package(datoviz)`.
