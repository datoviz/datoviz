# CMake Package Integration

Minimal C consumer for an installed Datoviz package.

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/datoviz/prefix
cmake --build build
./build/datoviz_cmake_package_example
```

The example uses `find_package(datoviz REQUIRED)` and links `datoviz::datoviz`.
