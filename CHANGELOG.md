# Change Log

## Development version

Ongoing developments planned for v0.4.0 (2026) include:

* Internal backend-related refactoring to better support Qt and offscreen rendering.
* Better support for compute shaders.
* More Vulkan/graphics features: multipass rendering, antialiasing, order-independent transparency...
* Integration with other GPU APIs such as CUDA/CuPy.


## v0.3.2 (2025-07-XX) [UPCOMING]

### Backends

* Experimental (and slow) PyQt Datoviz backend, available on the [experiments repository](https://github.com/datoviz/experiments/blob/main/qt/pyqt_offscreen.py).

### Examples

* Add colormaps feature example.
* Use fixed random seed in Python examples.

### Python API

* Add `dvz.colormaps()`, a function returning the list of all supported colormaps.
* Fix topology enum in App Python wrapper for basic visual creation.

### Colormaps

* Fix colormap scaling with variable-length color palettes.

### Shapes

* Fix `shape_transform` in ShapeCollection ([#106](https://github.com/datoviz/datoviz/pull/106) by @kshitijgoel007).

### C API

* Mouse functions now return a DvzMouseEvent.

### Build

* Fix nightly releases, prune old nightly releases weekly on GitHub Actions.

---


## v0.3.1 (2025-07-07) [LATEST RELEASE]

This is a minor release featuring a few additions, improvements, performance benchmarks and bug fixes, mostly a new **Wiggle** visual, several new examples including a **triangle splatting showcase example**, a few new features such as a **colorbar**, a **3D gizmo**, a **3D horizontal grid**, a **fly controller**, support for camera orbiting and experimental panel linking.

### Shapes

* Added several methods to `ShapeCollection`: `add_guizmo()`, `transform()`, `vertex_count()`, `index_count()`.

* Fixed texture orientation on shapes ([#96](https://github.com/datoviz/datoviz/pull/96) by @ron-adam).

* Add support for anisotropic scaling in shape collection ([#104](https://github.com/datoviz/datoviz/pull/104) by @kshitijgoel007).


### Examples

#### Showcase examples

* Added choropleth showcase example.
* Added triangle splatting showcase example.

#### Feature examples

* Added animation feature example.
* Added basic shape feature example.
* Added colorbar feature example.
* Added transparent mesh feature example.
* Added sphere texture feature example.
* Added stop feature example.
* Added timer feature example.


### Visuals

#### Wiggle

* Added a new **Wiggle** visual for displaying multichannel time series.

#### Basic

* Added a shape argument to the **Basic** visual.

#### Segment

* Fixed visual bug with segments in 3D (issue [#90](https://github.com/datoviz/datoviz/issues/90), pull request [#91](https://github.com/datoviz/datoviz/pull/91) by @ron-adam).

#### Path

* Fixed smooth color and linewidth gradient in **Path** visual (issue [#90](https://github.com/datoviz/datoviz/issues/90)).

#### Sphere

* Implemented textured spheres ([#89](https://github.com/datoviz/datoviz/pull/89) by @ron-adam).


#### Mesh

* Improved Python API for setting ambient, diffuse, specular, and emission parameters.


### Axes

* Implemented black or white background in axis component.


### Graphical components

* Implemented a preliminary colorbar.
* Implemented a 3D horizontal grid.
* Implemented a 3D gizmo to visualize arcball orientation with three colored arrows.


### GUI

* Added integer versions of the GUI slider widgets.


### Controllers

* Added a new fly controller.


### User events

* Added `last_pos` field in mouse drag event.


### Python API

* Wrapped the `frame` and `resize` event in the Python API.
* Added `Panel.orbit()` to easily create an animation with the camera orbiting around a fixed point.
* Added `Panel.link()` to link two panels together (experimental).
* Added fullscreen mode ([#102](https://github.com/datoviz/datoviz/pull/102) by @ron-adam).
* Fix bug with textre upload ([#105](https://github.com/datoviz/datoviz/pull/105) by @ron-adam).


### Performance

* Added support for a `DVZ_MAX_FPS` environment variable to limit the maximum frame rate and reduce GPU usage (defaults to 200 FPS, can be disabled with `DVZ_MAX_FPS=0`).

* Implemented and ran a preliminary performance comparison benchmark of Datoviz vs Matplotlib, showing an up to 10,000x speedup.

### Documentation

* Made various improvements.


### Build

* Implemented nightly builds on the `dev` branch on GitHub Actions.


---

## v0.3.0 (2025-05-28)

This is a major release featuring interactive 2D axes, a new user-friendly Pythonic API, significantly improved documentation and gallery, many additional examples, and numerous fixes and enhancements.


### Python API

* Introduced a new **Pythonic API** built on top of the raw `ctypes` bindings (`dvz.App`).
* Added `dvz.ShapeCollection()` for easy creation and manipulation of 2D/3D shapes.
* Rewrote all examples to use the new high-level API.
* Reformatted all Python code using [ruff](https://docs.astral.sh/ruff/) for consistency.


### Axes & ticks

* Added support for **2D axes**, including automatic tick positioning and optional value factoring.
* Implemented standalone **ticks** and **labels** components.
* Introduced axis rendering and layout within panels.


### Documentation

* Significantly improved the Python documentation, particularly the visual components.
* Added documentation for the new Python API.
* Added a gallery.
* Improved the documentation website based on mkdocs-material.


### Shapes

* Added support for new 2D and 3D shapes:
  * **2D**: sector, histograms
  * **3D**: sphere, cone, cylinder, 3D arrow, torus, five Platonic solids


### Examples

* Reorganized examples into categories: `visuals`, `features`, `showcase`.
* Added many new examples demonstrating the new Pythonic API, visuals, and features.
* Added a utility function `download_data()` to download example data.


### Visuals

#### Glyph

* Added `dvz_glyph_strings()` helper.


#### Image

* Improved **image visual** with respect to anchoring options and rendering modes.


#### Sphere

* Fixed scaling and lighting in the **sphere visual** ([#84](https://github.com/datoviz/datoviz/pull/84) by @ron-adam).


#### Mesh

* More powerful light system in the **mesh visual**, shared with the **sphere visual**, with initial materials support ([#86](https://github.com/datoviz/datoviz/pull/86) by @ron-adam)


### GUI

* Added a tree GUI widget


### API

* Renamed several functions for a more consistent API.
* Implemented various bug fixes and rendering improvements.
* Started minor internal refactor to better support upcoming WebAssembly and WebGPU backends.


---

## v0.2.3 (2025-03-26)

### Visuals

* Fixed lighting and positioning in sphere visual ([#80](https://github.com/datoviz/datoviz/pull/80) by @ron-adam).

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


---

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


---

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


---

## v0.2.0 (2024-08-05)

- Full internal architecture rewrite.
- Internal asynchronous message-based Datoviz Intermediate Protocol.
- Auto-generated Python bindings based on ctypes instead of Cython.
- Prebuilt binary Python wheels for Linux (manylinux), macOS, Windows.


---

## v0.1.0 (2021-02-17) [DEPRECATED]

- First experimental release.
- Cython bindings.
