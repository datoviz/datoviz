> **Execution Status**
> - **Status:** `FUTURE SCENE PROPOSAL / v0.5+`
> - **Updated on:** `2026-05-28`
> - **Purpose:** define the proposed coordinate-reference and span model for figure-level guides
>   that can cross panels while remaining linked to panel domains and controllers.
> - **Primary pressure tests:** linked time-series panels, image/scatter/histogram crosshairs,
>   orthoslice viewers, dashboards, and brushing/selection bands.

# Figure Guides And Cross-Panel Annotations


## Summary

Datoviz needs a future retained guide system for scientific annotations that are not naturally
owned by a single panel. Examples include shared time cursors across stacked plots, brushing bands
across linked panels, crosshairs shared by orthoslice views, and labels anchored to one panel's data
but displayed in a figure margin.

The right abstraction is not a one-off cross-panel cursor. It is a small system built from:

1. coordinate references that describe where a point or scalar comes from,
2. spans that describe where a guide may extend,
3. guide marks that turn references and spans into visible lines, bands, labels, brackets, or
   callouts.

This proposal is deliberately future-scoped. It should not block Datoviz v0.4 feature freeze.


## v0.4 Scope Decision

Figure-level guides are **not required for v0.4**.

The v0.4 release is already near feature freeze and should prioritize WebGPU/WASM scope, retained
textured mesh, Python binding, visible parity audit, API/status cleanup, and release example
proof. Existing v0.4 linked-panel examples may use panel-local synchronized bands or lines to show
shared domains, but they should not introduce a new public figure-guide API.

The first public guide slice should target v0.5 or later, after multiple examples prove the model.


## Non-Goals For The First Slice

The first guide slice should not attempt to provide:

1. a full dashboard layout system,
2. a DOM or CSS-like styling model,
3. arbitrary path routing or label collision solving,
4. text editing widgets,
5. full annotation-object picking,
6. draggable guides before static guide resolution is stable,
7. external GUI replacement behavior.


## Core Concepts

### Coordinate References

A coordinate reference answers: "where is this point or scalar anchored?"

Candidate reference kinds:

1. figure pixels,
2. normalized figure coordinates,
3. panel pixels,
4. panel plot normalized coordinates,
5. panel visual coordinates,
6. panel data coordinates,
7. axis value on a panel dimension.

For linked panels, the key primitive is an axis-value reference:

```c
DvzCoordRef x = dvz_ref_axis_value(panel, DVZ_DIM_X, 4.25);
```

This reference resolves through the panel's current visible domain and plot rectangle. If the panel
uses a shared X panzoom controller, the resolved pixel coordinate follows that controller
automatically.


### Spans

A span answers: "where may this guide extend?"

Candidate span kinds:

1. one full panel rectangle,
2. one panel plot rectangle,
3. the union of several panel plot rectangles,
4. a grid row, column, or cell range,
5. the full figure,
6. an explicit figure-pixel rectangle.

For a cursor crossing the left column of linked panels:

```c
DvzSpan y = dvz_span_plot_union(left_panels, 3);
```


### Guide Marks

A guide mark turns coordinate references and spans into geometry or text.

Candidate guide marks:

1. vertical line,
2. horizontal line,
3. X band,
4. Y band,
5. rectangle or region,
6. label,
7. bracket or measurement annotation,
8. callout leader line.

A cross-panel time cursor could look like:

```c
DvzGuideLayer* guides = dvz_figure_guides(figure);
DvzCoordRef x = dvz_ref_axis_value(top_panel, DVZ_DIM_X, 4.25);
DvzSpan y = dvz_span_plot_union(left_panels, 3);
dvz_guide_vline(guides, x, y, &cursor_style);
```


## Resolution Model

Guide synchronization should use a pull model during frame preparation, not callback copying.

For each frame:

1. resolve panel layout and plot rectangles,
2. resolve panel visible domains, including active panzoom/controller state,
3. resolve coordinate references into figure pixel coordinates,
4. resolve spans into figure pixel rectangles,
5. build figure-space guide geometry,
6. emit that geometry through the scene -> FramePlan -> DRP2 path.

For an X axis-value reference:

```c
double visible_min = 0.0;
double visible_max = 0.0;
dvz_panel_visible_domain(panel, DVZ_DIM_X, &visible_min, &visible_max);

DvzRect plot = {0};
dvz_panel_plot_rect_px(panel, &plot);

double t = (x - visible_min) / (visible_max - visible_min);
float x_px = plot.x + (float)t * plot.width;
```

This keeps synchronization derived from retained scene state. Panzoom, resize, grid changes, axis
reserve changes, DPI scale, and domain updates do not need per-feature event subscriptions.


## Rendering Model

Figure guides should render inside the scene path, not through ad hoc canvas drawing.

The first implementation may lower guides to generated fixed-position primitive, segment, image, or
text visuals. Longer term, guides may emit directly into a figure overlay render node. In either
case, they must remain visible to FramePlan validation, capture, replay, and future WebGPU
portability checks.

The guide layer should support explicit ordering:

1. below data,
2. above data and below axes,
3. above axes,
4. above all scene-native overlays and below external UI.

Clipping should also be explicit:

1. no clip,
2. figure clip,
3. span clip,
4. per-panel plot clip for panel-local guide variants.


## Relationship To Existing Systems

Panel-local annotations remain the right choice when a mark belongs to one panel and should be
clipped by that panel's plot or viewport.

Overlay cards remain the right choice for lightweight readout boxes, HUD text, and panel-attached
screen-space cards.

External UI remains the right choice for widgets, inspectors, menus, and application controls.

Figure guides fill the gap between these systems: semantic visualization marks that may cross panel
boundaries while still deriving their position from panel data domains and controllers.


## Scientific Visualization Use Cases

Useful guide scenarios include:

1. shared time cursor across traces,
2. stimulus or task epochs across physiology panels,
3. event markers spanning a stack of channels,
4. brushing bands across linked scatter, image, and histogram views,
5. synchronized crosshair across orthoslice panels,
6. ROI rectangles spanning a grid of image panels,
7. threshold lines tied to one panel's scale but drawn across a panel group,
8. measurement brackets and delta labels,
9. confidence interval bands,
10. chromatogram or spectrum peak markers,
11. quality-control warning regions,
12. labels anchored to one panel's data but displayed in a shared margin.


## Minimal v0.5 Slice

The first implementation-ready slice should be small:

1. internal `DvzCoordRef` and `DvzSpan` structs,
2. resolver helpers for figure pixels, panel plot rects, panel visible domains, and axis values,
3. a retained figure guide layer,
4. vertical line, X band, and label guide marks,
5. generated fixed primitive or segment visuals,
6. focused tests for resize, panzoom/domain updates, plot-union spans, and destroy lifecycle,
7. one C example proving static cross-panel guides.

Interactivity, dragging, guide picking, label routing, and richer mark families should follow only
after the static retained model is stable.


## Open Questions

1. Should public naming use `guide`, `figure_annotation`, or another term?
2. Should guides be owned only by figures, or should panels also expose local guide layers?
3. Should the first renderer use generated visuals or direct FramePlan overlay nodes?
4. How should guide clipping interact with axes, colorbars, and panel reserves?
5. Should guide labels reuse `DvzAnnotation` state or stay separate until text placement matures?
6. How much of the coordinate-reference resolver should be promoted into public API?
