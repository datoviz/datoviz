# Axis, Guide, Viewport Architecture Refactor Plan

Status: aggressive v0.4-dev pre-RC refactor plan. Breaking API/ABI changes are allowed when they
remove ambiguous old/new behavior and make Datoviz a strict backend target for GSP.

This plan complements `plans/GSP_GUIDE_BACKEND_BREAKING_PLAN.md`. That document defines the GSP
guide/backend contract. This document defines the Datoviz scene architecture needed to satisfy it
without relying on accidental draw order, implicit coordinate conversions, or per-example fixes.


## Problem Statement

The current axis/grid/guide path mixes several generations of scene behavior:

1. Axis ticks and grid lines are generated as retained primitive visuals.
2. DATA visuals may use old axis-domain normalization or the newer View2D data model.
3. Generated visuals choose DATA, VIEW, or fixed controller modes locally.
4. Grid visuals have special-case render routing for plot viewport and plot clipping.
5. Visual stacking relies on raw integer `z_layer` values such as `-1`, `0`, `1000`, and
   `INT32_MAX / 4`.
6. Alpha-pass behavior is partly explicit and partly inferred only by the runtime pipeline.
7. Guide spans expose fill and outline as ordinary visuals, but the semantic layer relationship
   between background, guide fills, grids, data, guide outlines, ticks, labels, and overlays is not
   encoded in one place.

This is not robust enough for GSP. GSP needs mechanical guarantees: one visible-domain model, exact
tick/grid alignment, deterministic layering, explicit unsupported query scopes, and stable
offscreen/interactive parity.


## Non-Negotiable Architecture Targets

1. **One panel coordinate snapshot.**
   Every frame uses one resolved panel snapshot containing:
   - panel rectangle and plot rectangle in logical pixels;
   - plot rectangle in panel VIEW coordinates;
   - ordered DATA domains for X/Y/Z where applicable;
   - View2D data-to-view transform;
   - controller-visible extent after panzoom;
   - aspect-ratio policy and resulting view extents.

2. **One generated-visual role model.**
   Axis spines, ticks, grid lines, labels, guide fills, guide outlines, guide labels, colorbar
   adornments, scale bars, overlays, and backgrounds are generated scene visuals with explicit
   semantic roles. Their layer, coordinate basis, clip rectangle, viewport rectangle, controller
   mode, depth policy, and alpha policy must come from role policy, not scattered call sites.

3. **One semantic layer stack.**
   Raw integer `z_layer` values remain an implementation detail, but generated visuals must use
   named semantic layers:
   - panel background;
   - guide fill / statistical fill;
   - grid;
   - default data;
   - guide outline / interval outline;
   - axis marks and spines;
   - axis text and labels;
   - overlay widgets/readouts.

   The layer stack must produce the same ordering in native Vulkan, offscreen capture, WebGPU
   fixture/live paths, and GSP visual QA.

4. **Alpha is semantic, not accidental.**
   Generated visuals with authored color alpha below 255 must render with source-over blending
   unless their role explicitly opts into another alpha mode. Opaque generated visuals must remain
   in the opaque pass. Changing a style color must update the generated visual's alpha mode before
   frame-plan emission.

5. **Grid and ticks share a render tick snapshot.**
   Grid lines, tick marks, and tick labels must be generated from the same render tick snapshot,
   including exact explicit ticks and reversed-domain inputs. Auto-tick caches must not be reused
   as explicit-tick state.

6. **Guides derive from the same visible-domain snapshot as data.**
   Horizontal/vertical guide lines and spans derive their free axis from the panel's visible DATA
   domain, not stale bounds or viewport guesses. They update when domains, View2D aspect, panzoom,
   panel reserve, or figure size change.

7. **Clip and viewport policy are role policy.**
   Plot-data visuals, axis grids, guide fills, and guide outlines clip to the plot area. Axis ticks,
   spines, tick labels, axis labels, colorbar labels, scale bars, and overlay cards clip to the
   panel or overlay region as their role dictates. Frame-plan emission should ask role policy, not
   a list of visual pointer special cases.

8. **GSP compatibility is a validation gate.**
   Datoviz tests are necessary but not sufficient. The refactor must preserve or improve GSP guide
   cases:
   - `guide/view2d_auto_grid`;
   - `guide/view2d_reversed_explicit`;
   - data/guide overlay cases using transparent spans;
   - visual QA involving aspect-ratio and viewport changes.


## Breaking-Change Policy

Before v0.4 RC1, it is acceptable to break:

1. internal struct layouts;
2. public descriptor struct layouts when the old fields encode ambiguous behavior;
3. enum values;
4. generated `datoviz.raw` ctypes layout;
5. top-level Python facade adaptation details;
6. example screenshots and tests that assert accidental node counts or old layer ordering.

Do not preserve old behavior if it conflicts with the targets above. Do preserve the active runtime
path:

```text
scene frame plans -> drp2 command streams -> vklite runtime ->
canvas/stream frame execution -> optional app presentation
```

Do not implement a Matplotlib or GSP backend inside Datoviz.


## Proposed Internal Model

### `DvzPanelFrameSnapshot`

Add an internal resolved snapshot, produced once per panel per prepare/emit cycle:

```c
typedef struct DvzPanelFrameSnapshot
{
    DvzRect panel_px;
    DvzRect plot_px;
    float plot_view[4];        /* xmin, xmax, ymin, ymax in panel VIEW coordinates */
    float controller_extent[4];/* visible VIEW extent after panzoom/controller */
    double data_x[2];          /* ordered endpoints */
    double data_y[2];
    double data_z[2];
    mat4 data_to_view;
    bool has_view2d;
    bool has_valid_data_x;
    bool has_valid_data_y;
} DvzPanelFrameSnapshot;
```

This is internal. Public API remains centered on `dvz_panel_set_domain()`,
`dvz_panel_set_view2d()`, and `dvz_panel_visible_domain()` unless a better public shape is required
by implementation.

### `DvzGeneratedVisualRole`

Add an internal generated-role enum:

```c
typedef enum DvzGeneratedVisualRole
{
    DVZ_GENERATED_PANEL_BACKGROUND,
    DVZ_GENERATED_GUIDE_FILL,
    DVZ_GENERATED_AXIS_GRID,
    DVZ_GENERATED_DATA_DEFAULT,
    DVZ_GENERATED_GUIDE_OUTLINE,
    DVZ_GENERATED_AXIS_MARKS,
    DVZ_GENERATED_AXIS_TEXT,
    DVZ_GENERATED_OVERLAY,
} DvzGeneratedVisualRole;
```

Each role resolves to:

```c
typedef struct DvzGeneratedVisualPolicy
{
    int32_t z_layer;
    DvzControllerMode controller_mode;
    DvzVisualCoordSpace coord_space;
    DvzFramePlanClipRect clip_rect;
    DvzFramePlanViewportRect viewport_rect;
    bool depth_test;
    DvzAlphaMode default_alpha_mode;
} DvzGeneratedVisualPolicy;
```

Initial layer values should preserve default data at `z_layer = 0`, but no caller should hand-code
magic numbers for generated roles after this refactor.


## Implementation Phases

### Phase 1 - Centralize Generated Visual Policy

1. Add an internal generated-role policy helper.
2. Convert panel background, axis grid, axis marks, axis text, guide fill, guide outline, and
   guide labels to use named policy instead of local magic `z_layer`, controller mode, coordinate
   space, depth, and alpha defaults.
3. Replace frame-plan pointer special cases for generated axis/grid/colorbar/legend visuals with
   role metadata where practical. If a full conversion is too broad for one checkpoint, install the
   role metadata first and leave compatibility shims with comments.
4. Ensure style alpha changes update visual alpha mode before emit.

Validation:

```sh
just test axis
just test scene_graph
just example-c features/guide_spans --png
git diff --check
```

Acceptance:

- Grid remains visible through transparent guide spans.
- Axis/grid tests no longer rely on accidental opaque-only node counts.
- Generated role policy is the only place defining generated-layer magic values.


### Phase 2 - Panel Frame Snapshot

1. Add the internal `DvzPanelFrameSnapshot` resolver.
2. Refactor axis visual/text/tick generation to consume the snapshot.
3. Refactor guide line/span upload to consume the same snapshot.
4. Make aspect-ratio, plot reserve, figure resize, and panzoom invalidation feed the snapshot
   rather than separate helper chains.

Validation:

```sh
just test axis
just test interaction
just test query
just example-c features/guide_lines --png
just example-c features/guide_spans --png
git diff --check
```

Acceptance:

- Data visuals, grids, ticks, guide spans, and public visible-domain readback agree under normal,
  reversed, panzoomed, resized, and equal-aspect panels.


### Phase 3 - Axis Render Tick Snapshot

1. Split axis state into explicit input, auto policy, computed render ticks, and copied render
   labels.
2. Make grid, ticks, tick labels, and explicit labels consume the render tick snapshot only.
3. Remove or rename cache fields whose names imply sorted-domain behavior when they are used for
   render ticks.

Validation:

```sh
just test axis
python3 tools/check_example_manifests.py
git diff --check
```

Acceptance:

- Explicit ticks are exact, ordered, and label-stable.
- Empty explicit ticks render no ticks or grids.
- Auto ticks remain stable and separate.


### Phase 4 - Guide and Fill Composites

1. Move guide fill/outline layering into `src/scene/annotation/guide.c`; examples should not need
   to tune guide span `z_layer` to see grids.
2. Apply the same generated-role policy to bars/bands where their fill/outline semantics match
   guide spans.
3. Ensure transparent fills render in source-over order while outlines and labels remain readable.
4. Keep guide query targets explicitly unsupported until guide picking is intentionally promoted.

Validation:

```sh
just test interaction
just example-c features/guide_spans --png
just example-c features/bars_bands --png
git diff --check
```

Acceptance:

- Examples express semantic style, not render-order workarounds.
- GSP can emit spans/bands without backend-specific layer hacks.


### Phase 5 - Python/ctypes and GSP Proof

1. Regenerate or update bindings if public structs/enums changed.
2. Ensure top-level `import datoviz as dvz` exposes the required GSP guide/domain/tick symbols.
3. Run the local Datoviz Python facade smoke.
4. Run the relevant `../GSP_API` Datoviz v0.4 protocol renderer and visual QA guide tests.

Validation:

```sh
just bindings-check
just ctypes-facade-smoke
cd ../GSP_API && uv run pytest tests/test_datoviz_v04_protocol_renderer.py \
    tests/test_matplotlib_guides.py tests/test_visual_qa_harness.py
git diff --check
```

Acceptance:

- GSP guide cases pass or report explicit, documented unsupported scopes.
- No private Datoviz module imports are required for the GSP path.


## Refactor Stop Conditions

Stop and reassess before RC if any phase requires:

1. a parallel renderer or frame-stream path;
2. a new public plotting grammar in Datoviz;
3. guide picking implementation beyond structured unsupported status;
4. broad nonlinear/categorical/geospatial axis support;
5. large unrelated Python binding generator redesign;
6. churn in generated/runtime binary payloads or the `data` submodule.


## First Implementation Checkpoint

The first code checkpoint should do only Phase 1:

1. add generated visual role policy;
2. apply it to axis grids and guide spans;
3. remove the `features/guide_spans.c` layer workaround;
4. keep tests green;
5. capture `feature_guide_spans` locally for visual inspection;
6. commit only source/test changes, excluding `data` and unrelated binding work.
