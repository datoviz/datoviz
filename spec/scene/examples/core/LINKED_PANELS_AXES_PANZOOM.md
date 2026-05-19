# Example: Linked Panels With Shared X Axis And Independent Y Axes

> **Agent Pickup**
> - **Category:** `core`
> - **Implementation target:** Small runnable C example or focused scene/DRP2 regression on the active v0.4 path.
> - **Data policy:** Inline or deterministic synthetic data unless this file explicitly names a cache.
> - **Preprocessing:** None for the first slice; keep any later generator deterministic and checked in or documented.
> - **Validation:** Bounded smoke run plus screenshot/readback or fixture coverage when practical.


## Summary

Build a two-panel 2D scene that proves shared-x and independent-y panzoom behavior while keeping each
panel's axis resources separate. Use deterministic inline or synthetic scatter/time-series data, with no
preprocessing, so the first slice can focus on controller binding, visual normalization, tick regeneration,
and frame-plan contributions. The shared X controller should act only as a visible-domain source; each
panel still owns its own x/y axis geometry and labels. Validation should follow the bounded smoke or
fixture path, with screenshot/readback checks when practical to confirm linked X navigation, independent Y
navigation, and axis updates without visual-data renormalization.


This example traces the full axis pipeline — domain source, tick generation, covered-domain
check, and `FramePlan` contribution — across two panels that share a panzoom X controller and
have independent Y controllers.


## Owning Specs

This example should be read against:

1. `../../semantics/AXES.md` for axis derivation, regeneration policy, domain source, pull model, and panel linking,
2. `../../interaction/CONTROLLERS.md` for first-class controller handles, per-dimension binding, and shared-handle
   linking,
3. `../../decisions/CONTROLLER_BINDING_MODEL.md` for the accepted controller ownership and binding model,
4. `../../pipeline/TRANSFORM_PIPELINE.md` for the DataSpace / VisualSpace split,
5. `../../pipeline/FRAME_PLAN.md` for the `UploadNode` and `RenderNode` structure,
6. `../../pipeline/INVALIDATION_AND_CACHING.md` for dirty scope rules under panzoom.


## Scene Setup

1. one scene,
2. two 2D panels arranged vertically,
3. one `marker` visual in panel A (scatter points),
4. one `path` visual in panel B (time series line),
5. X and Y axes on each panel,
6. one shared X panzoom controller bound to both panels on `DVZ_DIM_X`,
7. two independent Y panzoom controllers, one per panel.


## Family And Variant

Visual families:

1. `marker` in panel A,
2. `path` in panel B.

Axis-related scene objects:

1. panel A: x axis (shared X controller), y axis (local Y controller),
2. panel B: x axis (shared X controller), y axis (local Y controller).

Controller configuration:

```text
shared_x  = dvz_panzoom(scene, 0)
local_y_a = dvz_panzoom(scene, 0)
local_y_b = dvz_panzoom(scene, 0)

dvz_panel_bind_controller(panel_a, shared_x,  DVZ_DIM_X)
dvz_panel_bind_controller(panel_a, local_y_a, DVZ_DIM_Y)
dvz_panel_bind_controller(panel_b, shared_x,  DVZ_DIM_X)
dvz_panel_bind_controller(panel_b, local_y_b, DVZ_DIM_Y)
```

Both panels' X axes will query `shared_x` for their visible domain.
Each panel's Y axis queries its own local controller.


## Resource Schema Instance

Panel A — marker:

1. source `ItemTable` in `DataSpace` (x = time, y = spike amplitude),
2. derived normalized `ItemTable` in `VisualSpace`,
3. `ParameterBlockResource` for marker size and color.

Panel B — path:

1. source `GroupedItemTable` in `DataSpace` (x = time, y = signal value),
2. derived normalized `GroupedItemTable` in `VisualSpace`,
3. `ParameterBlockResource` for stroke width and color.

Axis-derived resources (per panel per axis):

1. `segment` contributions for axis lines and tick marks,
2. `glyph` contributions for tick labels and axis title.

Each axis owns its own derived resource set.
Panel A's X axis and panel B's X axis maintain independent derived resources even though they
share a controller — the controller is a domain source only, not a geometry owner.


## Transform Pipeline

For marker and path visuals:

1. source data lives in `DataSpace`,
2. normalization maps coordinates into `VisualSpace` (`[-1, 1]` range),
3. panzoom applies after normalization as a panel-local transform,
4. navigation does not force renormalization of visual data.

For axes:

1. tick values are selected in `DataSpace` from the current visible domain,
2. tick geometry is built in `VisualSpace`,
3. the panel-local panzoom transform moves that geometry afterward.

The key split:

1. marker and path data survive panzoom unchanged — `VisualSpace` resources are stable,
2. axes may need semantic regeneration when the visible domain changes enough.


## Axis Lifecycle Trace

This section traces four representative frame states to make the pipeline explicit.


### State 1 — Initial Frame

The user has just provided data. No panzoom has occurred yet. The axis domain comes from a
one-time fit to the data extents.

Axis update step for panel A's X axis:

```text
// Query visible domain from shared_x controller
dvz_controller_query_domain(shared_x, DVZ_DIM_X, &xmin, &xmax)
// → xmin = 0.0, xmax = 10.0  (fit to data)

// No prior tick layout exists → regeneration is unconditional
tick_values = axis_generate_ticks(xmin, xmax, scale=LINEAR, density_target, panel_width)
// → [0, 2, 4, 6, 8, 10]

label_strings = axis_format_labels(tick_values, format=AUTO)
// → ["0", "2", "4", "6", "8", "10"]

tick_anchors_visual = axis_map_to_visual_space(tick_values, xmin, xmax)

axis_build_geometry(tick_anchors_visual, label_strings)
// → produces segment and glyph contributions, stored as derived resources
// covered_domain = [−1.0, 11.0]  (with margin beyond visible range)
```

The same logic runs for panel A's Y axis (querying `local_y_a`), panel B's X axis (querying
`shared_x` — same result as panel A), and panel B's Y axis (querying `local_y_b`).

`FramePlan` for the initial frame:

```text
UploadNode  → normalized marker data (panel A)
UploadNode  → normalized path data (panel B)
UploadNode  → X axis derived resources (panel A)
UploadNode  → Y axis derived resources (panel A)
UploadNode  → X axis derived resources (panel B)
UploadNode  → Y axis derived resources (panel B)
RenderNode  → panel A: marker + axis contributions
RenderNode  → panel B: path + axis contributions
```


### State 2 — Small Pan Within Covered Domain

The user pans right by a small amount. The visible X domain shifts to `[1.5, 11.5]`.

Axis update step for panel A's X axis:

```text
dvz_controller_query_domain(shared_x, DVZ_DIM_X, &xmin, &xmax)
// → xmin = 1.5, xmax = 11.5

// Check: is the visible domain inside the covered domain?
// covered_domain = [−1.0, 11.0]
// xmax = 11.5 > 11.0  → covered_domain does not fully contain visible domain
```

Wait — in this case the visible domain has only slightly exceeded the right edge. Whether this
triggers regeneration depends on the threshold policy. If `11.5` is within the allowed overshoot
tolerance the axis retains the current layout; if it crosses the hard threshold it regenerates.

Assume the threshold is not yet crossed (visible domain is only slightly outside the margin).
The axis retains its current derived resources and relies on the panel-local panzoom transform
to move tick geometry live.

Panel B's X axis queries the same `shared_x` controller and reaches the same decision.

`FramePlan` for this frame:

```text
// No UploadNodes — no axis resources are dirty
RenderNode  → panel A: marker + axis contributions  (panzoom moves geometry live)
RenderNode  → panel B: path + axis contributions    (panzoom moves geometry live)
```

The marker and path normalized data are also unchanged.
The only per-frame cost is the panel transform push for both panels.


### State 3 — Pan That Exits Covered Domain

The user continues panning. The visible X domain is now `[4.0, 14.0]`.
This clearly exits the covered domain `[−1.0, 11.0]` on the right side.

Axis update step for panel A's X axis:

```text
dvz_controller_query_domain(shared_x, DVZ_DIM_X, &xmin, &xmax)
// → xmin = 4.0, xmax = 14.0

// Covered domain check fails → regeneration required
tick_values = axis_generate_ticks(4.0, 14.0, ...)
// → [4, 6, 8, 10, 12, 14]

label_strings = axis_format_labels(tick_values, ...)
// → ["4", "6", "8", "10", "12", "14"]

tick_anchors_visual = axis_map_to_visual_space(tick_values, 4.0, 14.0)
axis_build_geometry(...)
// covered_domain updated to [2.0, 16.0]
// marks axis derived resources dirty → UploadNode required
```

Panel B's X axis queries the same `shared_x` and observes the same domain.
It independently triggers regeneration and marks its own derived resources dirty.

The two panels regenerate independently — they share a domain source but own separate derived
resources. Panel A's axis upload does not block or share with panel B's.

Y axes are unaffected: `local_y_a` and `local_y_b` have not changed.

`FramePlan` for this frame:

```text
UploadNode  → X axis derived resources (panel A)  ← regenerated
UploadNode  → X axis derived resources (panel B)  ← regenerated
RenderNode  → panel A: marker + axis contributions
RenderNode  → panel B: path + axis contributions
```

No upload for Y axes, marker data, or path data.


### State 4 — Y Zoom On Panel A Only

The user performs a vertical scroll gesture over panel A. Only `local_y_a` changes.
`shared_x`, `local_y_b`, and panel B are unaffected.

Axis update step for panel A's Y axis:

```text
dvz_controller_query_domain(local_y_a, DVZ_DIM_Y, &ymin, &ymax)
// → new compressed range, covered domain no longer sufficient
// → Y axis regeneration required for panel A
```

All other axes query unchanged controllers and pass their covered-domain checks.

`FramePlan` for this frame:

```text
UploadNode  → Y axis derived resources (panel A)  ← regenerated
RenderNode  → panel A: marker + axis contributions
RenderNode  → panel B: path + axis contributions
```

Panel B is not redrawn unless its own state changed.
Marker and path data are unchanged.
Panel B's Y axis retains its existing resources.


## FramePlan Shape Summary

| Trigger                          | UploadNodes                         | RenderNodes       |
|----------------------------------|-------------------------------------|-------------------|
| Initial frame                    | marker, path, all 4 axis sets       | panel A, panel B  |
| Small pan (within covered)       | none                                | panel A, panel B  |
| Large pan (exits covered domain) | X axis (panel A), X axis (panel B)  | panel A, panel B  |
| Y zoom (panel A only)            | Y axis (panel A)                    | panel A (panel B optional) |


## DRP2 Categories Implied

1. resource writes for dirty axis-derived segment and glyph resources,
2. resource writes for dirty normalized visual data when data changes,
3. render-pass lifecycle for each visible panel,
4. draw commands for marker, path, segment, and glyph contributions,
5. queue submission.


## Pressure On The Spec

This example checks that:

1. sharing a controller handle correctly synchronizes both panels' X axes with no axis-linking API,
2. per-dimension binding allows X to be shared while Y remains independent,
3. the axis pull model produces deterministic results: same controller query → same domain →
   same tick generation decision on both panels,
4. axis derived resources are panel-local: both panels regenerate independently, no cross-panel
   resource sharing or ordering dependency,
5. panzoom within the covered domain produces no `UploadNode` — the covered-domain margin is
   load-bearing for interaction smoothness,
6. a Y zoom on one panel produces no changes in the other panel or its axes,
7. marker and path `VisualSpace` resources are stable under panzoom — normalization is not
   re-triggered by navigation,
8. the `FramePlan` correctly scopes uploads to only dirty axis resources rather than
   rebuilding all axis geometry every frame.
