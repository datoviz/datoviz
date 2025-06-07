# Contributing notes

This document is a work in progress.

## Management commands

We use the [just](https://github.com/casey/just) tool for all management commands.
The commands are implemented in `justfile`.


## Branches

* The `main` branch is reserved to stable releases.
* Development occurs in the `dev` branch.


## Python bindings

Datoviz provides two layers of Python bindings:

1. **Low-level bindings** — Automatically generated `ctypes` wrappers for the C API.
2. **High-level Pythonic API** — A more user-friendly layer built on top of the raw bindings.

### Low-level ctypes bindings

The low-level bindings are automatically generated from the C headers and written to `datoviz/_ctypes_.py`. This file is committed to the repository but **should not be edited manually**.

The C header files are parsed by `tools/parse_headers.py`, which outputs a structured representation to `build/headers.json`. This JSON file serves as the source for both:

- The generation of the `ctypes` Python bindings.
- The generation of the C API documentation.

C functions such as `dvz_function()` are exposed in Python as `dvz.function()` (after `import datoviz as dvz`).
C enums like `DVZ_MYENUM` become `dvz.MYENUM` in Python.

This layer is a nearly 1:1 mapping of the C API, useful for advanced users or debugging.

### High-level Pythonic API

Since version **v0.3**, Datoviz also includes a more idiomatic Python API built on top of the `ctypes` layer. This API offers simplified, object-oriented access to Datoviz functionality.

For example:

```python
import datoviz as dvz

app = dvz.App()
fig = app.figure()
panel = fig.panel()
marker = app.marker(...)
panel.add(marker)
app.run()
```

This Pythonic layer abstracts away many of the lower-level details while retaining full performance and flexibility. It is the recommended entry point for most Python users.

Documentation and examples are being expanded as the API evolves.


## Continuous integration/continuous delivery

GitHub Actions-based CI/CD is not yet active in this repository.
In the near future, we intend to activate it for:

- cross-platform automated testing of the C library and Python bindings ;
- automated build of the Python wheels on all supported platforms ;
- automated build of the documentation and gallery.


## Debugging

### Console logging

You can control the verbosity of Datoviz's console output by setting the `DVZ_LOG_LEVEL` environment variable:

- `DVZ_LOG_LEVEL=2` — Info level (default)
- `DVZ_LOG_LEVEL=1` — Debug level
- `DVZ_LOG_LEVEL=0` — Trace level (very verbose; use with caution)

### Datoviz Intermediate Protocol requests

Datoviz user-facing commands generate an internal stream of rendering requests, which are processed in real time by the Vulkan renderer.
For debugging, you can inspect these requests to determine whether issues originate in the **high-level code** (which builds the requests) or in the **low-level renderer** (less common).

To print a YAML representation of the generated requests to standard output, set:

- `DVZ_VERBOSE=prt`

### Screenshot capture

To render all Datoviz applications offscreen and save a screenshot, set:

- `DVZ_CAPTURE_PNG=path/to/image.png` — Saves the rendered figure to a PNG file.

### Performance monitoring

Set the following environment variables to enable performance-related diagnostics:

* `DVZ_FPS=1` enables an FPS (frames per second) counter
* `DVZ_MAX_FPS=200` sets a frame rate limit (default is 200 FPS to reduce GPU usage)
* `DVZ_MAX_FPS=0` disables the frame rate limit for benchmarking purposes
- `DVZ_MONITOR=1` — Show a GPU memory monitor (allocated memory usage).

!!! note

    The current FPS computation is suboptimal and may not reflect true frame rate. Improvements are planned — contributions are welcome!



## Styling Guide

This section provides guidelines for maintaining consistent code style in the Datoviz project, for both C source code and the Python wrapper. Consistency helps readability and maintainability. Most formatting is automatically handled by tools.

---

### C Code Style

#### Code Formatting

- **Automatic formatting**: We use `clang-format` with a custom configuration. The `.clang-format` file is located at the root of the repository.
- **Auto-format on save**: It is assumed that your IDE or editor is configured to automatically format C/C++ files on save using this configuration.


#### Additional conventions

- Use comment banners (`/*****...*****/`) to clearly separate sections.
- Group includes and sort them logically: system headers, followed by local headers.
- Use `ANN(ptr)` (assert not null) for pointer assertions and `ASSERT(condition)` for general assertions.
- Keep function definitions short and readable; use blocks and whitespace to visually separate logic when helpful.
- Prefer function names with consistent prefixes (e.g., `dvz_axes_resize`, `dvz_demo_panel_3D`).
- Avoid trailing whitespace and excessive blank lines (more than 3 is trimmed by clang-format).



### Python Wrapper Style

#### Code Formatting and Linting

- We use Ruff for linting and formatting.
- The configuration is in `pyproject.toml`.
- Python files should be auto-formatted and linted using `ruff format` and `ruff check`.


#### Additional conventions

- Use PEP8 naming and structure wherever possible.
- Keep line length ≤ 99 characters.
- Docstrings should follow NumPy-style (`""" """`) format, with `Parameters`, `Returns`, etc.
- Prefer `assert` for internal sanity checks in constructors.
- Type hints should be used consistently, including in attribute declarations.
- Wrap class-level attributes with type annotations (e.g., `c_axes: dvz.DvzAxes = None`).
- Group code sections using banners and comments (e.g., `# Axes`,` # ----...----`).
- Avoid complex logic in wrapper code; keep it minimal and declarative.
