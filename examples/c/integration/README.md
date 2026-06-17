# C/C++ Integration Examples

These examples are maintained as copy-pasteable smoke tests for downstream C and C++ projects.

- `cmake_package/` uses an installed Datoviz package with `find_package(datoviz REQUIRED)`.
- `fetchcontent/` embeds Datoviz as a CMake subproject with `FetchContent_MakeAvailable(datoviz)`.

From the repository root, run both examples with:

```sh
tools/c_integration_smoke.sh
```
