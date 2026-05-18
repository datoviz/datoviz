# Scene Grid Layout Spec

> **Execution Status**
> - **Status:** `DESIGN NOTE`
> - **Updated on:** `2026-05-18`
> - **Purpose:** define a retained grid/subplot layout layer for scene panels, including resize
>   behavior and future dashboard boundaries.


## Summary

Datoviz currently supports explicit scene panel rectangles through `DvzPanelDesc`:

```c
DvzPanel* panel = dvz_panel(
    figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 0.5f, .height = 1.0f});
```

That is the right low-level primitive for frame-plan emission because scene lowering can pass the
normalized rectangle directly to DRP2 viewport and scissor commands. What is missing is a retained
layout intent above those rectangles: rows, columns, spans, margins, gutters, weights, aspect
constraints, and future automatic adornment space.

The proposed grid layer should compute `DvzPanelDesc` values from semantic subplot placement. It
should not create a parallel render path, a new panel kind, or a dashboard widget model.


## Goals

1. Keep `DvzPanelDesc` as the renderer-facing panel rectangle contract.
2. Add a retained grid/subplot abstraction that can recompute panel rectangles when the figure size
   changes.
3. Support common scientific visualization layouts: regular subplot grids, unequal row/column
   sizes, spanning cells, fixed-width colorbar columns, fixed-height overview panels, and stable
   pixel gutters.
4. Leave room for axis labels, tick labels, titles, legends, and colorbars without forcing the
   first implementation to solve full automatic text measurement.
5. Keep dashboard layout separate from scene layout. A dashboard tile may contain a Datoviz figure,
   but it is not itself a scene panel.


## Non-Goals

1. Do not replace explicit `dvz_panel(figure, DvzPanelDesc)` placement.
2. Do not make scene panels draggable, dockable, or widget-like.
3. Do not implement a full constraint solver in the first slice.
4. Do not couple the C scene API to the future Python GSP/VisPy2 object model.
5. Do not require users to manually recompute every grid panel after a window resize.


## Existing Foundation

The current scene path already has the rendering pieces needed by a layout resolver:

1. `DvzPanelDesc` stores a normalized figure-space rectangle.
2. `DvzPanel` owns that descriptor.
3. Scene emission carries the descriptor into frame-plan render nodes.
4. DRP2 emission converts render nodes into viewport/scissor commands.
5. App resize already synchronizes the figure logical size before frame emission.
6. Panel controllers already receive panel pixel viewport information derived from the descriptor.

The missing piece is retained layout metadata plus a dirty/recompute step before emission.


## Core Model

Use three layers:

1. **Rectangle panel.** The existing `DvzPanelDesc` remains the low-level escape hatch and test
   primitive.
2. **Grid spec.** A retained layout object stores semantic grid intent and resolves it into panel
   rectangles.
3. **Higher-level plotting/dashboard layers.** GSP/VisPy2 can expose richer APIs above the C grid
   layer. Dashboard docking and widgets should remain above scene figures.

Conceptually:

```text
DvzFigure
  DvzGridSpec
    row/column definitions
    margins and gutters
    panel cell attachments
  DvzPanel
    current resolved DvzPanelDesc
```


## Units

Grid sizing should distinguish flexible and fixed dimensions.

| Unit | Meaning | Use |
|---|---|---|
| `WEIGHT` | share of the remaining space | regular subplot columns/rows |
| `PIXEL` | fixed logical-pixel size | colorbar columns, controls strips, stable gutters |
| `FRACTION` | fixed fraction of the figure | simple proportional layouts |
| `AUTO` | measured from adornments | future axes, labels, legends, colorbars |

The first implementation should support `WEIGHT` and `PIXEL`. `FRACTION` is convenient but not
essential. `AUTO` should be reserved in the type model even if it initially behaves like a default
weight or fixed value.


## Resize Behavior

On resize, the grid spec should be re-resolved from the current figure size into fresh panel
rectangles.

The intended flow is:

```text
window resize
  -> app updates figure size
  -> figure layout is marked dirty
  -> grid spec resolves using the new width and height
  -> grid-owned panel descriptors are updated
  -> controllers receive the new panel pixel viewport
  -> next dvz_figure_emit() emits updated viewport/scissor commands
```

Pixel units should be resolved before normalization. For example, with fixed margins and gutters:

```text
content_width = figure_width - margin_left - margin_right - gutter_x * (cols - 1)
column_width = content_width * column_weight / sum_column_weights
x_norm = x_px / figure_width
w_norm = w_px / figure_width
```

This keeps an 8 px gutter visually stable across window sizes. A purely normalized gutter would
become too large on small windows and too small on large windows.

The user-facing API should not require this pattern:

```c
/* Avoid requiring users to do this manually after every resize. */
dvz_panel_set_desc(panel, dvz_grid_resolve_panel(&grid, width, height, row, col, rs, cs));
```

Instead, grid-owned panels should remember their cell attachment and update when the figure layout
is dirty.


## Proposed C API Shape

The exact names are provisional, but the API should look like this class of operations:

```c
DvzGridSpec grid = dvz_grid_spec(2, 3);

dvz_grid_set_margins(&grid, 48.0f, 16.0f, 36.0f, 12.0f); /* left, right, bottom, top px */
dvz_grid_set_gutter(&grid, 8.0f, 8.0f);                  /* x, y px */

float col_weights[] = {1.0f, 2.0f, 1.0f};
float row_weights[] = {1.0f, 1.0f};
dvz_grid_set_col_weights(&grid, 3, col_weights);
dvz_grid_set_row_weights(&grid, 2, row_weights);

DvzPanel* main = dvz_panel_grid(figure, &grid, 0, 0, 1, 2);
DvzPanel* side = dvz_panel_grid(figure, &grid, 0, 2, 2, 1);
DvzPanel* hist = dvz_panel_grid(figure, &grid, 1, 0, 1, 2);
```

For implementation, prefer a figure-owned retained layout object over a stack-only grid copied by
value into panels. A stack-only helper is useful for quick rectangle computation, but retained
resize behavior needs ownership:

```c
DvzGridSpec* grid = dvz_figure_grid(figure, 2, 3);
DvzPanel* panel = dvz_grid_panel(grid, 0, 0, 1, 1);
```

The implementation can still provide pure resolver helpers for tests:

```c
bool dvz_grid_resolve(
    const DvzGridSpec* grid, uint32_t width, uint32_t height,
    DvzGridCell cell, DvzPanelDesc* out);
```


## Panel Mutation

A grid implementation needs an internal way to update a panel rectangle after creation. That can be
an internal helper first:

```c
bool _scene_panel_set_desc(DvzPanel* panel, DvzPanelDesc desc);
```

If exposed publicly, the API should be explicit that changing the descriptor updates panel
viewport/scissor on the next emit and invalidates controller viewport state:

```c
bool dvz_panel_set_desc(DvzPanel* panel, DvzPanelDesc desc);
```

Validation should reject non-finite values, non-positive extents, negative origins, and rectangles
that cannot produce a meaningful pixel viewport. Whether to allow rectangles partly outside
`[0, 1]` should be decided deliberately; the default should probably reject them for grid-owned
panels.


## Adornments And Plot Rects

The current `DvzPanelLayoutReserve` reserves visual-space room inside one panel for axes, legends,
colorbars, and future adornments. A grid spec should complement this rather than replace it:

1. Grid margins and gutters place outer panel rectangles in figure space.
2. Panel layout reservations and axis margins define the inner plot area in panel visual space.
3. Future automatic layout can measure text/colorbar needs and feed either grid-level fixed rows or
   panel-level reservations.

This preserves the useful distinction between:

```text
figure layout: where panels live
panel layout: where plot content lives inside a panel
```


## Scientific Visualization Use Cases

The grid layer should make these layouts easy:

1. Small multiples with shared axes and hidden duplicate tick labels.
2. Image grids with equal pixel aspect and shared colorbars.
3. Main view plus marginal histograms.
4. 3D panel plus linked orthographic slices.
5. Large point cloud view plus overview/minimap inset.
6. Volume rendering panel plus transfer-function editor panel.
7. Fixed-width legend/colorbar columns next to flexible plots.
8. Mixed 2D/3D dashboards where each render panel has independent controller state.


## Shared Axes And Links

GridSpec should not own domain linking directly, but it should make common shared-axis patterns easy
to express from a higher layer. The likely split:

1. Grid placement creates panels and records row/column membership.
2. A separate interaction/link API links domains, panzoom state, selections, or cameras.
3. Higher-level APIs can apply defaults such as "share x within each column" or "share y within
   each row".

This keeps layout orthogonal to interaction state.


## Dashboard Boundary

Scene grid layout and dashboard layout are related but not identical.

A scene panel is a render region inside one figure. A dashboard tile or dockspace item is a UI
container that may contain:

1. a Datoviz figure,
2. a rendered offscreen viewport,
3. controls,
4. tables,
5. logs,
6. inspectors,
7. multiple figures.

The current GUI viewport API already draws this boundary: an ImGui viewport can render a figure, but
it is not a scene `DvzPanel`. Keep that separation.

Recommended hierarchy:

```text
Dashboard
  Tile: 3D view
    Figure
      Panel: main 3D scene
      Panel: inset minimap
  Tile: controls
  Tile: histogram figure
    Figure
      Panel: histogram
      Panel: colorbar
```

This lets dashboards use docking/resizable UI layout while figures keep deterministic scientific
subplot layout.


## Implementation Slices

1. Add pure `DvzGridSpec` resolver tests for margins, gutters, weights, spans, and resize.
2. Add retained figure-owned grid objects and grid-owned panel attachments.
3. Add internal panel descriptor mutation with validation and dirty marking.
4. Recompute dirty figure layouts before `dvz_figure_emit()` and request processing.
5. Add examples: two-panel split, image grid with colorbar column, main plot plus marginal
   histogram.
6. Add shared-axis convenience only after the layout layer is stable.
7. Add automatic adornment measurement later, once text/annotation rendering has enough metrics.


## Open Questions

1. Should grid objects be public opaque handles or value descriptors copied into the figure?
2. Should fixed sizes use logical pixels only, or also physical pixels after DPI scaling?
3. Should grid-owned panels be allowed to overlap for insets, or should insets be a separate API?
4. Should out-of-bounds rectangles be rejected universally or only for retained grid layout?
5. How much of shared-axis behavior belongs in C versus GSP/VisPy2?
6. Should `AUTO` rows/columns wait for rendered text metrics, or support explicit user-supplied
   measured sizes first?
