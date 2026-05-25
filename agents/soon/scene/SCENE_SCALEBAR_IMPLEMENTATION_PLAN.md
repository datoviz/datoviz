# Scene Scale Bar Implementation Plan

> **Execution Status**
> - **Status:** `SOON IMPLEMENTATION PLAN`
> - **Updated on:** `2026-05-25`
> - **Purpose:** implement adaptive physical-unit scale bars as retained scene measurement
>   annotations, with a first 2D screen-space slice and a follow-up explicit-reference 3D slice.


## Context

Adaptive scale bars belong to the scene annotation and measurement layer, not to ad hoc example
geometry. The relevant durable design notes are:

1. [../../../spec/scene/proposals/active/ANNOTATION_MEASUREMENT_DESIGN.md](../../../spec/scene/proposals/active/ANNOTATION_MEASUREMENT_DESIGN.md)
2. [../../../spec/scene/proposals/active/ANNOTATION_TEXT_SCALE_API.md](../../../spec/scene/proposals/active/ANNOTATION_TEXT_SCALE_API.md)
3. [../../../spec/scene/proposals/active/AXES_DOMAIN_DESIGN.md](../../../spec/scene/proposals/active/AXES_DOMAIN_DESIGN.md)
4. [../../../spec/scene/semantics/ANNOTATIONS.md](../../../spec/scene/semantics/ANNOTATIONS.md)
5. [../../../spec/scene/semantics/TEXT.md](../../../spec/scene/semantics/TEXT.md)

The current code already has `DVZ_ANNOTATION_SCALEBAR` in the enum set, retained annotations,
rendered text/glyphs, panel domains, panzoom visible-domain helpers, and segment/path visuals. The
missing work is the typed scale-bar public surface, semantic state, shared nice-length/unit
formatting, derived segment/text realization, and resize/panzoom invalidation.


## Target Behavior

The first release-quality slice should support a panel-attached horizontal scale bar:

1. screen-space anchored to a panel edge or corner,
2. label above or below the bar,
3. automatic semantic length selection from a `1 / 2 / 5 * 10^n` nice-value ladder,
4. physical-unit formatting such as `5 mm`, `20 km`, or `100 um`,
5. updates after panzoom, visible-domain, panel resize, DPI, and format changes,
6. deterministic derived visuals in the normal scene -> DRP2 -> runtime path,
7. clean hide/destroy behavior with no stale derived visuals.

The first slice should be 2D-only by default. A 3D scale bar should require an explicit reference
point or depth because perspective projection does not have one global physical scale.


## Proposed Public Shape

The exact names may still change during API review, but the first typed constructor should look
close to this:

```c
DvzAnnotation* dvz_annotation_scalebar(DvzPanel* panel, const DvzScaleBarDesc* desc);
```

Initial descriptor fields:

```c
struct DvzScaleBarDesc
{
    DvzDim dimension;
    DvzSceneAnchor anchor;
    DvzScaleBarLabelPosition label_position;
    DvzTextStyle label_style;
    DvzTextPlacement placement;
    DvzFormatDesc format;
    const char* unit;
    double data_to_unit;
    float target_length_px;
    float min_length_px;
    float max_length_px;
    float offset_px[2];
    float tick_length_px;
    float line_width_px;
    uint8_t line_color[4];
    uint8_t background_color[4];
    uint32_t flags;
};
```

Keep this typed. A generic `dvz_annotation(..., kind, ...)` surface is too weak for scale bars
because the object owns semantic unit and length-selection policy.


## Subagent Work Split

Use subagents for bounded, disjoint write scopes. Keep final rendering integration local to the
main agent because it crosses annotation storage, text realization, resource keys, clipping, and
frame-plan emission.

### A. Nice Length And Unit Formatting

Ownership:

1. `src/scene/_scale_ticks.h`
2. `src/scene/scale_ticks.c`
3. focused scene tests for formatting and nice-length selection

Tasks:

1. extract a shared internal `1 / 2 / 5 * 10^n` nice-number helper,
2. add a helper that chooses a scale-bar semantic length from target/min/max pixel constraints,
3. add ASCII SI-prefix formatting for physical units, including `n`, `u`, `m`, `c`, `k`, and `M`,
4. preserve current axis and colorbar behavior unless a focused migration is explicitly included.

Notes:

1. Axis and colorbar currently duplicate nice-number logic.
2. `DvzFormatDesc` already covers precision, prefix, suffix, and unit display, but not automatic SI
   prefix selection.
3. Prefer ASCII `um` for micrometers until the project chooses a Unicode micro policy.


### B. Public API And Retained State

Ownership:

1. `include/datoviz/scene/types.h`
2. `include/datoviz/scene/annotation.h`
3. `src/scene/_scene.h`
4. annotation bookkeeping tests

Tasks:

1. add `DvzScaleBarDesc` and the label-position enum,
2. add `dvz_annotation_scalebar()`,
3. store scale-bar semantic state inside `DvzAnnotation`,
4. add derived-visual pointers for the bar segment and label text/glyph realization,
5. ensure create, format-set, visible, and destroy paths dirty the right state and request frames.

Notes:

1. Current retained annotations store only one generated visual; scale bars need at least a segment
   visual and a label text/glyph visual.
2. Follow colorbar and legend generated-adornment patterns more than the current label-only
   annotation lowering path.


### C. Visible Scale Computation

Ownership:

1. private scene helpers near panel/domain or scale-bar code,
2. focused tests for 2D visible domain and panzoom changes,
3. optional 3D helper tests if the explicit-reference slice starts.

Tasks:

1. compute 2D data units per pixel from `dvz_panel_visible_domain()` and panel pixel size,
2. support X and Y dimensions,
3. include panel resize and panzoom/domain changes in the cache key or invalidation path,
4. defer automatic 3D until the explicit reference-point/depth API is agreed.

Recommended 2D formula:

```text
units_per_px_x = (visible_xmax - visible_xmin) / panel_width_px
units_per_px_y = (visible_ymax - visible_ymin) / panel_height_px
```

For data domains, compute the visible data domain first, then apply `data_to_unit`.


### D. Rendering Integration

Ownership:

1. `src/scene/text_annotation.c`
2. `src/scene/panel_render_emit.c`
3. `src/scene/scene_resource_key.c`
4. scene visual/frame-plan tests

Tasks:

1. lower each scale bar to a fixed screen-space segment visual plus one label text object,
2. make the bar respect panel clipping and z ordering with other annotation overlays,
3. place the line from anchor, offset, selected pixel length, and label gap,
4. update the label string when semantic length or selected SI unit changes,
5. hide all derived visuals when disabled, destroyed, invalid, or unsupported,
6. assign stable resource keys to derived scale-bar visuals.

The rendering path should reuse ordinary scene visuals and glyph resources. Do not create a
parallel overlay renderer.


## 3D Follow-Up

The 3D API must be explicit about reference scale. Useful modes:

1. reference point in world or visual space,
2. reference camera/view-space depth,
3. object-attached reference based on selected visual bounds.

For perspective cameras, the physical scale changes with depth. Avoid a misleading automatic 3D
scale bar that silently chooses an arbitrary depth. The first 3D implementation should project or
unproject around the explicit reference point using the same panel MVP and viewport used during
frame emission.


## Example Target

Add a compact worked example after the 2D slice is visible:

1. worked-example spec:
   [../../../spec/scene/examples/core/SCALEBAR_2D_3D.md](../../../spec/scene/examples/core/SCALEBAR_2D_3D.md)
2. eventual C example target:
   `examples/c/annotations/scalebar_2d_3d.c`

The example should use two panels:

1. left panel: 2D panzoom image or scatter with a horizontal scale bar that adapts between
   micrometers, millimeters, and centimeters;
2. right panel: 3D mesh or sphere scene with an explicit-reference scale bar, for example meters
   to kilometers.


## Validation

Required before landing the first implementation slice:

1. `git diff --check`
2. `just build`
3. `just test scene`
4. focused annotation/scale-bar tests
5. focused panzoom/domain tests proving label changes across zoom levels
6. offscreen smoke proving the bar segment and label produce visible pixels

For changes touching runtime render emission or resource lifetimes, also run a narrow Vulkan-backed
scene smoke with validation layers when practical.
