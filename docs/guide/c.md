# Using Datoviz in C

Datoviz is written in portable C and can be used directly in C applications for high-performance GPU visualization. The C API gives you full access to the library's core functionality, including visuals, scenes, interactivity, and rendering.

This guide shows how to build and run a simple C program with Datoviz, and how the Python API relates to it.

---

## C API Overview

The C API is defined in a single public header (which includes a few other files in `include/`):

```c
#include <datoviz.h>
```

You create and manage:

* `DvzApp` and `DvzBatch`: top-level context and command submission
* `DvzScene`, `DvzFigure`, `DvzPanel`: layout and interaction
* `DvzVisual`: one of the GPU-accelerated visual types

See the [C API reference](../reference/api_c.md) for the full list of functions and types.

---

## Python ↔ C Correspondence

The Datoviz **Python bindings** consist of:

1. Low-level `ctypes` bindings that directly wrap the C library,
2. A high-level Python wrapper built on top, with objects like `App`, `Visual`, `Panel`, etc.

The mapping for the raw `ctypes` bindings is:

| C API                   | Python API                 |
| ----------------------- | -------------------------- |
| `dvz_point_alloc(...)`  | `dvz.point_alloc(...)` |
| `dvz_mock_color(...)`   | `dvz.mock_color(...)`      |
| `DVZ_MARKER_SHAPE_DISC` | `dvz.MARKER_SHAPE_DISC`    |

Python functions are named the same (minus the `dvz_` prefix) and constants use `dvz.CONSTANT_NAME`.

---

## Building a C Program

To build a C application based on Datoviz:

1. Ensure Datoviz is compiled from source or use a prebuilt version (not yet available).
2. Include the `datoviz/` header directory.
3. Specify the link directories containing `libdatoviz.so` (or the equivalent for your OS).
4. Link against `libdatoviz` and the standard math library.

### Example (GCC):

```bash
gcc -std=c11 -O2 -I/path/to/datoviz/include \
    my_program.c -L/path/to/datoviz/build -ldatoviz -lm -o my_program
```

!!! note

    We plan to provide downloadable builds for major platforms in the future.

---

## Full Example

```c
--8<-- "examples/c/scatter.c"
```

This program creates a scatter plot with 10,000 points using the point visual. It uses mock data helpers for positions, colors, and sizes.

---

## Notes

* C usage provides the lowest-level, most performant access to Datoviz.
* The API is designed to be clear and safe to use in systems programming.
* The auto-generated Python `ctypes` bindings is identical to this C API to ensure consistent behavior across both languages.
* Most examples and documentation use the higher-level Python wrapper built on top of the `ctypes` bindings, for user convenience and to better decouple user code from the C API, which may still change with each release.


---

## See Also

* [Python Quickstart](../quickstart.md)
* [C API Reference](../reference/api_c.md)
* [Datoviz Architecture](../discussions/ARCHITECTURE.md)
