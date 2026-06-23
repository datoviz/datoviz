# Scene Grid Layout Spec

> **Execution Status**
> - **Status:** `IMPLEMENTED BASELINE / DASHBOARD FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-25`
> - **Purpose:** record the landed retained grid/subplot layout layer for scene panels and the
>   remaining dashboard/adornment-layout boundaries.


## Summary

Current grid and panel layout rules are canonical in
[`../core/PANEL_LAYOUT.md`](../core/PANEL_LAYOUT.md). This note preserves landed context and
dashboard follow-up boundaries; do not use it as a second grid-layout source of truth.

Datoviz currently supports explicit scene panel rectangles through `DvzPanelDesc`:

```c
DvzPanel* panel = dvz_panel(
    figure, (DvzPanelDesc){.x = 0.0f, .y = 0.0f, .width = 0.5f, .height = 1.0f});
```

That remains the right low-level primitive for frame-plan emission because scene lowering can pass
the normalized rectangle directly to DRP2 viewport and scissor commands. The active v0.4 code now
adds retained grid layout intent above those rectangles: rows, columns, spans, margins, gutters,
weight/fixed sizing, and automatic resize-driven re-resolution for grid-owned panels.

The grid layer computes `DvzPanelDesc` values from semantic subplot placement. It does not create a
parallel render path, a new panel kind, or a dashboard widget model.


## Goals

1. Keep `DvzPanelDesc` as the renderer-facing panel rectangle contract.
2. Use the retained grid/subplot abstraction to recompute panel rectangles when the figure size
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
7. `DvzGrid` stores retained rows, columns, margins, gutters, per-row/per-column sizing, and
   grid-owned panel cell attachments.
8. Grid-owned panels are re-resolved before frame emission when figure size or grid layout changes.


## Core Model

Use three layers:

1. **Rectangle panel.** The existing `DvzPanelDesc` remains the low-level escape hatch and test
   primitive.
2. **Grid.** A retained layout object stores semantic grid intent and resolves it into panel
   rectangles.
3. **Higher-level plotting/dashboard layers.** GSP/VisPy2 can expose richer APIs above the C grid
   layer. Dashboard docking and widgets should remain above scene figures.

Conceptually:

```text
DvzFigure
  DvzGrid
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

The active implementation supports `WEIGHT` and fixed logical-pixel sizing. `FRACTION` and `AUTO`
remain future layout extensions.


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
  -> next frame artifact emission contains updated viewport/scissor commands
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


## C API Shape

The installed API uses a figure-owned retained grid object:

```c
DvzGrid* grid = dvz_figure_grid(figure, 2, 3);

dvz_grid_set_margins(grid, &(DvzPanelReserve){
    .left_px = 48.0f, .right_px = 16.0f, .bottom_px = 36.0f, .top_px = 12.0f});
dvz_grid_set_gutter(grid, 8.0f, 8.0f);

dvz_grid_col_size(grid, 0, DVZ_GRID_SIZE_WEIGHT, 1.0f);
dvz_grid_col_size(grid, 1, DVZ_GRID_SIZE_WEIGHT, 2.0f);
dvz_grid_col_size(grid, 2, DVZ_GRID_SIZE_WEIGHT, 1.0f);
dvz_grid_row_size(grid, 0, DVZ_GRID_SIZE_WEIGHT, 1.0f);
dvz_grid_row_size(grid, 1, DVZ_GRID_SIZE_WEIGHT, 1.0f);

DvzPanel* main = dvz_grid_panel_span(grid, 0, 0, 1, 2);
DvzPanel* side = dvz_grid_panel_span(grid, 0, 2, 2, 1);
DvzPanel* hist = dvz_grid_panel_span(grid, 1, 0, 1, 2);
```

The implementation also provides a pure resolver helper for tests and tooling:

```c
bool dvz_grid_resolve(
    const DvzGrid* grid, uint32_t width, uint32_t height,
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

Panel padding, manual pixel reserve, and automatic adornment reserves define plot space inside one
panel. A grid spec should complement this rather than replace it:

1. Grid margins and gutters place outer panel rectangles in figure space.
2. Panel padding and reserve bands define the inner plot area in logical pixels.
3. Future automatic layout can measure text/colorbar needs and feed either grid-level fixed rows or
   panel-level reserves.

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

1. Landed: pure resolver tests for margins, gutters, weights, spans, and resize.
2. Landed: retained figure-owned grid objects and grid-owned panel attachments.
3. Landed: internal panel descriptor mutation with validation and dirty marking.
4. Landed: dirty figure layouts are recomputed before frame emission.
5. Remaining: add examples for image grids with shared colorbar slots and main plot plus marginal
   histogram.
6. Remaining: add shared-axis convenience only after linked-domain semantics are stable.
7. Remaining: add automatic adornment measurement later, once text/annotation metrics and
   categorical legends are mature.


## Open Questions

1. Should fixed sizes use logical pixels only, or also physical pixels after DPI scaling?
2. Should grid-owned panels be allowed to overlap for insets, or should insets be a separate API?
3. Should out-of-bounds rectangles be rejected universally or only for retained grid layout?
4. How much of shared-axis behavior belongs in C versus GSP/VisPy2?
5. Should `AUTO` rows/columns wait for rendered text metrics, or support explicit user-supplied
   measured sizes first?
