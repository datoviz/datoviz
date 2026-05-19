# Scene 2D Axes Implementation Plan

> **Execution Status**
> - **Status:** `IMPLEMENTATION PLAN`
> - **Updated on:** `2026-05-16`
> - **Purpose:** stage an end-to-end v0.4 implementation of 2D axes through the active
>   scene -> FramePlan -> DRP2 path, while leaving text rendering as a separate workstream.


## Objective

Implement full 2D axes as panel-owned semantic scene objects.

The first complete axis slice should support:

1. panel-owned X/Y data domains,
2. panzoom-linked visible-domain queries,
3. deterministic linear tick generation,
4. major/minor tick and spine geometry as derived scene contributions,
5. optional grid-line geometry,
6. retained axis state and dirty/cached tick layouts,
7. offscreen and GLFW validation through the existing scene/app path,
8. a narrow text-provider contract for tick labels and axis labels, without implementing text
   rendering in this plan.

Axes must not call Vulkan, vklite, canvas, or DRP2 runtime APIs directly.
They should derive scene-side resources that the existing frame-plan emitter can lower into DRP2.


## Explicit Non-Goals

This plan does not implement:

1. a rendered text/glyph visual,
2. font loading, shaping, glyph atlases, text layout, or DPI-aware text rasterization,
3. full 3D axes,
4. polar/geographic/graticule axes,
5. categorical/date/time axes,
6. public Python bindings,
7. vector export semantics,
8. per-axis picking.

Text is still part of the axis semantic output: axes should produce label strings, style hints, and
placement requests. The text implementation will later consume those requests.


## Existing v0.3 Assets Worth Reusing

The v0.3 axis stack is useful as source material, but it should not be copied as architecture.

Reusable with adaptation:

1. [v0.3/src/scene/ticks.c](/home/cyrille/GIT/Viz/datoviz/v0.3/src/scene/ticks.c)
   - `nice_number()` using the 1/2/5 step ladder,
   - decimal precision selection,
   - factored offset/exponent detection,
   - `dvz_ticks_compute()` shape,
   - `dvz_ticks_linspace()` value generation.
2. [v0.3/include/datoviz/scene/ticks.h](/home/cyrille/GIT/Viz/datoviz/v0.3/include/datoviz/scene/ticks.h)
   - `DvzTicksSpec { format, precision, exponent, offset }` as a label-format metadata model.
3. [v0.3/src/scene/box.c](/home/cyrille/GIT/Viz/datoviz/v0.3/src/scene/box.c)
   and [v0.3/src/scene/ref.c](/home/cyrille/GIT/Viz/datoviz/v0.3/src/scene/ref.c)
   - linear data-domain -> visual-space normalization,
   - visual-space -> data-domain inverse mapping,
   - tests around box/ref normalization.
4. [v0.3/src/scene/panzoom.c](/home/cyrille/GIT/Viz/datoviz/v0.3/src/scene/panzoom.c)
   - visible VisualSpace extent formula:

   ```text
   xmin = -pan_x - 1 / zoom_x
   xmax = -pan_x + 1 / zoom_x
   ymin = -pan_y - 1 / zoom_y
   ymax = -pan_y + 1 / zoom_y
   ```

5. [v0.3/src/scene/axis.c](/home/cyrille/GIT/Viz/datoviz/v0.3/src/scene/axis.c)
   - update flow:

   ```text
   panzoom extent -> inverse through domain -> tick compute -> normalize tick anchors
   -> build tick geometry
   ```

6. v0.3 axis tests as behavior seeds:
   - [v0.3/tests/scene/test_ticks.c](/home/cyrille/GIT/Viz/datoviz/v0.3/tests/scene/test_ticks.c),
   - [v0.3/tests/scene/test_panzoom.c](/home/cyrille/GIT/Viz/datoviz/v0.3/tests/scene/test_panzoom.c),
   - [v0.3/tests/scene/test_ref.c](/home/cyrille/GIT/Viz/datoviz/v0.3/tests/scene/test_ref.c).

Do not reuse directly:

1. `DvzRef` as a public or internal v0.4 object name,
2. `DvzAxis` as a bundle of old `DvzBatch`, `glyph`, `segment`, `factor`, and `spine` visuals,
3. v0.3 view flags, clipping flags, or atlas/font ownership,
4. v0.3 `dvz_panzoom_bounds(pz, ref, ...)` coupling, because v0.4 should keep panzoom ignorant of
   data-domain semantics,
5. v0.3 log-tick flag, because log ticks were declared but not implemented.


## Target Architecture

Keep the responsibilities split like this:

```text
DvzPanzoom
  owns pan/zoom controller state
  answers visible VisualSpace extent only

DvzPanelDomain
  owns semantic data-domain ranges and scale policy
  maps DataSpace <-> VisualSpace
  converts panzoom VisualSpace extent into visible DataSpace extent

DvzAxis
  panel-owned semantic object
  owns tick policy, layout cache, style, and derived geometry state
  computes tick values in DataSpace
  maps tick anchors into VisualSpace
  emits derived geometry/text requests

FramePlan / emitter
  consumes derived axis contributions
  emits ordinary DRP2 resources and draw commands
```

The important rule is that panzoom does not know about scientific coordinates, and DRP2 does not
know about axis semantics.


## Proposed Internal/Public Types

The exact public names can move, but the implementation should converge on these concepts.

```c
typedef enum
{
    DVZ_DIM_X = 0,
    DVZ_DIM_Y = 1,
    DVZ_DIM_Z = 2,
} DvzDim;

typedef enum
{
    DVZ_DOMAIN_LINEAR = 0,
    DVZ_DOMAIN_LOG10,
} DvzDomainScale;

typedef struct DvzDataDomain
{
    double min;
    double max;
    DvzDomainScale scale;
    const char* unit;
    DvzFormatDesc format;
    uint32_t flags;
} DvzDataDomain;

typedef struct DvzAxisTickPolicy
{
    uint32_t target_count;
    float min_pixel_spacing;
    uint32_t minor_per_interval;
    double coverage_margin_steps;
    uint32_t flags;
} DvzAxisTickPolicy;

typedef struct DvzAxisStyle
{
    float spine_width;
    float major_tick_width;
    float minor_tick_width;
    float grid_width;
    float major_tick_length;
    float minor_tick_length;
    uint8_t spine_color[4];
    uint8_t major_tick_color[4];
    uint8_t minor_tick_color[4];
    uint8_t grid_color[4];
    bool show_spine;
    bool show_major_ticks;
    bool show_minor_ticks;
    bool show_grid;
} DvzAxisStyle;
```

Likely API shape:

```c
DVZ_EXPORT bool dvz_panel_set_domain(DvzPanel* panel, DvzDim dim, const DvzDataDomain* domain);
DVZ_EXPORT bool dvz_panel_domain(const DvzPanel* panel, DvzDim dim, DvzDataDomain* out);
DVZ_EXPORT bool dvz_panel_visible_domain(const DvzPanel* panel, DvzDim dim, double* min, double* max);

DVZ_EXPORT DvzAxis* dvz_panel_axis(DvzPanel* panel, DvzDim dim);
DVZ_EXPORT bool dvz_axis_set_visible(DvzAxis* axis, bool visible);
DVZ_EXPORT bool dvz_axis_set_tick_policy(DvzAxis* axis, const DvzAxisTickPolicy* policy);
DVZ_EXPORT bool dvz_axis_set_style(DvzAxis* axis, const DvzAxisStyle* style);
DVZ_EXPORT bool dvz_axis_set_label(DvzAxis* axis, const char* label);
```

Text-facing API shape, for the later text implementation:

```c
typedef struct DvzTextRunRequest
{
    const char* text;
    DvzTextPlacement placement;
    DvzTextStyle style;
    uint64_t owner_id;       /* semantic axis id */
    uint32_t role;           /* tick label, axis label, factor label */
} DvzTextRunRequest;
```

Axes should build `DvzTextRunRequest` records and keep them in the axis layout cache.
The text implementation can later lower them to glyph resources and draw calls.


## Stage 1 - Domain Foundation

Goal: add v0.4-native panel domain state without axes.

Implementation:

1. Add `DvzDim`, `DvzDataDomain`, and domain scale enums if no equivalent exists.
2. Add `DvzPanel` storage:

   ```text
   domains[3]
   domain_set[3]
   domain_version[3]
   ```

3. Implement internal helpers:

   ```text
   _scene_domain_validate()
   _scene_domain_normalize_1d()
   _scene_domain_inverse_1d()
   _scene_panel_normalize_position()
   _scene_panel_inverse_position()
   ```

4. Implement linear domains first.
5. Support inverted domains from the start: `min > max` is valid and maps monotonically in the
   declared direction.
6. Reserve log domains but reject invalid log inputs with diagnostics until Stage 9.
7. Do not rename this to `DvzRef`; the old `DvzRef` behavior should be absorbed into clearer
   panel-domain helpers.

Tests:

1. domain normalize/inverse roundtrip,
2. inverted X and Y domains,
3. unset domain falls back to pass-through VisualSpace behavior,
4. invalid log domain rejected when used,
5. domain version bumps only on semantic changes.

Validation:

```text
just build
just test scene
git diff --check
```


## Stage 2 - Panzoom Visible Extent Queries

Goal: expose robust controller extents in VisualSpace.

Implementation:

1. Add a v0.4 helper based on the v0.3 formula:

   ```c
   bool dvz_panzoom_extent(const DvzPanzoom* pz, DvzBox* out);
   ```

   or keep it internal if public exposure is premature:

   ```c
   bool _scene_panzoom_extent(const DvzPanzoom* pz, DvzBox* out);
   ```

2. Add `dvz_panzoom_set_extent()` if needed for tests and future `xlim` / `ylim` helpers.
3. Keep the result in VisualSpace. Do not pass a domain/ref object into panzoom.
4. Add panel-level visible-domain conversion:

   ```text
   _scene_panel_visible_domain(panel, dim, out_min, out_max)
   ```

5. Validate finite positive zoom and finite pan.
6. Decide how fixed-axis panzoom flags affect extents; likely they still report the visible extent
   based on current state.

Tests:

1. identity panzoom extent is `[-1, +1]` on X/Y,
2. pan/zoom state matches v0.3 test cases,
3. set-extent roundtrip if the setter is added,
4. panel visible domain converts VisualSpace extent through a linear domain,
5. inverted domain returns the expected data ordering.


## Stage 3 - Tick Engine Port

Goal: port v0.3 tick generation into a v0.4 internal helper.

Implementation:

1. Add internal files, likely:

   ```text
   src/scene/axis_ticks.c
   src/scene/_axis.h
   ```

2. Start with linear numeric ticks only.
3. Port:
   - nice-step calculation,
   - tick count from `lmin/lmax/lstep`,
   - format metadata,
   - label-string generation into fixed-size caller-owned buffers or scene-owned arrays.
4. Replace direct `calloc`, `free`, `memset`, `memcpy`, `snprintf` usage with project wrappers.
5. Harden edge cases:
   - NaN/inf domains,
   - zero span,
   - overflow in tick count,
   - too many ticks,
   - extremely small and large ranges,
   - negative-only ranges,
   - ranges crossing zero.
6. Add a deterministic maximum tick count such as `DVZ_SCENE_MAX_AXIS_TICKS`.

Important improvements over v0.3:

1. no per-label heap allocation in the core tick helper,
2. no implicit global `MAX_LABEL_LEN` allocation contract,
3. no direct printing API in core logic,
4. no always-false `dvz_ticks_dirty()` placeholder,
5. no misleading log flag until implemented.

Tests:

1. port representative v0.3 tick cases,
2. assert exact `lmin/lmax/lstep` for stable cases,
3. assert format metadata for large-offset and small-range cases,
4. assert no more than max ticks are emitted,
5. assert invalid domains fail without partially initialized output.


## Stage 4 - Axis State And Cache

Goal: add retained panel-owned axes with no rendering yet.

Implementation:

1. Add `DvzAxis` internal/public opaque type.
2. Add `DvzPanel` storage:

   ```text
   DvzAxis axes[3];
   bool axis_enabled[3];
   uint64_t axis_version[3];
   ```

   The first implementation can initialize X and Y only.

3. Add axis state:

   ```text
   panel pointer
   dimension
   visibility
   tick policy
   style
   label string
   covered_domain_min/max
   last_visible_domain_min/max
   last_panel_pixel_size
   tick_count
   tick values in DataSpace
   tick anchors in VisualSpace
   minor tick anchors
   text run requests
   dirty flags
   ```

4. Add default axes on panel creation, but allow disabling before emission.
5. Keep axes owned by the panel; do not allocate free-standing axes in the scene.
6. Add an internal axis update step that can run before frame-plan assembly.

Tests:

1. panels create X/Y axis state by default if that policy is selected,
2. axis handles are stable,
3. enabling/disabling axis changes only axis state,
4. axis policy/style updates mark axis layout dirty,
5. destroying a panel invalidates axis ownership cleanly.


## Stage 5 - Axis Layout Regeneration Policy

Goal: implement the cached semantic tick layout.

Implementation:

1. Compute visible data domain:

   ```text
   visible VisualSpace extent from panzoom
   -> panel-domain inverse mapping
   -> visible DataSpace min/max for axis dimension
   ```

2. Compute target tick count from panel pixel size:

   ```text
   usable_axis_pixels / min_pixel_spacing
   ```

   Keep the v0.3 `TICK_DENSITY * range_size / glyph_size` idea as a reference, but make it explicit
   and independent from the text implementation.

3. Generate ticks over a covered domain larger than visible:

   ```text
   covered_min = nice_lmin - margin_steps * lstep
   covered_max = nice_lmax + margin_steps * lstep
   ```

4. Reuse the current layout when:
   - visible domain is comfortably inside the covered domain,
   - tick spacing remains within density tolerance,
   - panel size has not materially changed,
   - tick policy and format have not changed,
   - domain version has not changed.

5. Mark only axis-derived resources dirty when the layout regenerates.
6. Keep the first heuristic simple and deterministic; tune later if live interaction shows churn.

Tests:

1. initial axis update regenerates,
2. small pan inside covered domain does not regenerate,
3. pan outside covered domain regenerates,
4. zoom that changes density enough regenerates,
5. unrelated visual data changes do not regenerate axes,
6. resize across a threshold regenerates.


## Stage 6 - Geometry Contribution Builder

Goal: build visible axis geometry without text rendering.

Implementation:

1. Generate CPU-side derived geometry for:
   - spine,
   - major tick marks,
   - minor tick marks,
   - optional grid lines.
2. Decide first geometry representation:
   - use existing `primitive` line-list support if it already emits line-list reliably,
   - or add a narrow retained `segment` visual family if line width and pixel-shifted endpoints
     require it.
3. Prefer a simple first slice:
   - axis/tick/grid endpoints in VisualSpace,
   - fixed RGBA colors,
   - one-pixel or implementation-supported line width,
   - no text.
4. Build major/minor tick positions using the v0.3 strategy:

   ```text
   major ticks: generated tick anchors
   minor ticks: evenly spaced between adjacent major ticks
   ```

5. For X axis:
   - varying X anchor,
   - fixed Y axis position, likely panel bottom in VisualSpace or panel overlay space.
6. For Y axis:
   - varying Y anchor,
   - fixed X axis position, likely panel left.
7. Keep grid lines optional and generated from major tick anchors.

Open design point:

1. v0.3 used fixed visual axes and pixel shifts for tick lengths.
2. v0.4 should avoid old fixed-axis shader flags.
3. The clean v0.4 approach is to generate tick endpoints in panel/VisualSpace using panel pixel
   size to convert tick length from pixels to VisualSpace units.

Tests:

1. X axis spine/tick endpoint generation,
2. Y axis spine/tick endpoint generation,
3. minor tick count,
4. grid line count and endpoints,
5. inverted domain still produces monotonic visual positions,
6. generated geometry remains finite.


## Stage 7 - FramePlan And DRP2 Emission

Goal: render axis geometry through the normal scene path.

Implementation:

1. Extend scene emission so panel axis update runs before render-node construction.
2. Add axis-derived geometry resources to the `FramePlan`.
3. Lower axis geometry as ordinary render visuals or internal render contributions.
4. Ensure axis resources have stable semantic labels for trace/debug output:

   ```text
   fig0_p0.axis.x.spine
   fig0_p0.axis.x.ticks.major
   fig0_p0.axis.x.ticks.minor
   fig0_p0.axis.x.grid
   fig0_p0.axis.y.spine
   ```

5. Draw ordering:
   - grid behind data,
   - data visuals in their normal z order,
   - spine/ticks above data by default.
6. Keep this ordering explicit rather than relying on incidental insertion order.
7. Do not emit text draw commands; emit only text requests into diagnostics or a retained text
   request array for later use.

Tests:

1. emitted stream includes line geometry resources for axes,
2. no text/glyph commands are required,
3. axis labels appear only as text requests/metadata,
4. z-order is deterministic,
5. axes work with multi-panel viewport/scissor,
6. axes do not affect panels where they are disabled.

Validation:

```text
just build
just test test_scene_axis
just test scene
DVZ_DRP2_TRACE=full DVZ_DRP2_TRACE_COLOR=0 ./build/examples/c/hello_axes_glfw 2
git diff --check
```


## Stage 8 - Public API And Example

Goal: make axes usable from C without exposing implementation details.

Public API shape:

```c
DvzPanel* panel = dvz_panel(figure, (DvzPanelDesc){0, 0, 1, 1});

dvz_panel_set_domain(
    panel, DVZ_DIM_X,
    &(DvzDataDomain){.min = 0.0, .max = 10.0, .scale = DVZ_DOMAIN_LINEAR});
dvz_panel_set_domain(
    panel, DVZ_DIM_Y,
    &(DvzDataDomain){.min = -1.0, .max = +1.0, .scale = DVZ_DOMAIN_LINEAR});

DvzAxis* x_axis = dvz_panel_axis(panel, DVZ_DIM_X);
DvzAxis* y_axis = dvz_panel_axis(panel, DVZ_DIM_Y);

dvz_axis_set_label(x_axis, "time (s)");
dvz_axis_set_label(y_axis, "signal");
```

Example:

1. add `examples/c/hello_axes_glfw.c`,
2. create a point/path scene with explicit domains,
3. attach panzoom,
4. show axes, ticks, and optional grid,
5. update a status print or trace with visible domain values.

No rendered tick labels are expected until the text workstream lands.

Tests:

1. API rejects NULL panel/axis/domain safely,
2. API rejects invalid dimension,
3. changing domain updates axis layout on next frame,
4. example builds with `just build`.


## Stage 9 - Log Domain And Inverted-Axis Hardening

Goal: broaden numeric axis semantics after linear axes are stable.

Implementation:

1. Implement log10 domain normalization:

   ```text
   normalized = linear_map(log10(value), log10(min), log10(max))
   ```

2. Reject values <= 0 for log domains before normalization/upload.
3. Add log tick generation:
   - major ticks at decades,
   - optional minor ticks at 2..9 within each decade when density allows.
4. Keep inverted log domains valid when both endpoints are positive.
5. Add diagnostics for invalid domain/data combinations.

Tests:

1. log normalize/inverse roundtrip,
2. invalid log domain rejected,
3. log ticks over one decade,
4. log ticks over multiple decades,
5. inverted log domain.


## Stage 10 - Linked Panels And Domain Sharing

Goal: support synchronized axes across panels without axis-specific linking.

Implementation:

1. Keep the primary linking mechanism at the controller/domain level.
2. Add panel-domain sharing only if current link-channel state is insufficient.
3. Ensure each panel still owns its own axis layout cache and derived geometry.
4. Shared X panzoom should cause both panels to query the same visible X range, but each panel should
   upload its own axis geometry only when needed.

Tests:

1. two panels with shared X panzoom produce matching visible X domains,
2. independent Y panzoom changes only one panel's Y axis,
3. panel resize affects only that panel's axis layout,
4. multi-panel offscreen render remains nonblank and scissored.


## Stage 11 - Text Integration Contract

Goal: prepare for text rendering without implementing it here.

Axis output for text should include:

1. tick label strings,
2. axis label string,
3. factored offset/exponent label if used,
4. role metadata,
5. semantic owner id,
6. anchor position in panel/VisualSpace or screen/pixel space,
7. alignment hint,
8. preferred style.

The text integration workstream should finalize:

1. glyph atlas implementation,
2. label collision/measurement,
3. rotated Y-axis label support,
4. DPI behavior,
5. multiline and rich text,
6. final release-quality lowering to DRP2.

Axes should not block on perfect text polish. Basic text rendering exists, so tests should verify
generated text requests, rendered geometry, and label integration separately while the final text
hardening work continues.


## Stage 12 - Cleanup And Consolidation

Goal: remove temporary scaffolding and make axes a normal part of scene planning.

Checklist:

1. move stable internal helpers to well-named scene files,
2. add public header docs for domain and axis APIs,
3. update `spec/scene/semantics/AXES.md` if implementation clarifies behavior,
4. update `docs/architecture/manual_scene_smoke.md`,
5. add a focused troubleshooting section for axis trace labels,
6. decide whether axes are default-on or opt-in for v0.4 examples,
7. update `agents/now/NEXT_STEPS.md` only after the implementation lands.


## Suggested File Plan

Likely new files:

```text
include/datoviz/scene/axis.h
src/scene/_axis.h
src/scene/axis.c
src/scene/axis_ticks.c
src/scene/domain.c
src/scene/tests/axis.c
examples/c/hello_axes_glfw.c
```

Likely touched files:

```text
include/datoviz/scene.h
include/datoviz/scene/types.h
include/datoviz/scene/enums.h
include/datoviz/scene/panzoom.h
src/scene/_scene.h
src/scene/panzoom.c
src/scene/scene.c
src/scene/scene_emit.c
src/scene/frame_plan.c
src/scene/frame_plan_emit.c
src/scene/visual_pipeline.c
src/scene/tests/test_scene.h
src/scene/tests/test_scene.c
```

If a true `segment` visual is needed for pixel-width ticks, add it as a separate visual-family
slice before binding axes to it.


## Validation Matrix

Narrow validation per stage:

```text
just build
just test test_scene_axis_domain
just test test_scene_axis_ticks
just test test_scene_axis_layout
```

Broad validation after FramePlan/DRP2 emission:

```text
just test scene
just test drp2
just test app
just spec-check
git diff --check
```

Graphics smoke after rendered geometry lands:

```text
./build/examples/c/hello_axes_glfw 60
DVZ_DRP2_TRACE=full DVZ_DRP2_TRACE_COLOR=0 ./build/examples/c/hello_axes_glfw 2
```

If axis rendering touches line pipelines, render-pass ordering, resize, or panel scissor behavior,
run at least one bounded GLFW smoke with Vulkan validation enabled.


## Recommended First Implementation Slice

The smallest useful first patch is:

1. internal `DvzDataDomain` state on `DvzPanel`,
2. domain normalize/inverse helpers,
3. v0.4 `dvz_panzoom_extent()` query,
4. ported linear tick engine with tests,
5. no rendering.

That slice creates the foundation for axes without entangling it with text, layout, or DRP2 emission.

The second patch should add retained `DvzAxis` state and layout-cache tests.

The third patch should render spine/tick/grid geometry through existing primitive/path infrastructure
or a deliberately small segment visual slice.
