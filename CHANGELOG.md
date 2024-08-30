# Change Log

## Development version

### Scene

- Implemented arcball GUI with sliders for rotation angles.
- Ortho interactivity (like panzoom but with fixed aspect ratio).

### Visuals

- Added open/close option in `path` visual.
- Added functionality in `basic` visual (point size, groups).
- Added new _experimental_ `monoglyph` visual (pure GLSL low-quality glyph generation), works on macOS but not on Linux (?)
- Implemented automatic data update of dirty visuals.

#### Image visual

- Improved the `image` visual API (position is now specified in normalized device coordinates, the size in pixels, the anchor in relative coordinates).
- Added rescaling options (with aspect ratio kept or not).
- Added rounded corners and stroke options.
- Added fill option to replace the texture by a uniform color.

#### Mesh

- Mesh wireframe option.
- Polygon triangulation with the earcut library.
- Polygon contour.

### Text

- Added feature to generate a texture containing a string, using a given font.
- Implemented multiline support in text rendering using the font API.

### GUI

- Added color picker.

### Documentation

- Added more documentation.
- Updated website `datoviz.org`.

### Miscellaneous

- Improved demo now showing a grid of visuals.
- Improved FPS computation.
- Added mock functions.
- Rename upper/lower to top/bottom in API.
- Fixed various minor bugs.


## v0.2.0 (2024-08-05) [LATEST RELEASE]

- Full internal architecture rewrite.
- Internal asynchronous message-based Datoviz Intermediate Protocol.
- Auto-generated Python bindings based on ctypes instead of Cython.
- Prebuilt binary Python wheels for Linux (manylinux), macOS, Windows.


## v0.1.0 (2021-02-17) [DEPRECATED]

- First experimental release.
- Cython bindings.
