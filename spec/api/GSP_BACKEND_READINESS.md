# Datoviz v0.4 GSP Backend Readiness

Status: RC-lane readiness checklist for Datoviz as a GSP/Matplotlib rendering target.

Purpose: make Datoviz v0.4 a stable, ergonomic rendering target for a future Matplotlib backend that lowers Matplotlib draw calls to GSP visuals and then to Datoviz.

---

## 0. Executive summary

The proposed Matplotlib backend will not call Datoviz directly from Matplotlib in the long term. It will use this path:

```text
Matplotlib Figure.draw(RendererGSP)
    -> RendererGSP emits GSP visuals/display list
    -> GSP renderer backend = Datoviz v0.4
    -> Datoviz retained scene + offscreen/interactive view
```

Datoviz v0.4 already has most of the right low-level concepts: retained scene, figures, panels, visual families, dense visual attributes, offscreen views, hosted-loop primitives, and Python `ctypes`/array-aware bindings. The work before RC should focus on making those APIs stable and ergonomic enough that GSP can depend on them without wrapping unstable internals.

The highest-value pre-RC changes are:

1. Stable Python API for retained-scene creation, visual creation, panel attachment, dense data
   updates, and app/view rendering.
2. Offscreen capture to memory (`RGBA`/PNG bytes), not only to a filesystem path.
3. Python-friendly wrappers for `dvz_visual_set_data_many()`; keep the existing
   `dvz_visual_set_data_range()` array facade in validation.
4. Clear, documented logical-pixel/device-pixel semantics for screen-space attributes.
5. Reliable panel clipping/scissor behavior for all attached visuals.
6. A stable line/path/polyline story suitable for Matplotlib `plot()` and `LineCollection`.
7. Hosted-loop primitives stable enough for later GUI/interactive backends.


## 0.1 Current v0.4 integration update

The current branch is closer to this plan than the original draft assumed:

- The public Python direction is already one generated `ctypes` binding with C-shaped `dvz_*`
  names: `import datoviz as dvz` for policy-declared NumPy adaptation, and `datoviz.raw` for exact
  pointers, counts, bytes, and callbacks.
  Do not add prefixless aliases such as `capture_rgba()` or `visual_set_data_many()`.
- `dvz_visual_set_data_many()` and `dvz_visual_set_data_range()` already exist in the C API.
  `dvz_visual_set_data_range()` is already array-adapted in the top-level Python package. The
  top-level `dvz_visual_set_data_many()` wrapper now accepts mappings or iterables of
  `(attr_name, array)` pairs and lowers them to `DvzVisualDataUpdate[]` for the raw call.
- Canvas-level memory capture already exists in C:
  `dvz_canvas_capture_rgba_into()` and `dvz_canvas_capture_rgba()`. The top-level Python package now
  exposes `dvz_view_capture_rgba(view)`, which reaches the canvas through `dvz_view_canvas()`, uses
  `dvz_canvas_capture_rgba_into()`, and returns a copied NumPy RGBA8 array without temporary files
  or Datoviz-owned memory lifetime hazards.
- PNG-to-memory is only partially aligned: `dvz_make_png()` writes RGB memory, while screenshot
  capture is RGBA8. Add an RGBA-capable PNG-bytes path or explicitly document alpha-dropping if
  RC1 defers alpha-preserving PNG bytes.
- Panel scissor/clip infrastructure is active in scene emission and runtime draw emission. Focused
  proof now covers adjacent panels with reserved plot bands and per-draw plot scissor commands.
- Path, image, and semantic text are already release-facing visual families. Use the current
  canonical APIs: `dvz_path(scene, flags)`, `dvz_image(scene, flags)` plus sampled fields, and
  semantic `dvz_text(panel, flags)` for text. Do not invent a parallel text-as-generic-visual API
  for RC.
- Hosted-loop primitives already include `dvz_view_render_once()`, `dvz_app_render_once()`,
  external-surface views, request-frame callbacks, and Qt bridge proof. Treat further GUI hosting
  as validation/documentation unless a specific missing primitive is found.

Revised pre-RC posture: make small Python binding/documentation additions around the existing C
surface. Avoid broad C API expansion before RC1 except for an alpha-preserving PNG-memory helper if
needed.


## 0.2 RC-lane completion snapshot

Completed in the current RC lane:

1. `dvz.dvz_visual_set_data_many(visual, updates)` accepts mappings and `(attr_name, array)`
   iterables in the top-level package, validates item counts before mutation, and preserves raw
   descriptor-array passthrough when `update_count` is supplied.
2. `dvz.dvz_view_capture_rgba(view)` returns Python-owned NumPy RGBA8 memory shaped
   `(framebuffer_height, framebuffer_width, 4)` with top-row-first rows.
3. `docs/reference/ctypes.md` documents the Python API, dense data updates,
   offscreen RGBA capture, and the GSP/VisPy2 boundary.
4. `examples/python/direct/offscreen_point.py` and `tools/bindings/ctypes_render_smoke.py` cover the
   direct facade offscreen point/capture path where native runtime support is available.
5. Logical-pixel, framebuffer-pixel, screen-space attribute, and RGBA capture semantics are
   documented in reference docs.
6. Adjacent-panel plot scissor proof is covered by
   `test_scene_adjacent_panels_plot_scissor_no_bleed`.
7. View2D/domain readback uses ordered endpoints, including reversed finite domains. Axis/grid
   generation uses explicit internal sorted intervals only where numeric low/high bounds are needed.
8. `DvzAxisTicks`, `dvz_axis_set_ticks()`, and `dvz_axis_clear_ticks()` provide exact explicit
   tick positions and optional copied labels. The top-level Python package accepts NumPy-compatible
   values and Python labels.
9. Query target scopes now distinguish deferred guide and all-rendered requests:
   `DVZ_SCENE_TARGET_GUIDE` and `DVZ_SCENE_TARGET_ALL_RENDERED` return
   `DVZ_QUERY_STATUS_UNSUPPORTED_TARGET` instead of degrading to data-only picking.

These slices make the first GSP/Matplotlib backend path viable without private Python modules and
without temporary files for ordinary RGBA capture.


## 0.2.1 GSP backend compatibility matrix

| Capability | Status | Public C API | Top-level Python | Tests/proof | Notes |
| --- | --- | --- | --- | --- | --- |
| View2D/domain readback | supported | `dvz_panel_set_domain()`, `dvz_panel_set_view2d()`, `dvz_panel_visible_domain()` | exposed through `datoviz` facade | `test_panel_view2d_reversed_domains` | Public domains are ordered endpoints, not sorted bounds. |
| Reversed finite domains | supported | same as View2D/domain | exposed | axis/panel View2D tests | Data mapping, visible-domain readback, axes, and grid use one oriented snapshot. |
| Explicit axis ticks | supported | `DvzAxisTicks`, `dvz_axis_set_ticks()`, `dvz_axis_clear_ticks()` | `dvz.dvz_axis_set_ticks(axis, values, labels=None)` | `test_axis_explicit_ticks_and_labels`, `testing/test_array_facade.py` | Values are exact data coordinates and keep caller order. |
| Explicit tick labels | supported | `DvzAxisTicks.labels` | Python labels supported | axis label copy tests, array facade tests | Labels are copied before the C setter returns. |
| Grid alignment | supported | axis tick state | exposed through axis API | `test_axis_explicit_reversed_ticks_grid_alignment` | Grid lines use the same render tick snapshot as tick marks. |
| Axis labels | supported | `dvz_axis_set_label()` | exposed | axis tests and text rendering tests | Uses the scene text/glyph pipeline. |
| Guide query | deferred, explicit | `DVZ_SCENE_TARGET_GUIDE` | enum exposed in `datoviz.raw`; top-level query APIs exposed | `test_scene_query_deferred_guide_targets_are_unsupported`, `ctypes_smoke.py` | Returns `DVZ_QUERY_STATUS_UNSUPPORTED_TARGET`. |
| All-rendered with guides | deferred, explicit | `DVZ_SCENE_TARGET_ALL_RENDERED` | enum exposed in `datoviz.raw`; top-level query APIs exposed | same query tests/smoke | No data-only fallback while guide picking is absent. |
| Data query payload completeness | supported/experimental by family | `DvzQueryResult` fields and visual-family query ops | `DvzQueryResult`, `dvz_panel_query_px()`, `dvz_scene_poll_query()` | `just test query`, `ctypes_smoke.py` | Point/marker/image/sample/mesh paths fill promoted fields; unsupported gaps report status. |
| Colorbar render | supported | `dvz_colorbar()`, title/format/orientation/anchor/layout setters | symbols probed by `array_facade_smoke.py` | `test_scene_scale_colormap_colorbar_core`, `test_scene_colorbar_auto_reserve_and_visuals`, `test_app_offscreen_colorbar_has_visible_ramp_and_labels` | Range comes from the shared `DvzScale`; explicit colorbar ticks are not a separate RC API. |
| Colorbar query | deferred | use guide/all-rendered query scopes | unsupported scopes exposed | deferred-target query test | No CPU fallback query for colorbar adornments. |
| Text render | supported | `dvz_text()`, `dvz_text_set_string()`, style/placement helpers | symbols and `DvzTextPlacement` probed by `array_facade_smoke.py` | semantic text and atlas tests under `scene/interaction` and `scene/text_atlas` | Placement may be screen, data, or world; offsets and size are logical pixels. |
| Text query | deferred for semantic text | `DVZ_SCENE_TARGET_TEXT` currently family/backend dependent | query APIs exposed | query unsupported/family tests | Use explicit unsupported statuses unless a promoted visual-family path handles the target. |
| Mesh render | supported | `dvz_mesh()`, `dvz_mesh_set_geometry()`, dense/index uploads | symbols probed by `array_facade_smoke.py` | mesh geometry/state tests and examples | Indexed triangle topology; supports `z=0` 2D overlays and 3D depth paths. |
| Mesh query | supported/experimental | query item targets on mesh visuals | query APIs exposed | `test_scene_mesh_query_resolves_item`, `test_scene_mesh_query_resolves_instance_item` | Face-level semantics beyond promoted item/instance payloads remain family-specific. |


## 0.3 Remaining RC/post-RC follow-up

Remaining work is optional for RC unless a downstream GSP integration finds a concrete blocker:

1. Add `dvz_view_capture_png_bytes(view)` only if alpha-preserving PNG bytes are required before
   RC; otherwise defer and keep RGBA memory as the stable backend target.
2. Keep direct Python binding validation in release evidence: `testing/test_array_facade.py`,
   `tools/bindings/ctypes_render_smoke.py`, and the adjacent-panel scissor test.
3. Add deeper family-specific docs only when needed by a real GSP lowering path, especially image
   upload/update convenience and semantic text update examples.
4. Do not add a Matplotlib backend, GSP implementation, or high-level plotting aliases inside
   Datoviz for v0.4 RC.

---

## 1. Scope and non-goals

### In scope before RC

Stabilize the Datoviz v0.4 surfaces needed by GSP:

- Python-level retained-scene API.
- Dense visual data update API.
- Offscreen rendering API.
- Basic screen-space visual semantics.
- Panel creation, sizing, clipping, axes/domain plumbing.
- Line/path, point, marker, image, text, mesh, segment primitives.
- Error handling and diagnostics for invalid visual updates.

### Not in scope before RC

Do not attempt to implement Matplotlib compatibility inside Datoviz.

Do not add a Matplotlib backend to Datoviz.

Do not promise Datoviz-native PDF/SVG/vector export. Matplotlib vector output should remain an Agg/Matplotlib fallback concern.

Do not block RC on perfect text/mathtext/TeX parity. The future Matplotlib backend can use Agg fallback for complex text.

---

## 2. Required API stability surface

The GSP Datoviz v0.4 backend should be able to rely on the following Python calls or close equivalents.

### Scene and layout

Required:

```python
import datoviz as dvz

scene = dvz.dvz_scene()
figure = dvz.dvz_figure(scene, width, height, flags)
panel = dvz.dvz_panel(figure, desc)       # normalized figure rect
panel = dvz.dvz_panel_full(figure)
```

Needed behavior:

- Scene owns figure, panels, visuals, and controllers.
- Object lifetimes are clear and documented.
- Destroying the scene releases all retained scene objects.
- Figure size can be updated after creation.
- Panel rect updates are legal and reflected on the next frame.

### Visual creation

Required visual families for first Matplotlib/GSP subset:

```python
point = dvz.dvz_point(scene, flags)
pixel = dvz.dvz_pixel(scene, flags)
marker = dvz.dvz_marker(scene, flags)
segment = dvz.dvz_segment(scene, flags)
mesh = dvz.dvz_mesh(scene, flags)
image = ...        # whichever retained v0.4 image API is canonical
text = ...         # whichever retained v0.4 text API is canonical
path_or_vector = ...
```

The public Python layer should expose only stable names. Avoid requiring imports from private modules or generated internals except `datoviz.raw` for low-level consumers.

### Visual attachment

Required:

```python
desc = dvz.dvz_visual_attach_desc()
desc.z_layer = z
# desc.controller_mode, coord_space, etc.
dvz.dvz_panel_add_visual(panel, visual, desc)
```

Needed behavior:

- Multiple visuals can be attached to one panel.
- A visual can be detached or destroyed cleanly.
- Attachment options expose at least `z_layer`, controller mode, and coordinate space.
- Panel clipping applies to attached visuals by default.

### Dense visual data updates

Required:

```python
dvz.dvz_visual_set_data(visual, "position", positions)
dvz.dvz_visual_set_data(visual, "color", colors)
dvz.dvz_visual_set_data(visual, "diameter_px", diameters)
```

The top-level Python package should accept NumPy arrays directly and infer pointer, dtype, shape, and item count from policy declarations.

Required attributes for the initial Matplotlib subset:

| Visual family | Required attributes |
|---|---|
| point | `position`, `color`, `diameter_px`, optional `item_state` |
| pixel | `position`, `color`, `pixel_size_px`, optional `item_state` |
| marker | `position`, `color`, `diameter_px`, `angle`, `shape` or `symbol`, optional `item_state` |
| segment | `position_start`, `position_end`, `color`, `stroke_width_px` |
| path/polyline | `position`, `color`, `stroke_width_px`, subpath/group metadata if applicable |
| mesh | `position`, optional `color`, optional `normal`, optional `texcoords`, optional index buffer |
| image | RGBA texture/data plus position/extent or textured quad attributes |
| text | `text`, `position`, optional `anchor`, `size`, `color`, `angle` |

---

## 3. High-priority pre-RC updates

## 3.1 Add memory-based offscreen capture

### Problem

A Matplotlib backend needs to implement APIs such as:

```python
canvas.buffer_rgba()
canvas.print_png(path_or_file)
canvas.tostring_argb()
```

It should not be forced to write a temporary PNG file and read it back.

### Required Datoviz additions

Add Python-level APIs equivalent to:

```python
rgba = dvz.capture_rgba(scene, figure, width, height)
png = dvz.capture_png_bytes(scene, figure, width, height)
```

or a stateful API:

```python
app = dvz.dvz_app(scene)
view = dvz.dvz_view_offscreen(app, figure, width, height)
dvz.dvz_app_run(app, 1)
rgba = dvz.dvz_view_capture_rgba(view)
png = dvz.dvz_view_capture_png_bytes(view)
```

### Requirements

- `capture_rgba()` returns a contiguous `np.ndarray` or `memoryview` with shape `(height, width, 4)` and dtype `uint8`.
- Document channel order: `RGBA`, not `BGRA`, not premultiplied unless explicitly named.
- Document origin: first row is top or bottom. Prefer top-left for image buffers if matching common display APIs; if bottom-left, expose an explicit flag.
- `capture_png_bytes()` returns `bytes`.
- No temporary file required.
- Safe to call repeatedly.
- Safe after `dvz_view_render_once()` or `dvz_app_run(app, 1)`.
- Fail with a clear Python exception if no GPU/context/offscreen support is available.

### Acceptance criteria

- A Python test creates a scene with one point visual, renders offscreen, obtains RGBA bytes/array, and verifies non-background pixels.
- The test does not create any temporary PNG file.
- A second render after a visual data update returns a different image.

---

## 3.2 Add Python-friendly `visual_set_data_many()`

### Problem

Matplotlib scatter/path collections update several attributes together:

- positions
- colors
- diameters/sizes
- edge/stroke widths
- marker shapes

Calling `dvz_visual_set_data()` sequentially can create transient invalid state when item counts change.

### Required Datoviz addition

Expose an ergonomic top-level Python wrapper while preserving the C-shaped name:

```python
dvz.dvz_visual_set_data_many(
    visual,
    {
        "position": positions,          # float32 (N, 3)
        "color": colors,                # uint8 (N, 4)
        "diameter_px": diameters,        # float32 (N,)
    },
)
```

`datoviz.raw.dvz_visual_set_data_many()` remains the exact `ctypes` descriptor-array call.

### Requirements

- Validate all arrays before mutating retained visual state.
- All per-item attributes must agree on item count.
- Return/raise clear errors for unsupported attribute names, wrong dtype, wrong shape, inconsistent counts.
- Copy semantics must match C API: input arrays may be reused/released after the call.
- Preserve raw `dvz_visual_set_data_many()` in `datoviz.raw` for low-level users.

### Acceptance criteria

- Test one call updates point position/color/diameter for N items.
- Test inconsistent item counts raise a deterministic exception and do not partially update the visual.
- Test wrong dtype gives a useful error message naming the offending attribute.

---

## 3.3 Add Python-friendly `visual_set_data_range()`

### Problem

Interactive Matplotlib and future VisPy2/GSP workflows may update slices of large arrays. Full re-upload every frame will waste CPU/GPU bandwidth.

### Required Datoviz addition

The current top-level package already exposes this C-shaped convenience:

```python
dvz.dvz_visual_set_data_range(
    visual,
    "position",
    first_item,
    positions_chunk,
)
```

### Requirements

- Attribute must already exist and have compatible total item count.
- Chunk shape determines `item_count`.
- Validate dtype and shape.
- Clear errors if range is out of bounds.
- Support all dense attributes used by point, marker, segment, image, mesh, and text as applicable.

### Acceptance criteria

- Test full allocation with `visual_set_data_many()` followed by a partial position update.
- Render before and after update and verify image changes.
- Test out-of-range update fails cleanly.

---

## 3.4 Freeze screen-space attribute semantics

### Problem

Matplotlib rendering is dominated by screen/logical-pixel quantities:

- marker size
- linewidth
- image extent
- text size
- dash lengths
- offset transforms
- clipping rectangles

Datoviz already has pixel-space attributes such as `diameter_px`, `pixel_size_px`, `stroke_width_px`, and text size/position. These semantics need to be fully documented and stable.

### Required documentation/spec updates

For every screen-space attribute, document:

- Is it in logical pixels or framebuffer pixels?
- Is it affected by device scale?
- Is it affected by `render_scale`?
- Is it affected by user scale?
- Is it affected by panel controllers?
- Which origin convention applies?
- Is positive Y up or down?

Recommended policy:

```text
Data positions: panel data/visual coordinates unless explicitly fixed/screen.
Screen sizes: logical pixels.
Framebuffer scale: handled by Datoviz view/device scale.
Matplotlib backend: converts Matplotlib display pixels to Datoviz logical pixel semantics.
```

### Acceptance criteria

- Docs include a table for point, marker, pixel, segment, image, text.
- A smoke test renders the same marker diameter under two device-scale settings and confirms expected behavior.

---

## 3.5 Stabilize panel clipping/scissor behavior

### Problem

Matplotlib clips nearly all axes content to the axes rectangle. If Datoviz visuals bleed outside panels, a GSP/Matplotlib backend cannot be correct.

### Required behavior

- Every visual attached to a panel is clipped/scissored to the panel plot rectangle by default.
- Panel background/border/chrome behavior is separate from data visual clipping.
- If panel reserve/padding is used, document whether data visuals clip to full panel rect or plot rect.
- There should be a way to opt out for overlays if already supported, but default should be safe for axes-like content.

### Acceptance criteria

- Test point/segment/image/text at positions outside panel bounds: outside portions are not visible.
- Test two adjacent panels: visuals in one panel do not draw into the other.

---

## 3.6 Provide a stable path/polyline story

### Problem

Matplotlib `plot()` and `LineCollection` need an efficient representation for stroked polylines. Rendering every line as many independent segments is acceptable for a prototype but bad for joins/caps and performance.

### Required outcome before RC

Use and document the canonical v0.4 API for stroked polylines/paths.

Current branch canonical form:

```python
path = dvz.dvz_path(scene, flags)
dvz.dvz_visual_set_data(path, "position", positions)
dvz.dvz_visual_set_data(path, "color", colors)
dvz.dvz_visual_set_data(path, "stroke_width_px", widths)
dvz.dvz_path_set_subpaths(path, lengths)
dvz.dvz_path_set_caps(path, start_cap, end_cap)
dvz.dvz_path_set_join(path, join, miter_limit)
```

### Minimum required features

- One or more subpaths.
- Open polylines.
- Constant color/width for first milestone; per-item/per-segment color can come later.
- Cap style: butt/round/square if available.
- Join style: miter/round/bevel if available.
- Panel clipping.
- Dense data mutation after creation.

### Acceptance criteria

- Python example draws a sine curve as one retained path visual.
- Python example draws a `LineCollection`-like set of 1000 short lines.
- Data updates mutate the same retained visual.

---

## 3.7 Stabilize image visual behavior

### Problem

Matplotlib `imshow()` and fallback layers both require reliable RGBA image upload and placement.

### Required features

- Upload `uint8 RGBA` arrays.
- Draw a 2D image in a panel with explicit extent.
- Support nearest and linear interpolation if available; nearest alone is acceptable for first backend if documented.
- Control origin (`upper`/`lower`) or document conversion required by caller.
- Support alpha.
- Support panel clipping.
- Support replacing image data without recreating the visual.

### Acceptance criteria

- Test an RGBA checkerboard image in a panel.
- Test updating image data in place.
- Test image extent and clipping.

---

## 3.8 Stabilize basic text visual behavior

### Problem

The Matplotlib backend can fallback for complex mathtext/TeX, but simple titles, labels, and tick text should eventually be native.

### Required features

- Text strings array.
- Pixel or panel-local positions.
- Font size in logical pixels or points with documented conversion.
- RGBA color.
- Anchor/alignment.
- Rotation angle.
- Panel clipping.
- Update text strings without recreating the scene.

### Acceptance criteria

- Python example draws three labels with different anchors.
- Python example updates label text and color.

---

## 3.9 Stabilize hosted-loop primitives

### Problem

The first backend may be offscreen-only, but interactive Matplotlib support will need a hosted event loop.

### Required stable C/Python primitives

- Create offscreen and GLFW views.
- Create external-surface hosted view.
- Emit resize, pointer, wheel, and key events.
- Register request-frame callback.
- Render one frame without running Datoviz's own loop.
- Wake/post callbacks from another thread if supported.

### Acceptance criteria

- Minimal Python example creates a view, calls render-once, then captures RGBA.
- Hosted-loop API names and signatures are documented as RC-stable or explicitly marked experimental.

---

## 4. Error handling and diagnostics

### Required improvements

All Python wrappers should raise typed exceptions instead of silently returning `-1`/`false` unless the user explicitly calls `datoviz.raw`.

Suggested exception hierarchy:

```python
class DatovizError(RuntimeError): ...
class DatovizValidationError(DatovizError): ...
class DatovizRuntimeError(DatovizError): ...
class DatovizGpuUnavailableError(DatovizRuntimeError): ...
```

Every failed data update should include:

- visual family;
- attribute name;
- expected dtype/shape;
- actual dtype/shape;
- item count if relevant.

---

## 5. Tests to add in Datoviz

Add tests under the existing Python test suite, or create a focused `tests/python/test_retained_scene_python.py` module.

### Required smoke tests

1. `test_scene_point_offscreen_rgba()`
   - Create scene/figure/panel/point.
   - Set position/color/diameter.
   - Capture RGBA bytes/array.
   - Verify shape and non-background pixels.

2. `test_visual_set_data_many_atomic_validation()`
   - Attempt inconsistent item counts.
   - Verify exception.
   - Verify retained visual state not partially changed.

3. `test_visual_set_data_range()`
   - Allocate N points.
   - Update a slice.
   - Capture before/after.

4. `test_panel_clipping()`
   - Draw outside panel.
   - Verify no bleed into adjacent panel.

5. `test_image_rgba_upload_update()`
   - Upload checkerboard.
   - Update to inverted checkerboard.
   - Verify render changes.

6. `test_segment_or_path_smoke()`
   - Draw polyline/segments.
   - Capture.

7. `test_text_smoke()`
   - Draw simple text.
   - Capture.

Tests that require GPU/offscreen support should be skipped with a clear reason if the environment cannot create a Datoviz offscreen view.

---

## 6. Documentation updates

Add a short page, e.g. `docs/reference/python-retained-scene.md`, containing:

- scene/figure/panel lifecycle;
- visual families and supported attributes;
- dense data update API;
- capture-to-memory API;
- coordinate and pixel semantics;
- offscreen rendering example;
- hosted-loop summary;
- known limitations.

Include one compact example equivalent to:

```python
import numpy as np
import datoviz as dvz

scene = dvz.dvz_scene()
figure = dvz.dvz_figure(scene, 800, 600, 0)
panel = dvz.dvz_panel_full(figure)

visual = dvz.dvz_point(scene, 0)
pos = np.random.uniform(-1, 1, (1000, 3)).astype("float32")
color = np.full((1000, 4), [255, 255, 255, 255], dtype="uint8")
diam = np.full(1000, 4, dtype="float32")

dvz.visual_set_data_many(visual, {
    "position": pos,
    "color": color,
    "diameter_px": diam,
})
dvz.dvz_panel_add_visual(panel, visual, None)

rgba = dvz.capture_rgba(scene, figure, 800, 600)
```

---

## 7. Implementation status

Completed RC-lane implementation:

1. Keep the existing generated binding model: top-level `datoviz` for NumPy-adapted `dvz_*` calls,
   and `datoviz.raw` for exact `ctypes` calls.
2. `dvz.dvz_visual_set_data_many(visual, {"attr": array, ...})` validates arrays, constructs
   `DvzVisualDataUpdate[]`, keeps temporaries alive through the raw call, and raises deterministic
   Python exceptions on validation failure.
3. `dvz.dvz_view_capture_rgba(view)` returns a `(height, width, 4)` `uint8` NumPy array using
   caller-owned Python memory.
4. Focused tests cover `set_data_many`, `set_data_range`, and capture memory;
   `tools/bindings/ctypes_render_smoke.py` runs raw and direct offscreen smoke examples when runtime
   support is available.
5. `docs/reference/ctypes.md` documents the Python API, dense data updates, and RGBA
   capture, with cross-links from status/reference pages.
6. `docs/reference/coordinate-systems.md` and `docs/reference/visual-attributes.md` document
   logical-pixel, framebuffer-pixel, screen-space attribute, panel clipping, and capture semantics.
7. `test_scene_adjacent_panels_plot_scissor_no_bleed` covers adjacent-panel plot scissor emission.

Optional follow-up: add `dvz.dvz_view_capture_png_bytes(view)` later if an alpha-preserving
PNG-memory path is required.

---

## 8. Definition of done

This document is complete when GSP can implement a Datoviz v0.4 renderer without relying on private Datoviz Python modules and without temporary files for ordinary PNG/RGBA capture.

Minimum acceptance:

- `import datoviz as dvz` exposes stable retained-scene functions.
- Python can create scene/figure/panel, create point/marker/segment/path/image/text content through
  the current canonical APIs, set NumPy data, attach visuals, render offscreen, and get RGBA memory
  without temporary files through `dvz_view_capture_rgba(view)`.
- Atomic multi-attribute updates are ergonomic from top-level Python through
  `dvz.dvz_visual_set_data_many(...)`.
- Partial updates remain available through `dvz.dvz_visual_set_data_range(...)`.
- Screen-space size semantics are documented as logical-pixel semantics unless a family explicitly
  says otherwise.
- Panel clipping/scissor behavior is documented and covered by focused DRP2 scissor proof for
  adjacent panels.
- Tests cover the above.
