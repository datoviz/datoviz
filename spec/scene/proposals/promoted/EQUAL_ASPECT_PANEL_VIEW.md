> **Execution Status**
> - **Status:** `PROMOTED / IMPLEMENTED IN V0.4-DEV`
> - **Updated on:** `2026-06-13`
> - **Canonical areas:** `../../core/PANEL_LAYOUT.md`, `../../pipeline/TRANSFORM_PIPELINE.md`,
>   `../../interaction/CONTROLLERS.md`, `../../semantics/AXES.md`, and
>   `../../api/API_SURFACE.md`.

# Equal-Aspect Panel View

This note preserves the promoted decision record for equal-aspect 2D panel views. Active behavior
belongs in the canonical scene specs and the implementation.


## Decision

The panel 2D view owns the resolved controller-visible extent. Panzoom controllers own semantic
navigation state: pan, zoom, limits, interaction baselines, and gesture policy. They do not encode
panel aspect by mutating raw zoom.

During frame planning, each panel evaluates the active controller against its current plot rectangle
and view-fit policy to produce:

1. the panel apply MVP;
2. the current visible view extent;
3. DATA-to-VIEW normalization;
4. inverse view-to-DATA domains for axes, grids, scale bars, and queries.

This preserves the scene transform split:

```text
DataSpace -> ViewSpace -> controller/panel view -> ClipSpace
```


## Public Shape

The implemented public API uses panel view-fit names as the primary names and keeps the old
view-fit names:

1. `DvzPanelViewFit`, `DvzPanelViewFitMode`, and `DvzPanelViewAspectMode`;
2. `dvz_panel_view_fit()`, `dvz_panel_set_view_fit()`, `dvz_panel_clear_view_fit()`, and
   `dvz_panel_view_extent()`;
3. domain-fit aliases are removed before the v0.4 release surface.

Coordinate spaces are split so users can choose whether equal aspect applies:

1. `DVZ_COORD_VIEW`: metric panel view coordinates, affected by equal-aspect view fit;
2. `DVZ_COORD_DATA`: data/domain coordinates, mapped through panel DATA -> VIEW;
3. `DVZ_COORD_PANEL`: normalized panel coordinates, intentionally viewport-shaped.


## Implementation

Primary implementation points:

1. public types: `include/datoviz/scene/types.h`;
2. coordinate-space enum: `include/datoviz/scene/enums.h`;
3. public declarations: `include/datoviz/scene.h`;
4. panel view resolver and attachment MVP composition: `src/scene/core/panel_geometry.c`;
5. panel-aware panzoom evaluation: `src/controller/panzoom.c`;
6. view-fit compatibility wrappers: `src/scene/annotation/axis.c`.

The resolver computes both fitted DATA domains and the base VIEW extent. With equal aspect enabled,
wide panels use `[-plot_aspect, +plot_aspect] x [-1, +1]`; tall panels use
`[-1, +1] x [-1 / plot_aspect, +1 / plot_aspect]`. The invariant is that one VIEW unit maps to the
same pixel scale along X and Y.

`DVZ_COORD_PANEL` attachments intentionally keep the old normalized-panel behavior and may stretch
with the panel rectangle.


## Linked Panels

Linked panzoom panels share semantic controller state, then each panel resolves that state against
its own plot rectangle and view-fit policy. For different panel aspect ratios, identical X extent,
identical Y extent, and equal screen scale cannot all hold at once.

The implemented priority is:

1. explicit `EXTENT_X` links preserve the linked X visible extent;
2. explicit `EXTENT_Y` links preserve the linked Y visible extent;
3. equal-aspect panels expand the unlinked dimension per panel;
4. full XY extent identity is valid only when panel plot aspects match or aspect is free.


## Validation

Focused tests cover:

1. equal-aspect DATA domains and resize behavior in `src/scene/tests/axis.c`;
2. equal-aspect axes/grid alignment in `src/scene/tests/axis.c`;
3. DATA-coordinate visual transforms after resize in `src/scene/tests/scene_interaction_graph.c`;
4. `DVZ_COORD_VIEW` versus `DVZ_COORD_PANEL` frame-plan MVP behavior in
   `src/scene/tests/scene_interaction_graph.c`;
5. linked panzoom extent behavior in `src/scene/tests/panzoom_arcball.c`.


## Remaining Cleanup

The core resolver lives in `src/scene/core/panel_geometry.c`, but the public view-fit wrappers and
domain-fit compatibility wrappers still live in `src/scene/annotation/axis.c`. Moving those wrappers
to a core panel API file would improve module ownership, but this is cleanup, not active proposal
work.
