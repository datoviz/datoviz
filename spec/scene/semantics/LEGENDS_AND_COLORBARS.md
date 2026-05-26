# Scene Legends And Colorbars

Status: normative v0.4 scene semantics spec.

Legends and colorbars are annotation-side semantic objects. They explain scene mappings and emit
ordinary contributions; they are not visual families, backend widgets, or shader/pipeline details.


## Purpose

Legends and colorbars expose human-readable meaning for visual encodings, support panel-local and
shared summaries, fit annotation layout/export/interaction, and preserve scene semantics across
capability fallbacks.


## API Model

`DvzColorbar` and `DvzLegend` are installed as retained API handles. `DvzColorbar` explains
continuous or ordered scalar mappings, while `DvzLegend` explains categorical/discrete mappings.
Legends and colorbars remain distinct API concepts because their content models differ.

Both may reference a first-class `DvzScale*` from [SCALES.md](SCALES.md). They do not own the scale.


## Core Rules

1. Explain semantic mappings, not descriptor bindings, texture units, uniform layouts, or shader
   specialization.
2. Aggregate only mappings with identical semantic identity unless the user explicitly declares a
   shared explanatory object.
3. Keep anchor, attachment, placement, content derivation, and interaction policy explicit.
4. Reuse annotation, resource, transform, and invalidation policy by link instead of restating it.


## Terminology

| Term | Meaning |
|---|---|
| legend | discrete/semi-discrete entries for categories, styles, shapes, sizes, states, or variants |
| colorbar | continuous or sampled scalar/ordered mapping shown as a ramp with ticks/labels |
| scale object | semantic mapping such as palette, colormap domain, size, opacity, or symbol map |
| visual mapping | active relationship between scene data/parameters and visible encoding |


## Attachment And Ownership

| Scope | Use |
|---|---|
| panel-attached | default; explains what one panel shows |
| scene-shared | shared consolidated mapping across panels/export layout |
| visual-attached | one mapping for one visual, usually placed in a panel |
| axis-attached | colorbar/ramp aligned with axis-side domain interpretation |

Legends and colorbars are annotation classes; shared anchor/placement behavior is defined in
[ANNOTATIONS.md](ANNOTATIONS.md).

The installed colorbar and legend APIs support two placement modes:

1. attached explanatory objects contribute fixed logical pixels on a panel edge to the panel's resolved
   reserve and render in that panel's adornment band;
2. detached explanatory objects do not reserve plot space and use explicit `DvzPlacement` anchored in panel
   or figure pixel space.

Attached colorbars and legends use the same panel reserve aggregation path as axes. This keeps data
visuals, axis geometry, and explanatory-object geometry aligned on one resolved plot rectangle while
preserving separate retained objects and invalidation rules.


## Source Of Truth And Mapping Identity

The source of truth is a scene-semantic mapping description from visual parameters, explicit scale
objects, variants, categorical metadata, axis-side domain interpretation, or filtering/selection
state. Backend shader code and texture objects without semantic range information are insufficient.

A stable mapping identity must capture:

1. semantic quantity or category set;
2. mapping policy (palette, continuous colormap, size scale, symbol map, etc.);
3. domain/category definitions;
4. interpretation details that change meaning.


## Content Models

| Object | Required conceptual content |
|---|---|
| legend entry | id, label, sample mark(s), optional association, state, optional interaction |
| colorbar | id, domain, scale, tick/label policy, orientation, thresholds, units/title, out-of-range policy |

Sample marks and ramps may lower to `marker`, `segment`, `path`, `glyph`, or `image`, but that is an
implementation consequence.


## Discrete, Continuous, Mixed

| Explanation | Examples |
|---|---|
| discrete | categories, clusters, selection states, marker shapes, line dashes |
| continuous | intensity, probability, temperature, elevation, transfer-function domain |
| mixed | ramp with thresholds, segmented bands, categorical legend plus size scale |


## Layout, Ordering, Visibility, Interaction

| Policy | Rules |
|---|---|
| placement | attached panel edge, detached anchored placement, axis side, export layout region, shared multi-panel region, callout-like placement |
| ordering/grouping | preserve user order; allow grouping by visual/section; deterministic export order |
| visibility | explicit rules: always, attached-visual, export-only, hidden, collapsed, or omitted |
| interaction | passive by default; may support pickable entries, category toggles, hover highlight, linked filtering |

Interactive behavior must mutate scene/controller state rather than bypassing the scene layer.


## Resources, Transforms, Invalidation

Typical inputs include `ParameterBlockResource`, `SampledField`, `DerivedField`, small derived
`ItemTable`-like resources, and panel-local label/layout resources. Placement is usually panel- or
viewport-relative; semantic mapping and ticks may still originate in data/scale space.

Common invalidation sources:

1. visual visibility or variant changes;
2. scale domain, colormap, palette, category set, units, or label format changes;
3. panel size/layout changes;
4. interaction state for interactive legends;
5. export-target policy changes.

Distinguish placement-only updates, entry/tick regeneration, ramp regeneration, full layout
recomputation, and upload changes. Shared dirty scopes are canonical in
[`../pipeline/INVALIDATION_AND_CACHING.md`](../pipeline/INVALIDATION_AND_CACHING.md).


## Export And Adaptation

Export may request consolidated, higher-resolution, differently placed, or export-only explanatory
objects. Capability fallback should preserve meaning first: omit interactivity, reduce ramp
fidelity, simplify grouped legends, or suppress decorative sample marks before dropping core labels,
ticks, or semantic entries.


## Validation

Scene validation should catch:

1. colorbar requested for a visual without scalar mapping;
2. missing category labels for legend entries;
3. incompatible implicit aggregation;
4. log-scale colorbar with invalid domain;
5. interactive legend without required picking support;
6. axis-attached colorbar on an incompatible panel or visual mode.

Validation rules live in `../validation/VALIDATION.md`. Implementation slices are
[`../slices/COLORBAR_RENDERING_SLICE.md`](../slices/COLORBAR_RENDERING_SLICE.md) and
[`../slices/LEGEND_SLICE.md`](../slices/LEGEND_SLICE.md).
