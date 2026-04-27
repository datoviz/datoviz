# Panel Layout

This document defines how panels are positioned and sized within a figure in the future scene
layer.


## Purpose

The panel layout system should:

1. cover the common case (uniform grid of panels) with minimal user effort,
2. support fixed-size columns and rows for colorbar and legend slots,
3. support row/column span for asymmetric multi-panel figures,
4. support free placement as an escape hatch for non-grid arrangements,
5. remain responsive — proportional layouts adapt when the figure is resized,
6. expose tight layout as an explicit manual call, not an automatic constraint.


## Core Rule

A panel is a logical viewport region occupying a rectangular area of the figure surface.

Panel positions are always expressed in **normalized figure coordinates** `[0, 1]` (x right,
y down, origin at top-left), which makes all proportional layouts automatically responsive to
figure resize.

**Margins** (the inset between a panel's outer boundary and its inner renderable area, used
to reserve space for axis labels and titles) are expressed in **pixels** — they have a fixed
physical size regardless of figure dimensions.


## Two Placement Modes

### Free Placement

```text
panel = dvz_panel(fig, x, y, w, h)   // normalized [0,1], matches v0.3 semantics
```

`x, y` is the top-left corner; `w, h` is the size.
All four values are in normalized figure coordinates.

Free placement is the escape hatch for non-grid layouts: inset panels, overlapping panels,
and panels whose positions are derived externally.

Inset panels are just free-placement panels with coordinates inside another panel's area.
No special API is needed.


### Grid Layout

```text
DvzGridOpts opts = {
    .spacing   = 0.01,        // inter-panel gap, normalized units
    .margin    = {50, 20, 50, 60},  // outer margin: top, right, bottom, left — pixels
}
DvzGrid* grid = dvz_figure_grid(fig, rows, cols, &opts)
```

`dvz_figure_grid` partitions the figure into a rows×cols grid and returns a `DvzGrid*` handle.
Each cell maps to one panel, accessible via:

```text
panel = dvz_grid_panel(grid, row, col)
```

Row and column indices are zero-based.
Multiple grids may exist in one figure (for mixed-layout figures), but the common case is one.


#### Column Widths And Row Heights

By default all columns have equal width and all rows have equal height.

Override with relative weights or fixed pixel sizes:

```text
dvz_grid_col_width(grid, col, DVZ_SIZE_WEIGHT, 2.0)   // twice as wide as default
dvz_grid_col_width(grid, col, DVZ_SIZE_FIXED_PX, 60)  // exactly 60 px wide
dvz_grid_row_height(grid, row, DVZ_SIZE_WEIGHT, 1.0)  // default weight
dvz_grid_row_height(grid, row, DVZ_SIZE_FIXED_PX, 40) // exactly 40 px tall
```

`DVZ_SIZE_WEIGHT` values are relative to each other — `2.0` and `1.0` means the first column
is twice as wide as the second, with remaining space split proportionally.
`DVZ_SIZE_FIXED_PX` columns/rows consume their declared pixel size before proportional
distribution of the remainder.


#### Row/Column Span

A panel can span multiple contiguous grid cells:

```text
panel = dvz_grid_panel_span(grid, row, col, row_span, col_span)
```

`row_span = 1, col_span = 1` is the default (single cell).
A panel spanning multiple cells occupies the union of those cells including any inter-cell
spacing.

Cells covered by a span that are not the origin cell should not also be used by other panels.
The scene does not enforce this, but overlapping spans produce undefined visual results.


#### Colorbar And Legend Slots

A colorbar or legend is a fixed-size column or row adjacent to a data panel.

The standard pattern is a fixed-pixel column (or row) for the colorbar, with the data column
carrying the remaining proportional weight:

```text
// 2-column grid: data panel (proportional) + colorbar (60 px fixed)
DvzGrid* grid = dvz_figure_grid(fig, 1, 2, &opts)
dvz_grid_col_width(grid, 0, DVZ_SIZE_WEIGHT,    1.0)   // data panel
dvz_grid_col_width(grid, 1, DVZ_SIZE_FIXED_PX,  60)    // colorbar slot

DvzPanel* data_panel  = dvz_grid_panel(grid, 0, 0)
DvzPanel* cbar_panel  = dvz_grid_panel(grid, 0, 1)
```

The colorbar visual is attached to `cbar_panel` as a normal visual.
No special colorbar-panel type is needed — it is an ordinary panel with a fixed-width column.

A convenience wrapper is available that encapsulates the grid setup above:

```text
DvzPanel* cbar_panel = dvz_panel_attach_colorbar(data_panel, DVZ_PANEL_SIDE_RIGHT, 60)
```

`dvz_panel_attach_colorbar` creates a fixed-width column or row adjacent to the given panel
and returns a `DvzPanel*` for the colorbar. `DVZ_PANEL_SIDE_RIGHT`, `_LEFT`, `_TOP`, and
`_BOTTOM` select the side; the second argument is the width (or height) in pixels.

For a shared colorbar spanning multiple data rows, use a row-span:

```text
DvzGrid* grid = dvz_figure_grid(fig, 3, 2, &opts)
// columns: 0 = data, 1 = colorbar
dvz_grid_col_width(grid, 1, DVZ_SIZE_FIXED_PX, 60)
// colorbar panel spans all 3 rows
DvzPanel* cbar_panel = dvz_grid_panel_span(grid, 0, 1, 3, 1)
```


### ImGui-Driven Layout

A panel can be bound to a Dear ImGui window so that the panel viewport is determined each
frame by the ImGui window's current position and size.

```text
dvz_panel_gui(panel, "Window Title", flags)
```

This is the **third layout mode**, alongside grid and free placement.

In ImGui-driven mode:
1. an ImGui window with the given title is created and managed by ImGui,
2. the user can move and resize that window with the mouse,
3. each frame, the scene reads the ImGui window's current rect and calls
   `dvz_panel_resize()` to update the panel viewport,
4. the `FramePlan` rebuilds with the updated viewport on the next frame.

Docking is enabled with `DVZ_GUI_FLAGS_DOCKING`, which allows panels to be snapped into
ImGui dock spaces and moved collectively.

**Layout serialization** for ImGui-driven panels is handled automatically by ImGui via
`imgui.ini`, which persists window positions and sizes across sessions.
Datoviz does not need its own serialization mechanism for this layout mode.

ImGui-driven panels and grid/free-placement panels may coexist in the same figure.
The grid allocates space for grid panels; ImGui-driven panels float freely over the figure
surface in their own ImGui windows.

See `EXTERNAL_UI.md` for the full ImGui rendering architecture and input routing details.


## Panel Margins

Each panel has inner margins that inset the renderable area from the panel boundary.
Margins reserve space for axis tick labels, axis titles, and figure titles.

```text
dvz_panel_margins(panel, top, right, bottom, left)  // pixels, carries forward from v0.3
```

Default margins are system-defined fixed values (e.g., 10 px top/right, 50 px bottom/left
to accommodate typical axis labels).
Users may set explicit margins to override the defaults.

Margins are **not** updated automatically.
See the Tight Layout section below.


## Panel Fixed Aspect Ratio

A panel's renderable area (inside margins) can be constrained to a fixed pixel aspect ratio:

```text
dvz_panel_set_aspect_ratio(panel, width_over_height)   // e.g., 1.0 for square
dvz_panel_set_aspect_ratio(panel, 0)                   // 0 = unconstrained (default)
```

When a fixed aspect ratio is set:
1. the panel resizes to the largest rectangle with that ratio that fits inside its allocated
   grid cell (after margins),
2. excess space is left empty (not redistributed to neighbors).

This is distinct from `DVZ_ASPECT_EQUAL` on a panzoom controller, which is a data-space
constraint. A panel fixed aspect ratio is a layout constraint — it controls the panel's pixel
shape regardless of the data domain.

Typical uses: geographic maps (1:1 or geographic ratio), square heatmaps, image panels.


## Shared-Width Constraint

When two panels in different rows share an X controller (linked panzoom), their column widths
should be identical so that data positions align horizontally across panels.

```text
dvz_grid_link_col_width(grid, col_a, col_b)
```

This forces columns `col_a` and `col_b` to always have the same computed pixel width.
The constraint is satisfied during layout computation before each render when the figure is
resized.

Similarly for shared Y axes across rows:

```text
dvz_grid_link_row_height(grid, row_a, row_b)
```


## Panel Visibility

```text
dvz_panel_set_visible(panel, true)   // show
dvz_panel_set_visible(panel, false)  // hide — panel does not render, space still allocated
```

Hiding a panel does not change the layout — its allocated space remains reserved.
This preserves stable positions of neighboring panels.


## Tight Layout

Tight layout adjusts panel margins to prevent axis labels and titles from being clipped or
overlapping neighboring panels.

It is a **manual one-shot call**:

```text
dvz_figure_tight_layout(fig)
```

It is **not applied automatically**.
Default margins are fixed system values.

Reasons tight layout is manual:
1. Axis tick label widths change when the user pans or zooms (tick values change).
   Automatic margin updates would make panel sizes jump on every pan gesture.
2. Some figures require pixel-identical panel sizes across subplots (papers, multi-figure
   layouts) where automatic adjustment would break consistency.
3. Real-time or animated figures would rerun tight layout every frame, which is expensive
   and produces unstable layout.

**When to call it**: after setting up the full figure, setting all domains and axis labels,
and before export or display. Do not call it inside an animation loop.

Tight layout measures the rendered extents of axis tick labels and titles (using font metrics)
and sets panel margins to the minimum values that prevent clipping.
It operates on the current axis state; if domains change significantly later, call it again.


## Panel Size Constraints

**Minimum panel size:** 16 × 16 logical pixels. The layout engine silently clamps any panel to
this minimum on extreme resize rather than producing a zero-size panel.

**Clip regions:** Panel clipping is always rectangular (scissor). Non-rectangular or circular
clip regions are not supported.


## Responsive Behavior On Resize

All proportional (weight-based) column widths and row heights are recomputed when the figure
is resized.
Fixed-pixel sizes remain constant; the remaining space is redistributed proportionally.

The layout system recomputes panel viewport rectangles on every resize event before the next
frame build.
No user call is required.

Tight layout margins are **not** recomputed on resize.
If the figure is resized significantly, the user should call `dvz_figure_tight_layout` again.


## v0.3 Correspondence

| v0.3 | v0.4 |
|---|---|
| `dvz_panel(fig, x, y, w, h)` | unchanged — free placement in normalized coords |
| `dvz_panel_resize(panel, x, y, w, h)` | unchanged |
| `dvz_panel_margins(panel, t, r, b, l)` | unchanged, now explicit default values documented |
| `dvz_panel_default(fig)` | `dvz_grid_panel(grid, 0, 0)` for single-panel figures, or `dvz_panel(fig, 0, 0, 1, 1)` |
| — | `dvz_figure_grid` — new |
| — | fixed-size columns/rows — new |
| — | row/column span — new |
| — | `dvz_panel_set_aspect_ratio` — new |
| — | `dvz_grid_link_col_width` / `dvz_grid_link_row_height` — new |
| — | `dvz_figure_tight_layout` — new |
