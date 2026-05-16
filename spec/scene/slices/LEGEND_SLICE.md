# Legend Slice

This document defines the implementation boundary for discrete legends.

It is intentionally marked as a later slice. Continuous colorbars should be implemented first via
[COLORBAR_RENDERING_SLICE.md](COLORBAR_RENDERING_SLICE.md).


## Status

Status: deferred implementation slice.

The semantic model exists in [../semantics/LEGENDS_AND_COLORBARS.md](../semantics/LEGENDS_AND_COLORBARS.md),
but the active public API currently has `DvzColorbar` and does not yet expose a dedicated
`DvzLegend` handle.


## Preconditions

Do not implement this slice until these are true:

1. categorical scale labels and ordering are represented in retained scene state,
2. rendered text is active,
3. label annotation rendering is active,
4. continuous colorbar rendering has established panel-edge layout conventions,
5. the public API decision for `DvzLegend` versus legend-like `DvzColorbar` variants is recorded in
   `../api/API_SURFACE.md`.


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


## Required Public API Decision

Before implementation, decide and document the minimal public surface:

1. `DvzLegend* dvz_legend(DvzPanel* panel, DvzScale* scale, const DvzLegendDesc* desc)`, or
2. a narrower categorical colorbar/legend constructor if `DvzLegend` is intentionally deferred.

The semantic specs prefer a distinct `DvzLegend` handle. The current installed headers do not yet
provide one, so implementation should not invent hidden behavior behind `DvzColorbar`.


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


## Validation

Validate before planning:

1. legend panel and scale belong to the same scene,
2. scale kind is categorical or discrete,
3. category labels are present or a deterministic fallback label policy is documented,
4. category order is stable,
5. sample mark type is supported by the active visual contribution path,
6. runtime supports text rendering.


## FramePlan Contribution

A legend contributes:

1. optional title glyphs,
2. one glyph contribution per label or one batched text contribution,
3. one sample mark contribution per entry,
4. optional border/background if the style contract requires it.

The first implementation should batch entries where practical but preserve entry identity metadata
for future picking.


## Tests

Add focused tests for:

1. stable entry order,
2. label fallback behavior,
3. cross-scene scale rejection,
4. categorical-only validation,
5. destroy lifecycle,
6. emission of text plus sample mark contributions.


## Acceptance

This slice is complete when:

1. the public `DvzLegend` boundary is explicit,
2. one categorical scale can produce a visible panel-attached legend,
3. legend ordering is deterministic,
4. unsupported aggregation and unsupported scale kinds produce scene diagnostics,
5. tests cover the retained state, validation, emission, and lifecycle paths.

