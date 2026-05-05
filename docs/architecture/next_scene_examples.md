# Next scene-layer examples — implementation brief

**Priority: HIGH — main next step in the examples/ roadmap.**

Three new examples to add under `examples/c/`, building on the existing `hello_point.c`.
All three use the **scene + app** layer (`dvz_scene`, `dvz_figure`, `dvz_panel`, `dvz_app`).

---

## Overview

| Example             | Visual needed         | New API required?            | Status  |
|---------------------|-----------------------|------------------------------|---------|
| `hello_scatter.c`   | `dvz_point` (exists)  | None — richer use of existing | Ready   |
| `hello_triangle.c`  | `dvz_triangle` / mesh | New visual constructor        | Blocked |
| `hello_texture.c`   | `dvz_image`           | New visual constructor        | Blocked |

`hello_scatter.c` can be written immediately; the other two first need a new visual added to
the scene layer.

---

## Part 1 — `hello_scatter.c` (scene + app, many points)

### Goal

Show `dvz_point` used for real scientific data: a scatter plot of ~1 000 random 2-D points,
each with an independent colour and size.  Demonstrates `dvz_visual_set_data` with non-trivial
arrays and the normalised coordinate system.

### Data layout

```c
#define N 1000
float pos[N * 3];    /* x, y, z=0 in [-1, 1] */
uint8_t color[N * 4];  /* RGBA */
float size[N];       /* point diameter in pixels */
```

Fill with random values (use `dvz_rand_float()` or plain `rand()`).

### Skeleton

```c
DvzScene*  scene  = dvz_scene();
DvzFigure* fig    = dvz_figure(scene, 800, 600, 0);
DvzPanel*  panel  = dvz_panel(fig, dvz_panel_default());
DvzVisual* visual = dvz_point(scene, 0);

dvz_visual_set_data(visual, "pos",   pos,   N);
dvz_visual_set_data(visual, "color", color, N);
dvz_visual_set_data(visual, "size",  size,  N);
dvz_panel_add_visual(panel, visual);

DvzApp* app = dvz_app(scene);
dvz_app_run(app, 1);
dvz_app_window_capture_png(dvz_app_window(app, fig), "hello_scatter.png");

dvz_app_destroy(app);
dvz_visual_destroy(visual);
dvz_figure_destroy(fig);
dvz_scene_destroy(scene);
```

Check `hello_point.c` and `dvz_app()` docs for the exact `DvzApp` setup.  Check
`src/scene/tests/test_scene.c` for the attribute names accepted by `dvz_point`.

### CMakeLists addition

```cmake
dvz_add_example(hello_scatter)
```

---

## Part 2 — `hello_triangle.c` (scene + app, mesh visual)

### Blocked on: `dvz_triangle` or `dvz_mesh` visual constructor

The scene layer currently only exposes `dvz_point`.  Before writing this example, add a
minimal triangle / filled-polygon visual to the scene layer:

```c
DVZ_EXPORT DvzVisual* dvz_triangle(DvzScene* scene, uint32_t flags);
```

Expected attributes (analogous to the vklite raw example):
- `"pos"` — `float[3][2]` or `float[N][3]` vertex positions
- `"color"` — `uint8_t[N][4]` per-vertex RGBA

The implementation should emit the appropriate DRP2 pipeline + draw commands via the scene's
frame plan.  Reference: `src/scene/visuals/` (pattern used by the point visual).

Once the visual exists the example itself is straightforward: hard-code three vertices, set
data, run one offscreen frame, save PNG.

---

## Part 3 — `hello_texture.c` (scene + app, image visual)

### Blocked on: `dvz_image` visual constructor

Add a 2-D image visual to the scene layer:

```c
DVZ_EXPORT DvzVisual* dvz_image(DvzScene* scene, uint32_t flags);
```

Expected attributes:
- `"pos"` — four corner positions (a quad)
- `"texcoords"` — corresponding UV coordinates
- `"texture"` — a `DvzTexture*` or equivalent handle

The example would load a small test image (or generate a procedural RGBA buffer), upload it as
a texture, and display it via the visual.

For texture upload, check whether a public `dvz_texture_*` API already exists (search
`include/datoviz/` for `dvz_texture`); add one if not.

---

## Existing code to read before starting

- `examples/c/hello_point.c` — canonical scene+app skeleton
- `src/scene/tests/test_scene.c` — shows attribute names, data shapes, and the full
  scene→figure→panel→visual→app flow used in tests
- `src/scene/visuals/` — existing visual implementations to follow as a pattern
- `src/scene/app.c` — `dvz_app()` internals

---

## Guide updates needed

Once examples are written, add sections to `docs/guide/c.md`:

- Add `just example-c hello_scatter` etc. to the build snippet
- Add Example 4 / 5 / 6 sections with `--8<--` source includes (same style as existing examples)
- Extend the *Key public APIs* table with any new functions introduced
