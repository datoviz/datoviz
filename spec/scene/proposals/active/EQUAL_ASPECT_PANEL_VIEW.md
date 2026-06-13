> **Execution Status**
> - **Status:** `IMPLEMENTED IN V0.4-DEV`
> - **Updated on:** `2026-06-13`
> - **Purpose:** define the v0.4 API-breaking fix for equal-aspect 2D panels, retained
>   coordinate-space semantics, panzoom evaluation, linked panels, resize behavior, and axes/grid
>   alignment.
> - **Primary pressure tests:** equal-aspect 2D circles in non-square panels, DATA-coordinate
>   axes/grid alignment after resize, linked panzoom panels with different aspect ratios, native and
>   WebGPU frame-plan parity.

# Equal-Aspect Panel View


## Problem

`DvzPanelDomainFit.aspect = DVZ_PANEL_DOMAIN_ASPECT_EQUAL` currently fixes only the axis/data-domain
path. It expands X/Y data domains from the panel plot rectangle so DATA-coordinate visuals and axes
can preserve equal data-unit-per-pixel scale.

Retained visuals attached in visual coordinates do not see that fit. They render through the panel
panzoom MVP, whose initial projection maps `[-1, +1] x [-1, +1]` to the full viewport. On
non-square panels, a visual-space circle therefore stretches even when the panel has an equal-aspect
domain fit.

This must not be solved in examples by manually setting:

```c
dvz_panzoom_zoom(panzoom, (vec2){1.0f, width / height});
```

That workaround hides panel layout in controller state, breaks linked-panel semantics, and does not
give axes, grid, queries, resize, and WebGPU a shared contract.


## Architecture Decision

The panel 2D view owns the resolved controller-visible extent.

The panzoom controller owns semantic navigation state: center/pan, zoom, limits, interaction
baselines, and gesture policy. It does not own one canonical viewport and it does not encode panel
aspect by mutating raw zoom. During frame planning, the panel evaluates the controller against its
current plot rectangle and view-fit policy to produce:

1. the panel apply MVP,
2. the current visible view extent,
3. DATA-to-view normalization,
4. inverse view-to-DATA domains for axes, grids, scale bars, and queries.

This preserves the existing split from `TRANSFORM_PIPELINE.md`:

```text
DataSpace -> ViewSpace -> controller/panel view -> ClipSpace
```

The current `VisualSpace` name is ambiguous in this area. For the implementation, treat the metric
2D space affected by equal aspect as `ViewSpace`, even if public names are finalized separately.


## Public API Break

Replace the domain-fit API with a panel view-fit API, or keep the old names only as temporary
compatibility aliases inside v0.4 development.

Proposed public shape:

```c
typedef enum
{
    DVZ_PANEL_VIEW_FIT_NONE = 0,
    DVZ_PANEL_VIEW_FIT_CONTAIN,
} DvzPanelViewFitMode;

typedef enum
{
    DVZ_PANEL_VIEW_ASPECT_FREE = 0,
    DVZ_PANEL_VIEW_ASPECT_EQUAL,
} DvzPanelViewAspectMode;

typedef struct DvzPanelViewFit
{
    uint32_t struct_size;
    uint32_t flags;
    DvzPanelViewFitMode fit;
    DvzPanelViewAspectMode aspect;
    DvzDataDomain x;
    DvzDataDomain y;
    double padding;
} DvzPanelViewFit;

DvzPanelViewFit dvz_panel_view_fit(void);
int dvz_panel_set_view_fit(DvzPanel* panel, const DvzPanelViewFit* fit);
void dvz_panel_clear_view_fit(DvzPanel* panel);
bool dvz_panel_view_extent(DvzPanel* panel, float out[4]);
```

Keep `dvz_panel_set_domain()` for explicit axis/data domains, but remove fit ownership from axes.
Axes consume resolved panel visible domains; they do not own view fitting.


## Coordinate Spaces

Split attachment coordinates so users can choose whether equal aspect applies:

```c
typedef enum
{
    DVZ_COORD_VIEW = 0,   /* metric panel view coordinates; affected by equal-aspect view fit */
    DVZ_COORD_DATA,       /* data/domain coordinates; mapped through panel DATA -> VIEW */
    DVZ_COORD_PANEL,      /* normalized panel/plot coordinates; intentionally viewport-shaped */
} DvzVisualCoordSpace;
```

Recommended migration:

1. `DVZ_COORD_DATA`: data visuals, axes-bound plots, scale-bar-measured objects.
2. `DVZ_COORD_VIEW`: retained 2D geometry that should preserve metric shape under equal aspect.
3. `DVZ_COORD_PANEL`: backgrounds, borders, guide overlays, and normalized panel chrome that should
   fill or track the panel rectangle.

Current `DVZ_COORD_VISUAL` should be removed, renamed to `DVZ_COORD_VIEW`, or retained only as an
explicit deprecated alias during the v0.4 development branch. It should not continue to mean both
metric view coordinates and panel-normalized coordinates.


## Internal Resolved State

Add one internal resolver in the scene core, not in annotation code:

```c
typedef struct DvzPanelView2DResolved
{
    float view_extent[4];      /* xmin, xmax, ymin, ymax before panzoom */
    double data_x[2];          /* fitted full DATA domain */
    double data_y[2];          /* fitted full DATA domain */
    mat4 data_to_view;         /* DATA -> VIEW model transform */
} DvzPanelView2DResolved;
```

Suggested location:

1. new `src/scene/core/panel_view.c`, or
2. `src/scene/core/panel_geometry.c` if the first slice stays small.

Move the core of `_scene_panel_apply_domain_fit()` out of `src/scene/annotation/axis.c`. Axis code
may keep tick generation and axis visuals, but it should not calculate panel view-fit state.


## Equal-Aspect Resolution

For a panel plot rectangle with pixel aspect:

```text
plot_aspect = plot_width / plot_height
```

and a source data domain with:

```text
data_aspect = x_span / y_span
```

the resolver computes both:

1. fitted DATA domains for DATA-coordinate visuals and axes;
2. a base VIEW extent for controller-applied VIEW-coordinate visuals.

For metric view coordinates, identity panzoom should preserve screen scale:

```text
square plot:      view_extent = [-1, +1] x [-1, +1]
wide plot:        view_extent = [-plot_aspect, +plot_aspect] x [-1, +1]
tall plot:        view_extent = [-1, +1] x [-1 / plot_aspect, +1 / plot_aspect]
```

The exact extent may also be centered on a fitted domain center when future view-origin policies are
added. The important invariant is:

```text
view_units_per_pixel_x == view_units_per_pixel_y
```

when aspect is equal.


## Panzoom Evaluation

Do not make `DVZ_PANZOOM_FLAGS_KEEP_ASPECT` mutate initial projection or raw zoom. Split panzoom
behavior into:

1. panel view aspect policy, resolved by the panel from viewport size;
2. optional gesture constraint, where `KEEP_ASPECT` couples drag/wheel zoom deltas.

Add a panel-aware panzoom evaluation helper:

```c
typedef struct DvzPanzoomEval
{
    float base_extent[4];
    float viewport_width;
    float viewport_height;
} DvzPanzoomEval;

typedef struct DvzPanzoomResolved
{
    DvzMVP mvp;
    float visible_extent[4];
} DvzPanzoomResolved;

bool dvz_panzoom_resolve(
    const DvzPanzoom* panzoom, const DvzPanzoomEval* eval, DvzPanzoomResolved* out);
```

The current `dvz_panzoom_mvp()` may remain as a standalone default that evaluates against
`[-1, +1] x [-1, +1]`, but scene frame emission must use the panel-aware resolver.


## Linked Panels

The accepted controller model says a shared controller must not own one canonical native viewport.
This proposal preserves that rule.

Linked-panel behavior:

1. sharing a controller shares semantic panzoom state;
2. each panel evaluates that state against its own plot rectangle and view-fit policy;
3. partial links copy semantic state components, not viewport-derived projection hacks;
4. `EXTENT_X` and `EXTENT_Y` should copy resolved visible extents or visible DATA domains, not just
   raw `pan` and `zoom` fields.

Important limitation:

```text
identical X extent + identical Y extent + equal screen scale
```

cannot all hold across panels with different plot aspect ratios. When panel aspects differ, equal
screen scale means at least one domain extent must differ. The implementation should document and
test the chosen priority instead of silently breaking equal aspect.

Recommended priority:

1. explicit `EXTENT_X` link keeps X visible DATA domain identical;
2. explicit `EXTENT_Y` link keeps Y visible DATA domain identical;
3. equal-aspect panels may expand the unlinked dimension per panel;
4. full XY extent identity is valid only when panel plot aspects match or aspect is free.


## Axes, Grid, Scale Bars, And Queries

Axes and grid should ask the panel view resolver for visible DATA domains. They should not infer
controller state directly and should not own fit policy.

Required behavior:

1. DATA-coordinate visuals, axes, grid lines, scale bars, and domain queries use the same
   `data_to_view` and inverse visible-domain calculations.
2. Resizing a figure or changing panel reserves invalidates the panel view resolver and axes layout.
3. Panzoom changes invalidate panel transform and visible-domain consumers without reuploading
   normalized data unless the visual family explicitly requires it.
4. Fixed overlays and `DVZ_COORD_PANEL` visuals do not participate in view-domain queries.


## Backend And WebGPU Parity

This is a scene/frame-plan change. Native Vulkan and WebGPU should receive the same corrected MVPs
and per-visual transforms.

Do not add backend-specific aspect correction. The runtime path should continue to upload the
scene-common MVP and viewport uniforms from frame-plan render nodes.

Parity requirements:

1. DRP2 frame plans contain the corrected apply MVP for VIEW-coordinate visuals.
2. DATA-coordinate visuals still receive per-visual DATA-to-VIEW model matrices when needed.
3. WebGPU consumes the same frame artifact packets and should not reimplement panel aspect logic in
   JavaScript or WGSL host glue.


## Implementation Plan

1. Add the internal panel view resolver and move domain-fit math out of axis code.
2. Add `DvzPanelViewFit` public types and functions, or rename the existing domain-fit API if the
   branch accepts direct API breakage.
3. Update `DvzPanel` storage from `domain_fit_enabled/domain_fit` to
   `view_fit_enabled/view_fit` plus cached invalidation state if useful.
4. Update figure resize, panel reserve, panel padding, grid layout, and axis reserve refresh paths
   to invalidate and re-resolve panel view state.
5. Update `_scene_panel_apply_mvp()` to evaluate panzoom through the resolved panel base extent.
6. Update `_scene_panel_data_model()` and `dvz_panel_data_to_visual_positions()` to use the
   resolver's DATA-to-VIEW transform.
7. Split `DVZ_COORD_VISUAL` into `DVZ_COORD_VIEW` and `DVZ_COORD_PANEL`, then migrate existing
   attachments:
   - data plots: `DVZ_COORD_DATA`,
   - metric retained visuals: `DVZ_COORD_VIEW`,
   - backgrounds/borders/panel chrome: `DVZ_COORD_PANEL` plus `DVZ_CONTROLLER_FIXED` where needed.
8. Update panzoom extent and controller-link logic so visible-domain links operate on resolved
   panel extents, not raw viewport-blind panzoom fields.
9. Update docs and examples to stop recommending manual zoom aspect hacks.
10. Run focused tests, then broad scene validation.


## Tests

Add focused C tests before broad examples:

1. equal-aspect `DVZ_COORD_VIEW` circle or square emits an MVP whose X/Y screen scale is equal on a
   wide panel;
2. the same test on a tall panel;
3. DATA-coordinate equal-aspect domain fit still maps X/Y data units to equal screen pixels after
   figure resize;
4. `DVZ_COORD_PANEL` geometry intentionally fills a non-square panel and is allowed to stretch;
5. axes/grid tick positions match DATA-coordinate visual positions after panzoom and resize;
6. shared panzoom on two different-aspect panels shares semantic state but resolves different
   panel MVPs;
7. `EXTENT_X` and `EXTENT_Y` controller links preserve the linked DATA domain while equal aspect
   expands the unlinked dimension;
8. WebGPU/DRP2 fixture or stream-shape test proves the corrected MVP appears in frame artifacts.

Suggested validation loop:

```sh
just test scene
just spec-check
git diff --check
```

Use narrower test filters while iterating.


## Commit Hygiene For The Implementing Agent

Before committing code changes:

1. inspect `git status --short`;
2. do not stage `data` gitlink changes unless explicitly approved in the current turn;
3. do not stage generated/runtime binaries such as `libs/vulkan/`, `*.dylib`, `*.so`, `*.dll`,
   `*.npy`, `*.npz`, or `.DS_Store`;
4. do not stage unrelated example or user changes;
5. run `git diff --check`;
6. run `git diff --cached --stat` and verify the staged set contains only the intended files.

Prefer a logical checkpoint commit for this architecture slice after focused tests pass.
