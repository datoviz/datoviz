# Next scene-layer examples — implementation brief

**Priority: MEDIUM — continue after the first scene examples.**

This note tracks the next scene examples after the first point/scatter slice. All examples use the
**scene + app** layer (`dvz_scene`, `dvz_figure`, `dvz_panel`, `dvz_app`).

---

## Overview

| Example             | Visual needed         | New API required?            | Status  |
|---------------------|-----------------------|------------------------------|---------|
| `visuals/point.c`   | `dvz_point` (exists)  | None — richer use of existing | Done    |
| `visuals/primitive.c`  | `dvz_primitive`       | New topology-parametric family | Done    |
| `visuals/image.c`   | `dvz_image`           | New visual constructor        | Done    |
| `visuals/mesh.c`      | `dvz_mesh`            | Existing mesh visual          | Done    |
| `visuals/path.c`      | path-as-line/strip    | Existing path helper          | Done    |
| `visuals/image.c`     | sampled field + image | Existing field/image path     | Done    |
| `techniques/pick_hover.c` | point picking + app | Existing request/app path | Done    |
| `techniques/image_probe.c` | image probe + app | Existing request/app path | Done    |
| `visuals/image.c` | sampled-field update + app | Existing field update path | Done |
| `techniques/multi_panel.c` | multi-panel + app | Existing panel/controller path | Done |
| `techniques/linked_panels.c` | linked panzoom panels + app | Existing panel/controller path | Done |

The first scene example wave now exists under `examples/c/`. The next example work should focus less
on adding another visual constructor and more on native pressure tests: an interactive 3D mesh/depth
scene, clearer manual request smoke tests, and resize/capture/status behavior.

---

## Part 1 — `visuals/point.c` (scene + app, many points)

### Status

Done. `examples/c/visuals/point.c` renders 1 000 random colored points through the scene/app path.

### Goal

Show `dvz_point` used for real scientific data: a scatter plot of ~1 000 random 2-D points,
each with an independent colour and size.  Demonstrates `dvz_visual_set_data` with non-trivial
arrays and the normalised coordinate system.

### Data layout

```c
#define N 1000
float pos[N * 3];    /* x, y, z=0 in [-1, 1] */
uint8_t color[N * 4];  /* RGBA */
float size[N];       /* point diameter_px in pixels */
```

Fill with random values (use `dvz_rand_float()` or plain `rand()`).

### Skeleton

```c
DvzScene*  scene  = dvz_scene();
DvzFigure* fig    = dvz_figure(scene, 800, 600, 0);
DvzPanel*  panel  = dvz_panel_full(fig);
DvzVisual* visual = dvz_point(scene, 0);

dvz_visual_set_data(visual, "position", pos,   N);
dvz_visual_set_data(visual, "color", color, N);
dvz_visual_set_data(visual, "size",  size,  N);
dvz_panel_add_visual(panel, visual);

DvzApp* app = dvz_app(scene);
dvz_app_run(app, 1);
dvz_app_window_capture_png(dvz_app_window(app, fig), "visuals/point.png");

dvz_app_destroy(app);
dvz_visual_destroy(visual);
dvz_figure_destroy(fig);
dvz_scene_destroy(scene);
```

Check `visuals/point.c`, `visuals/point.c`, and `src/scene/tests/test_scene.c` for the current
attribute names accepted by `dvz_point`.

### CMakeLists addition

```cmake
dvz_add_example(visuals/point)
```

---

## Part 2 — `visuals/primitive.c` (scene + app, primitive visual)

### Status

Done. `examples/c/visuals/primitive.c` renders a single colored triangle through `dvz_primitive`
with `DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST`. The visual family supports POINT_LIST, LINE_LIST,
LINE_STRIP, TRIANGLE_LIST, TRIANGLE_STRIP. Scene tests cover triangle-list and line-strip emit.

The full contract is specified in [`spec/scene/visuals/PRIMITIVE.md`](../../spec/scene/visuals/PRIMITIVE.md);
this section summarises what the example needs.

The scene layer currently only exposes `dvz_point`.  Rather than a per-shape family
(`dvz_triangle`, `dvz_line`, `dvz_strip`, …), add **one** topology-parametric family with
built-in pass-through shaders:

```c
DVZ_EXPORT DvzVisual* dvz_primitive(
    DvzScene* scene, DvzPrimitiveTopology topology, uint32_t flags);
```

`DvzPrimitiveTopology` is a public scene-layer enum (per the spec):

```
DVZ_PRIMITIVE_TOPOLOGY_POINT_LIST
DVZ_PRIMITIVE_TOPOLOGY_LINE_LIST
DVZ_PRIMITIVE_TOPOLOGY_LINE_STRIP
DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
```

(No `TRIANGLE_FAN`, no indexed rendering — indexed geometry belongs to the future `mesh`
family.)

Why one family, not many: the visual is just `position + color → standard MVP transform →
varying color → fragment color`.  Nothing else differs between a triangle list and a line
strip except the topology constant fed to the pipeline.  A single family covers triangles,
lines, strips, fans, and raw points all at once with no CPU-side preprocessing and no
shader specialisation.

This is **not** a "raw" or "custom shader" escape hatch.  Users who need their own shaders
go through the lower-level `examples/c/advanced/raw_triangle_vklite.c` or
`examples/c/advanced/raw_triangle_drp2.c`
(DRP2 stream) paths, which are already in the tree.

### Attributes

- `"position"` — `float[N][3]` vertex positions
- `"color"` — `uint8_t[N][4]` per-vertex RGBA

(Match the existing `dvz_point` attribute names so users moving between families don't have
to relearn the API.)

### Implementation notes

1. Add `DVZ_VISUAL_TYPE_PRIMITIVE` to `DvzVisualType` in `src/scene/_scene.h` (alongside
   the existing POINT/PIXEL/MARKER/… entries).
2. Store the topology in the visual struct; `src/scene/converter.c` (`_resolve_pipeline`
   path around line 1340) already branches on `DvzVisualType` to pick shaders + topology —
   add a `DVZ_VISUAL_TYPE_PRIMITIVE` arm that uses the new pass-through shader pair and
   forwards the visual's topology to `VkPipelineInputAssemblyStateCreateInfo.topology`.
3. Built-in shaders are trivial:
   ```glsl
   /* vertex */
   #version 450
   layout(location=0) in vec3 inPos;
   layout(location=1) in vec4 inColor;
   layout(location=0) out vec4 vColor;
   void main() { gl_Position = vec4(inPos, 1.0); vColor = inColor; }

   /* fragment */
   #version 450
   layout(location=0) in vec4 vColor;
   layout(location=0) out vec4 outColor;
   void main() { outColor = vColor; }
   ```
   Wire up the standard MVP UBO once the panel transform path lands; until then leave
   positions in clip space (consistent with the current `dvz_point` story).
4. Reference the current point visual path in `src/scene/scene.c`,
   `src/scene/converter.c`, and `src/scene/tests/test_scene.c`.

### Example

`visuals/primitive.c` itself is then a few lines: hard-code three vertices, call
`dvz_primitive(scene, DVZ_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0)`, set data, run one
offscreen frame, save PNG.

Once the family exists, follow-up examples (`hello_lines.c`, `hello_strip.c`) reuse the
same constructor with a different topology — no new visual code required.

---

## Part 3 — `visuals/image.c` (scene + app, image visual)

### Status

Done. `examples/c/visuals/image.c` renders a 16×16 procedural RGBA texture on a quad covering
most of the panel. The visual accepts `"position"` (four clip-space corners, TRIANGLE_STRIP
order TL/BL/TR/BR), `"texcoords"` (four UV pairs), and a texture supplied via
`dvz_visual_set_texture_rgba8(visual, pixels, width, height)`.

Scene tests cover both CPU-side emit (stream is non-empty, no diagnostics) and app-layer
pixel readback (`test_app_offscreen_image_has_nonblank_pixels` checks that a solid-red texture
produces red-dominant pixels in the captured frame).

`dvz_image` is implemented in `src/scene/scene.c`; the converter arm (sampler + bind group
creation, texture upload emission) lives in `src/scene/converter.c`.

---

## Existing code to read before starting

- `examples/c/visuals/point.c` — minimal scene+app skeleton
- `examples/c/visuals/point.c` — non-trivial point data through the same path
- `src/scene/tests/test_scene.c` — shows attribute names, data shapes, and the full
  scene→figure→panel→visual→app flow used in tests
- `src/scene/scene.c` and `src/scene/converter.c` — existing point visual implementation/emission path
- `src/app/app.c` — `dvz_app()` internals
- `src/app/status.c` and `src/app/trace.c` — live FPS/status and DRP2 tracing

---

## Guide updates needed

Follow-up guide updates should add or refresh sections in `docs/guide/c.md`:

- Add `just example-c visuals/point` etc. to the build snippet
- Add sections for the current scene examples with `--8<--` source includes.
- Extend the *Key public APIs* table with scene/app request, field, mesh, and controller functions.
- Add one manual-interactive subsection for GLFW examples so expected mouse/hover behavior is clear.
