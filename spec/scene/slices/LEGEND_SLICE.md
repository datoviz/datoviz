# Legend Slice

This document defines the implementation boundary for discrete legends.

Continuous colorbars have established the first panel-edge layout conventions via
[COLORBAR_RENDERING_SLICE.md](COLORBAR_RENDERING_SLICE.md). Legends now follow as a narrow
categorical explanatory-object slice.


## Status

Status: ready for implementation after the retained categorical scale entry API is added.

The semantic model exists in [../semantics/LEGENDS_AND_COLORBARS.md](../semantics/LEGENDS_AND_COLORBARS.md),
and [../api/API_SURFACE.md](../api/API_SURFACE.md) records the decision to use a dedicated
`DvzLegend` handle rather than categorical `DvzColorbar` behavior.


## Preconditions

Proceed once these are true:

1. categorical scale labels, ids, sample colors, and ordering are represented in retained scene state,
2. rendered text is active,
3. label annotation rendering is active,
4. continuous colorbar rendering has established panel-edge layout conventions,
5. the public API decision for `DvzLegend` is recorded in `../api/API_SURFACE.md`.


## Scope

The first legend slice should support:

1. one panel-attached legend,
2. one categorical scale source,
3. stable category order,
4. one text label per entry,
5. one sample mark per entry,
6. vertical stacked layout,
7. optional title.


## Non-Goals

Do not implement these in the first legend slice:

1. multi-visual aggregation beyond identical scale identity,
2. interactive filtering,
3. grouped sections,
4. dense packing or collision avoidance,
5. continuous ramps,
6. sample marks for every visual styling dimension.


## Public API Decision

Use a dedicated retained legend handle:

```c
DVZ_EXPORT DvzLegend* dvz_legend(
    DvzPanel* panel, DvzScale* scale, const DvzLegendDesc* desc);

DVZ_EXPORT void dvz_legend_destroy(DvzLegend* legend);

DVZ_EXPORT bool dvz_legend_set_layout(DvzLegend* legend, const DvzLegendDesc* desc);

DVZ_EXPORT void dvz_legend_set_title(DvzLegend* legend, const char* title);
```

Do not implement categorical legends by accepting categorical scales in `dvz_colorbar()`.
Colorbars and legends remain separate explanatory object families.


## First-Slice Layout Decisions

Use the colorbar placement vocabulary where it applies:

1. attached legends reserve a fixed logical-pixel band on one panel edge,
2. detached legends use explicit `DvzPlacement` in panel or figure pixel space,
3. first implementation may support only attached panel-edge legends,
4. first implementation uses vertical stacked entries,
5. first implementation does not run collision avoidance or tight layout,
6. shared/grid-slot legends are deferred until fixed-size grid slots are implemented.

Recommended first descriptor fields:

```c
struct DvzLegendDesc
{
    DvzLegendPlacementMode placement_mode;
    DvzSceneAnchor anchor;
    const char* title;
    float reserve_px;
    float edge_offset_px;
    float plot_gap_px;
    float entry_gap_px;
    float mark_size_px;
    float mark_label_gap_px;
    DvzPlacement placement;
    uint32_t flags;
};
```

If a separate `DvzLegendPlacementMode` is unnecessary, reuse the colorbar placement-mode enum only
after confirming the name does not make the public API read as colorbar-specific.


## Retained State

The first legend needs retained state for:

1. panel,
2. categorical scale,
3. title,
4. anchor,
5. orientation or stack direction,
6. visibility policy,
7. ordered category entries,
8. per-entry label,
9. per-entry color or sample mark description.

The ordered category entries come from the referenced categorical scale. Legend-specific entry
overrides are out of scope for the first slice.


## Validation

Validate before planning:

1. legend panel and scale belong to the same scene,
2. scale kind is categorical or discrete,
3. category labels are present or a deterministic fallback label policy is documented,
4. category order is stable,
5. sample mark type is supported by the active visual contribution path,
6. runtime supports text rendering,
7. continuous scales are rejected with an explicit diagnostic that a colorbar should be used.


## FramePlan Contribution

A legend contributes:

1. optional title glyphs,
2. one glyph contribution per label or one batched text contribution,
3. one sample mark contribution per entry,
4. optional border/background if the style contract requires it.

The first implementation should batch entries where practical but preserve entry identity metadata
for future picking.

Lowering decision for the first slice:

1. labels and title use the existing text/glyph path,
2. sample marks use one generated primitive or marker visual,
3. background and border are deferred unless tests show labels are unreadable without them,
4. legend-derived visuals use stable resource labels such as `legend.0.marks`,
   `legend.0.labels`, and `legend.0.labels.glyph`.


## Tests

Add focused tests for:

1. stable entry order,
2. label fallback behavior,
3. cross-scene scale rejection,
4. categorical-only validation,
5. destroy lifecycle,
6. emission of text plus sample mark contributions.
7. repeated prepare/emission is idempotent when scale entries and panel size are unchanged.


## Acceptance

This slice is complete when:

1. the public `DvzLegend` boundary is explicit,
2. one categorical scale can produce a visible panel-attached legend,
3. legend ordering is deterministic,
4. unsupported aggregation and unsupported scale kinds produce scene diagnostics,
5. tests cover the retained state, validation, emission, and lifecycle paths.
