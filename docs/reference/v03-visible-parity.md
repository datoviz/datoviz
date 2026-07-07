# v0.3 Visible Parity

This audit classifies visible v0.3-era capabilities for the v0.4 release candidate. It does not
promise source or ABI compatibility with v0.3. The active v0.4 API is the C scene/app path, the
Python `import datoviz as dvz` path with NumPy array uploads, and the lower-level `datoviz.raw`
binding for exact C calls. High-level Python plotting belongs above Datoviz in GSP/VisPy2.

Status values:

- `fixed`: covered by the v0.4 scene/app surface, examples, tests, or public docs.
- `experimental`: active v0.4 surface, but not required as a v0.3 visible parity blocker.
- `deferred`: intentionally not part of the v0.4 release surface.
- `external/GSP`: owned by GSP, VisPy2, Matplotlib, or an application layer above Datoviz.

| Visible capability | Disposition | v0.4 route | Notes |
| --- | --- | --- | --- |
| Retained scene, figure, panel, and visual workflow | fixed | `dvz_scene()`, `dvz_figure()`, panels, retained visuals | The API is redesigned; parity is behavioral, not source-compatible. |
| Native GLFW/app presentation | fixed | App/view runtime and `examples/c/runtime/app_glfw.c` | Datoviz-owned presentation remains in the runtime foundation. |
| Offscreen rendering and PNG/screenshot capture | fixed | `DvzView`/canvas capture and `examples/c/runtime/offscreen_capture.c` | v0.4 capture contract is sRGB RGBA8. Linear `f16`/`f32` scientific readback is deferred. |
| Frame callbacks and timer animation | fixed | callback/timer examples and portable scenario frame callbacks | Browser/WASM support is limited to the promoted experimental subset. |
| Multi-panel figures, linked panels, and panel backgrounds | fixed | panel grid/multi/link/background examples | Dense dashboards and high-level layout authoring remain above the core renderer. |
| Panzoom controller | fixed | `dvz_panzoom()` and panzoom examples | Native path is supported; WebGPU follows the promoted live-route subset. |
| Arcball, fly, and turntable controllers | fixed | controller examples | Native path is release-facing; browser parity is experimental. |
| 2D axes, ticks, labels, and plot-like scientific composition | fixed | axes, axis-label, scientific-plotting, and linked-panel examples | Scene-managed nonlinear/geographic transforms are deferred; CPU pre-projection is the supported pattern. |
| Text blocks and annotations | fixed | semantic text, glyph lowering, annotation/readout examples | Complex shaping, glyph/text picking, and broader world/data placement remain deferred. |
| Color maps, scales, colorbars, and categorical legends | fixed | sampled-field, colorbar, colormap-scale, and legend examples | First release slice is active; richer shared layout and query payloads remain follow-up work. |
| Scale bars, guide lines/spans, overlays, and readouts | fixed | scale-bar, guide, overlay, and probe-label examples | These are v0.4 semantic/composite routes, not restored v0.3 helper APIs. |
| Point visual | fixed | `dvz_point()` and point examples | Scalar color/diameter_px modes, shifts, and richer selection styling are deferred. |
| Pixel visual | fixed | `dvz_pixel()` and pixel examples | Scalar color mode and data-space pixel sizing are deferred. |
| Marker visual and marker symbols | fixed | `dvz_marker()` plus built-in symbol ids | Built-in code-SDF vocabulary covers the v0.3 marker set plus target; exact SDF picking and atlas variants remain deferred. |
| Primitive/basic visual | fixed | `dvz_primitive()` | v0.3 `basic` is renamed and narrowed as a low-level primitive family. |
| Segment and path visuals | fixed | `dvz_segment()`, `dvz_path()`, path join/curve examples | Dashes, arrow caps, closed-path API, and richer path/span picking are deferred. |
| Vector visual | fixed | `dvz_vector()` and wind/vector examples | Independent head dimensions and scalar styling are deferred. |
| Image visual and 2D sampled fields | fixed | `dvz_image()`, `DvzSampledField`, image probe examples | Tiled/LOD image policy and richer probe payloads are deferred. |
| Mesh, textured mesh, material mesh, and OBJ loading | fixed | `dvz_mesh()`, material/textured mesh examples, OBJ loading | Face/region picking, full PBR, and a broader public geometry-resource API remain deferred. |
| Sphere visual | fixed | `dvz_sphere()` and sphere examples | Texture variants and per-item PBR are deferred. |
| Volume visual and 3D sampled fields | fixed | `dvz_volume()`, volume, volume-occlusion, and brain-volume examples | Slice/MIP/composite paths are active. Isosurfaces, MPR, categorical label volumes, and DVR/MIP ray-hit picking are deferred. |
| Picking, selection, image probing, and readback | fixed | unified query API, picking/selection/probe examples | First broad item/sample paths are active. Rich mesh face, text/glyph, path/span, and volume ray identities are deferred. |
| Retained data updates, partial updates, transforms, and visibility | fixed | update, partial-update, transform, and visibility examples | Broader span/group source APIs remain future work unless a family documents support. |
| Raster video export | experimental | experimental video example | Active, but not a required v0.3 visible parity blocker. |
| Scene compute and compute-to-render particles | experimental | experimental compute+graphics slice | Active as a v0.4 feature, not a v0.3 visible parity requirement. |
| WebGPU/WASM browser rendering | experimental | experimental WebGPU subset | The browser route is promoted for selected examples but is not native Vulkan parity. |
| DRP2/DVZR command streams, fixtures, and replay | experimental | advanced/unstable contributor surface | Useful for runtime authors; not a v0.3 user-facing parity requirement. |
| Error bars, box plots, statistical plotting helpers, and high-level trace APIs | external/GSP | compose from primitives or use GSP/VisPy2 | Datoviz may expose low-level visuals/composites, but statistical plotting policy belongs above the renderer. |
| Old object-oriented Python plotting API | external/GSP | GSP/VisPy2 | Datoviz v0.4 does not recreate the v0.3 Python API. |
| Publication-quality PDF/SVG/vector export | external/GSP | GSP/Matplotlib | Datoviz owns interactive GPU rendering and raster capture. |
| Notebook, dashboard, napari, and application workflows | external/GSP | GSP/VisPy2/application integrations | Datoviz should provide backend and hosting capabilities, not high-level workflow APIs. |
| Custom visual/render shader replacement and hot reload | deferred | future custom visual/provider route | Built-in shader ABI is internal for v0.4. |
| Scene-managed nonlinear/geographic coordinate transforms | deferred | CPU pre-projection before upload | The renderer accepts ordinary uploaded positions; domain transforms belong in a future scene/GSP layer. |

For implementation-level gaps by family, see [Visual families](visual-families/index.md). The
implementation-facing scene API snapshot lives in `spec/scene/api/API_IMPLEMENTATION_READINESS.md`.
