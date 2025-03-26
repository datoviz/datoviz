# Change Log

## Development version

Work in progress.

### Ongoing developments

* Internal backend-related refactoring to better support Qt and offscreen rendering
* Better support for compute shaders
* More Vulkan/graphics features: multipass rendering, antialiasing, order-independent transparency...
* Integration with other GPU APIs such as CUDA/CuPy.


## v0.2.3 (2025-03-26) [LATEST RELEASE]

### Visuals

* Fixed lighting and positioning in sphere visual ([#80](https://github.com/datoviz/datoviz/issues/80) by @ron-adam).

### GUI

* Added docking support in ImGui wrapper.
* Added initial support for GUI panels, Datoviz panels that are movable and dockable with ImGui.

### Datoviz Rendering Protocol (DRP)

* Added support for push constants in DRP.
* Added support for color masks in DRP.

### App

* Added a way to recover the precise timestamps of the presentation of the last frames (`dvz_app_timestamps()`).

### Examples

* Added an example of recording a video of a Datoviz animation in Python using `imageio` (`examples/video.py`).

### Miscellaneous

* Implemented various bug fixes and optimizations.


## v0.2.2 (2025-01-19)

Minor release with a few improvements to the mesh visual, a couple of new GUI features, and preparatory work for the API export of the *Datoviz Rendering Protocol* (DRP) layer for more advanced used-cases from Python or C/C++.

### Visuals

#### Mesh visual

* Improved transparency capabilities (still imperfect, no order-independent transparency yet).
* Added support for multiple directional lights.
* Started support for multiple blend types.
* Added an example of using the mesh visual with dynamic updates.

#### Volume visual

* Fixed a visual issue whenever volume textures are non-zero on the volume boundaries ([#72](https://github.com/datoviz/datoviz/issues/72)).

### GUI

* Added a thin wrapper for simple trees.
* Added a thin wrapper for simple tables with selectable rows.

### API

* Exported the *Datoviz Rendering Protocol* (DRP) layer (not yet activate because of compatibility issues of shaderc on manylinux image -- fix upcoming).
* Enforced C_CONTIGUOUS ndarray arguments instead of generic `void*` pointers in some functions ([#71](https://github.com/datoviz/datoviz/issues/71)).
* Added a box module, used internally by the upcoming axes system.

### Build

* Added basic error callback mechanism, used in assertions for now.
* Improved Windows build instructions (see [discussion #73](https://github.com/datoviz/datoviz/discussions/73), pull requests [#74](https://github.com/datoviz/datoviz/pull/74) and [#75](https://github.com/datoviz/datoviz/pull/75)).
* Added initial support for OpenMP multicore parallelization of tight loops (not yet active because of compatibility issues on Windows).
* Added support for the [*shaderc*](https://github.com/google/shaderc/) to support GLSL to SPIR-V compilation from Datoviz.


## v0.2.1 (2024-09-17)

Minor release with various improvements to visuals and CI/CD system.

### Scene

- Implemented arcball GUI with sliders for rotation angles.
- Added ortho interactivity (like panzoom but with fixed aspect ratio).
- Implemented automatic data update of dirty visuals.

### Visuals

#### Basic visual

- Added functionality in `basic` visual (point size, groups).

#### Image visual

- Improved the `image` visual API (position is now specified in normalized device coordinates, the size in pixels, the anchor in relative coordinates).
- Added rescaling options (with aspect ratio kept or not).
- Added rounded corners and stroke options.
- Added fill option to replace the texture by a uniform color.

#### Mesh visual

- Added wireframe option in `mesh` visual.
- Added mesh isolines.
- Implemented polygon triangulation with the [earcut](https://github.com/mapbox/earcut.hpp) C++ library.
- Added experimental stroke contour option for polygons (fragment shader implementation). Better implementations coming soon.

#### Path visual

- Added open/close option in `path` visual.

#### Text visuals

- Added feature to generate a texture containing a string, using a given font.
- Implemented multiline support in text rendering using the font API.
- Added new _experimental_ `monoglyph` visual (pure GLSL low-quality glyph generation), works on macOS but not on Linux (?)

### GUI

- Added color picker widget.

### Documentation

- Added more documentation.
- Updated website `datoviz.org` to `v0.2.0`.

### Miscellaneous

- Improved demo now showing a grid of visuals.
- Improved FPS computation.
- Added mock functions.
- Rename upper/lower to top/bottom in API.
- Fixed various minor bugs.

### CI/CD

- Create Docker images with all build and run dependencies for Ubuntu, Almalinux (manylinux), Windows.
- Set up automated testing on GitHub Actions: Linux, macOS, Windows (partially).
- Set up automated wheel building on GitHub Actions: Linux, macOS, Windows.


## v0.2.0 (2024-08-05)

- Full internal architecture rewrite.
- Internal asynchronous message-based Datoviz Intermediate Protocol.
- Auto-generated Python bindings based on ctypes instead of Cython.
- Prebuilt binary Python wheels for Linux (manylinux), macOS, Windows.


## v0.1.0 (2021-02-17) [DEPRECATED]

- First experimental release.
- Cython bindings.
