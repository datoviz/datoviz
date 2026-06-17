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

Use the wheel-provided package config for CMake projects:

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


## Headers

Use the umbrella header for application code:

```c
#include <datoviz.h>
```

The wheel bundles Datoviz public headers and the public third-party headers required by those
headers, including Vulkan and volk headers.


## Local Validation

Repository developers can validate the wheel C integration path with:

```bash
just build
tools/wheel_c_integration_smoke.sh
```

The smoke builds a temporary wheel, installs it into a temporary target directory, compiles a C
consumer with `datoviz-config`, and builds an out-of-tree CMake consumer with
`find_package(datoviz)`.
