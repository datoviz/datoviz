# Change Log

## Development version

Work in progress.

## v0.2.2 (2024-12-XX) [IN PREPARATION]

Minor release with a few improvements to the mesh visual, a couple of new GUI features, and an API export of the *Datoviz Rendering Protocol* (DRP) layer for more advanced used-cases from Python or C/C++.

### Visuals

#### Mesh visual

* Improved transparency capabilities (still imperfect, no order-independent transparency yet)
* Added support for multiple directional lights
* Started support for multiple blending types
* Added an example of using the mesh visual with dynamic updates

#### Volume visual

* Fixed a visual issue whenever volume textures are non-zero on the volume boundaries ([#72](https://github.com/datoviz/datoviz/issues/72))

### GUI

* Added a thin wrapper for simple trees
* Added a thin wrapper for simple tables with selectable rows

### API

* Exported the *Datoviz Rendering Protocol* (DRP) layer
* Added a box module, used internally by the upcoming axes system

### Build

* Added support for the [*shaderc*](https://github.com/google/shaderc/) to support GLSL to SPIR-V compilation from Datoviz.
* Added initial support for OpenMP multicore parallelization of tight loops
* Added basic error callback mechanism, used in assertions for now


## v0.2.1 (2024-09-17) [LATEST RELEASE]

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
