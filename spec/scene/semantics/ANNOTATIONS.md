# Scene Annotations

Status: normative v0.4 scene semantics spec.

Annotations are semantic scene objects for explanatory, diagnostic, and interactive context. They
may emit ordinary visual-family or overlay contributions, but they are not merely draw calls or
backend concepts.


## Purpose

Annotations attach context to scene content, support static and interaction-driven overlays, honor
panel transforms/picking/invalidation, and keep public concepts independent from glyph buffers,
segment batches, texture quads, or overlay passes.


## API Model

The current installed API exposes one retained `DvzAnnotation` handle for the first annotation slice,
including label annotations. Future subtypes may use dedicated handles when their content models
justify it. Shared behavior belongs here: attachment, anchor, placement, invalidation, z-order, and
pick identity.

Colorbars are dedicated `DvzColorbar` objects and legends are dedicated explanatory objects in the
semantic model; neither should become ordinary annotation subtypes. See
[LEGENDS_AND_COLORBARS.md](LEGENDS_AND_COLORBARS.md).


## Core Rules

1. Model annotations by semantic role, not rendering technique.
2. Keep anchor and placement separate.
3. Keep attachment scope explicit.
4. Route interaction through scene/controller state.
5. Use ordinary family contributions where possible.
6. Integrate with normal dirty scopes and `FramePlan` assembly.


## Annotation Classes

| Class | Meaning/examples |
|---|---|
| label | point labels, panel titles, axis-adjacent labels, selection labels, static text |
| guide | threshold lines, boxes, region outlines, reference planes |
| probe | hover/readout/nearest-item/sampled-value summaries |
| crosshair | cursor-aligned panel guides, often linked to probes or panels |
| callout | target anchor, leader/guide, text block/badge, optional emphasis |
| overlay | panel/viewport coordinates: corner labels, status badges, export stamps, HUD text |
| legend/colorbar | mapping summaries; specified in [LEGENDS_AND_COLORBARS.md](LEGENDS_AND_COLORBARS.md) |


## Attachment Scope

| Scope | Examples |
|---|---|
| scene-global | shared titles, mirrored probe state, global explanatory overlays |
| panel-attached | panel title, crosshair, corner overlay, panel-local legend |
| visual-attached | selected-point labels, visual threshold guide, visual-specific readout |
| axis-attached | axis title, domain marker, colorbar, ruler/probe |
| interaction-derived | hover tooltip, drag rectangle, cursor probe, measurement guide |


## Anchor Model

Every annotation defines what it refers to before placement chooses where it is drawn.

| Anchor | Meaning |
|---|---|
| data-space | semantic coordinate/domain value such as threshold `x = 3.2` |
| visual-space | normalized/layout position before panel transform |
| panel-space | panel-local coordinates for crosshairs or drag boxes |
| viewport-relative | panel edge/corner placement |
| identity | picked/selected item, span, group, or visual identity |
| sampled-value | field/probe result that supplies content and/or placement |


## Placement Model

Placement policies may include direct anchor placement, offsets, edge docking, collision avoidance,
leader-line/callout placement, and panel-relative stacking. Viewport-relative overlays may bypass
`DataSpace`; data/visual anchors follow the transform discipline in
[`../pipeline/TRANSFORM_PIPELINE.md`](../pipeline/TRANSFORM_PIPELINE.md).


## Contributions

One logical annotation may emit multiple contribution types:

| Annotation | Possible contributions |
|---|---|
| label | `glyph` plus optional background geometry |
| crosshair | one or more `segment` contributions |
| probe | `glyph`, `segment`, highlight marker |
| callout | leader geometry, text, emphasis marker |
| overlay | `glyph`, `image`, `segment`, or panel-local primitives |

The public semantic model must not expose those realization choices as the primary API.


## Interaction And Picking

Annotations may be persistent, controller-lifetime, picking-result-driven, or mirrored across linked
panels. If pickable, they pick by scene identity:

| Pick mode | Meaning |
|---|---|
| non-pickable | ignored by picking |
| object-pickable | returns the annotation id |
| subpart-pickable | returns entry/badge/handle identity when needed |

Interaction-driven probes and annotation picks should align with `../interaction/PICKING.md` and
`../interaction/CONTROLLERS.md`.


## Invalidation And Resources

Common dirty sources are content changes, anchor changes, placement policy, panel transform,
interaction state, picking results, attached visual/axis state, panel layout/size, and export target
policy. Distinguish cheap movement, semantic content rebuild, layout recomputation, and upload
changes; dirty scopes are canonical in
[`../pipeline/INVALIDATION_AND_CACHING.md`](../pipeline/INVALIDATION_AND_CACHING.md).

Annotations may reuse shared resources when semantics allow, but panel-local derived resources are
valid for viewport layout, text placement, picking overlays, linked-panel mirror state, and
export-only overlays.


## Export And Capability Adaptation

Annotations are first-class export concerns: they may be export-only, hidden in interactive display,
or target-specific. Capability gaps such as text quality, picking support, layered composition, or
dynamic resource limits require explicit semantic fallback rather than backend escape hatches.


## Boundaries

This document does not freeze the final public API, text shaping/font pipeline, overlay pass
topology, styling vocabulary, collision-avoidance algorithm, or exact DRP2 command sequence.
Implementation slices: [`../slices/ANNOTATION_LABEL_SLICE.md`](../slices/ANNOTATION_LABEL_SLICE.md);
measurement annotations wait for label and text rendering slices.
