# Developer notes

This page contains useful notes for developers and contributors.



## Manage script

For Linux and macOS, a bash script `manage.sh` at the root folder provides useful commands for developers and contributors.

For example, `./manage.sh build` builds the library with CMake.

!!! note "Windows users"
    A similar `manage.bat` Windows script is in progress. Another option would be to add Windows-specific commands to `manage.sh` and to require Windows users to use MSYS for running bash scripts. Help needed!

Each section on this page provides the details of the supported management commands.



## Building

| Command | Description |
| ---- | --- |
| `./manage.sh build` | compile the library |
| `./manage.sh rebuild` | recompile the library from scratch |
| `./manage.sh cython` | build the Cython bindings |
| `./manage.sh parseheaders` | force reparsing the C headers (used by the Cython and documentation generators) |

We use CMake to build the library.


### Library dependencies

* **Vulkan**: mandatory. The Vulkan SDK is required when *compiling* the library (not when *using* it).
* **cglm**: mandatory. It is included as a git submodule in `external/`.
* **glfw**: mandatory. On Linux, it needs to be installed via the package manager. On other systems, it is automatically downloaded by CMake (`FetchContent_Declare`).
* **Dear ImGui**: mandatory. It is included as a git submodule in `external/`.
* **glslc**: *optional*. If enabled, it is automatically downloaded by CMake.
* **libpng**: *optional*. If enabled, it needs to be installed via the package manager. **Support for other operating systems is lacking at the moment**.
* **ffmpeg**: *optional*. If enabled, it needs to be installed via the package manager. **Support for other operating systems is lacking at the moment**.

!!! note "Dear ImGui submodule"

    * We use [a fork of Dear ImGui in the Datoviz GitHub organization](https://github.com/datoviz/imgui) that we regularly keep in sync with the upstream repository.
    * We use a `datoviz` branch that derives from the `docking` branch from the upstream repository.
    * The only addition in the `datoviz` branch compared to the `docking` branch is that we use a [patch](https://github.com/martty/imgui/commit/f1f948bea715754ad5e83d4dd9f928aecb4ed1d3) in order to support ImGui panels (coming soon).


### Command-line tool

Datoviz includes an executable called `datoviz` that provides a testing suite and some examples. It is implemented in the `cli/` subfolder.


### C headers parsing

The script `utils/parse_headers.py` implements a simple parser (based on the pyparsing library) of the Datoviz headers. It exports the enums, structs, and functions to a JSON file in `utils/headers.json`. This JSON file is used by the Cython bindings generator, and by the documentation generator (mostly for the API doc generation).


### Cython bindings

Cython bindings are implemented in `bindings/cython`. The Cython module is built with dynamic linking to the C library libdatoviz.

The bindings are done semi-automatically. Specifically, a large part of the `cydatoviz.pxd` Cython file is generated automatically by the `utils/generate_cython.py` script. This script is called automatically by the command `./manage.sh cython`.


### Binary resource embedding

Binary resources such as SPIR-V-compiled shaders of all included graphics, the colormap texture, fonts, etc, are bundled into the shared C library as part of the CMake build.

The binary files are translated into standalone C files (generated in `build/_colortex.c`, `build/_shaders.c`, etc.) which are compiled with libdatoviz.


### Shaders

The shaders of the builtin graphics pipelines are found in `src/glsl/`. They are automatically compiled to SPIR-V as part of the CMake build process.

They include snippets of GLSL code found in `include/datoviz/glsl/`. This include path must be passed to the `glslc` command with the `-I` flag (this is done automatically by CMake).

SPIR-V compiled shaders are bundled into the library as part of the binary resource embedding process described above.



## Linting and static analysis

| Command | Description |
| ---- | --- |
| `./manage.sh format` | format all files with clang-format |
| `./manage.sh valgrind build/datoviz test test_scene_empty` | run Valgrind to debug segmentation faults and chase down memory leaks |
| `./manage.sh cppcheck` | static analysis of the codebase |
| `./manage.sh prof` | inspect the profiling information saved in `gmon.out` |


### Formatting

Formatting rules are defined in `.clang-format`. [We follow loosely this coding guide.](https://developer.lsst.io/cpp/api-docs.html)

### Valgrind

Valgrind output is saved to `.valgrind.out.txt`.

### Profiling

By default, Datoviz is built with profiling information automatically exported to `gmon.out`. Use `./manage.sh prof` to inspect the profiling information.



## Testing

| Command | Description |
| ---- | --- |
| `./manage.sh test [test_array_]` | run all tests, or optionally only those containing a given string |
| `./manage.sh pytest` | run all Python tests |
| `./manage.sh demo [mandelbrot]` | run all demos, or optionally only those containing a given string |


### C tests

Datoviz comes with an extensive suite of unit tests, for most modules in the library.

They are implemented in `tests/`. Datoviz uses a custom testing suite runner.

Some tests rely on automatic screenshot generation and comparison with a reference image. At the moment, the reference images are only saved locally, so they are only useful to check non-regression compared to previous runs on the same machine.


### Python tests

The Datoviz Python bindings come with a minimal testing suite, implemented in `bindings/cython/tests/` with pytest. Python tests also use automatic comparison with reference images saved locally.



## Documentation

| Command | Description |
| ---- | --- |
| `./manage.sh doc` | rebuild the doc website in `site/` |
| `./manage.sh docs` | serve the website on `localhost:8000` |
| `./manage.sh publish` | upload the generated website to GitHub |

We use mkdocs, with material theme, and several markdown, theme, and mkdocs plugins. See `mkdocs.yml`. The site is generated in the `site/` subfolder. We use GitHub Pages to serve the website.

Several parts of the documentation are auto-generated, via mkdocs hooks implemented in `utils/hooks.py`. Building the documentation requires Python dependencies found in `utils/requirements-build.txt`. In particular, we use the `mkdocs-simple-hooks` package to make it possible to use custom Python functions as mkdocs plugin hooks.

* **API documentation**: the list of functions to document is found in the `docs/api/*.md` files. At documentation build time, the API doc generation script (`utils/generate_doc.py`) relies on the generated `utils/headers.json` (created by `utils/parse_headers.py`) to extract the C docstrings and insert them at the right place in the API documentation pages.
* **Enumerations**: the documentation file `api/enums.md` contains a list of headers of enumerations. A script parses the enums in the library header files and inserts them in this file, at documentation build time.
* **Colormaps**: colormaps definitions are saved in a CSV file in `data/textures/color_texture.csv`. This file is parsed by `utils/export_colormap.py` and the table of all colormaps is automatically generated, using NumPy and Pillow to generate base64-encoded individual colormap images. The table is inserted at the end of `docs/user/colormaps.md`.
* **Visual documentation**: visuals are documented manually, screenshots are automatically generated by the testing suite.
* **Graphics documentation**: the item, vertex, params structure fields are automatically generated. The screenshots are automatically generated by the testing suite.
* **Code snippets and screenshots**: the documentation build script parses `<!-- CODE_PYTHON path/to/file.py -->` and `<!-- IMAGE path/to/image.png -->` in documentation sources and inserts the code file contents, or the image.



## Packaging

| Command | Description |
| ---- | --- |
| `./manage.sh wheel` | build a wheel package |
| `./manage.sh testwheel` | test the wheel package in a fresh virtual environment |

!!! warning
    The package creation pipeline is still a work in progress. The created wheels may not work on all systems.

We aim at providing an easy-to-install wheel package of both the C library and the Cython bindings, for the most common operating systems and environments. Installing the library should be as easy as doing `pip install datoviz`.

!!! note
    The wheels are created with no ffmpeg support for now. This will be fixed soon.

!!! note
    We only build wheels for Python 3.8 at the moment, we will support more Python versions soon.

!!! note
    We don't use conda for now, only Python wheels. They should be fully compatible with anaconda/miniconda distributions, however.

The main complication when building a wheel for Datoviz is that the Cython module has a dynamic dependency to libdatoviz, and possibly other libraries (especially when the library is compiled with ffmpeg support). We need to make sure these libraries are found by the Cython module, either by bundling them into the wheel, or by ensuring they are found on the user's environment. Some dependencies (such as libvulkan) require GPU access and therefore cannot be bundled with the wheel.

To test the wheel, the `./manage.sh testwheel` script automatically creates a fresh virtual environment, installs the `.whl` wheel package with `pip`, and executes the following code:

```python
from datoviz import canvas, run
canvas().gui_demo()
run()
```

Make sure you have the latest versions of pip and virtualenv.


### Windows

Creating a Python wheel for Windows should be as easy as running the following commands:

```
manage.bat build
manage.bat wheel
manage.bat testwheel
```

On Windows, the `setup.py` script automatically bundles the file `libdatoviz.dll` (created by the build script) into the wheel.

The wheel creating script uses the following command to create the wheel: `python setup.py bdist_wheel`.


### macOS

On macOS, in addition to running the standard wheel creation command `python setup.py bdist_wheel`, we also need to bundle `libdatoviz.dylib` into the wheel.

The [**delocate**](https://github.com/matthew-brett/delocate) script does just that. However, it will also bundle other dependencies such as libvulkan, which wouldn't work as Datoviz needs to use the system's libvulkan.

For this reason, we provide a [patch to delocate](https://github.com/matthew-brett/delocate/pull/106) which allows us to exclude libvulkan from the wheel. The patched version of delocate needs to be installed before running `./manage.sh wheel`.


### Linux

On Linux, like on macOS, we need to bundle dynamic libraries to the wheel, but *not* libvulkan as well as other graphics-related libraries.

While delocate is provided for macOS, the Python developers provide the [**auditwheel**](https://github.com/pypa/auditwheel/) tool to bundle dependencies into Python wheels. We need a [patched version of auditwheel](https://github.com/pypa/auditwheel/pull/310) so that we can exclude some libraries.

Another complication is that, in order to build a manylinux wheel, it is basically required to use Docker. The wheel needs to be compiled on an old Linux distribution so as to be compatible with as many Linux distributions as possible.

On Linux, the `./manage.sh wheel` script involves the following step:

1. Build a Docker image based on `quay.io/pypa/manylinux_2_24_x86_64` (provided by the Python developers). This image is based on Debian. Other Docker images are available, but they are based on CentOS, and Vulkan doesn't work well on old CentOS distributions.
2. We add a few extra packages to that Docker image that are required for building Datoviz (see the `/Dockerfile_wheel` file), including:
    * the Vulkan SDK,
    * a few Python libraries,
    * our patched version of auditwheel.
3. We create a Docker container based on this custom image, and we mount the git repository in the container.
4. We run a script in the container: `/wheel.sh`. This script does the following:
    * Build the Datoviz C library in the `/build_wheel` subfolder.
    * Copy the C headers, the build directory, and the Cython source files in a temporary directory in the container so as to avoid polluting the main Datoviz directory.
    * Build the Cython module.
    * Create a Python wheel.
    * Run the patched version of auditwheel to bundle libdatoviz in it.
    * Copy the repaired wheel to `/bindings/cython/dist`.

The resulting manylinux wheel may be uploadable to PyPI, [*but* we have to make sure it works on different Linux-based systems first](https://github.com/pypa/auditwheel/pull/310#issuecomment-849858348). We're likely to run into issues and we may have to include more dependencies via auditwheel. Until then, we only upload the wheel on GitHub.



## Environment variables

| Environment variable              | Description                                           |
|-----------------------------------|-------------------------------------------------------|
| `DVZ_DEBUG=1`                     | Run demos and examples interactively                  |
| `DVZ_LOG_LEVEL=0`                 | Logging level                                         |

* **Logging levels**: 0=trace, 1=debug, 2=info (default), 3=warning, 4=error
