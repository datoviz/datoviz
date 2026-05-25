# Annotation Label Slice

This slice records the first rendered annotation path: `dvz_annotation_label()`.

Status: first rendered label slice landed.

It depends on [TEXT_RENDERING_SLICE.md](TEXT_RENDERING_SLICE.md). Labels should reuse the same text
renderer rather than creating a second text path.


## Scope

Visible label annotations attached to a panel are implemented as the first annotation rendering
slice.

The landed first slice supports:

1. `DVZ_ANNOTATION_LABEL`,
2. retained text content from `DvzAnnotation.text`,
3. `DvzTextStyle`,
4. `DvzTextPlacement`,
5. optional `DvzFormatDesc` override for labels that derive display text from a value later,
6. panel clipping,
7. update and destroy lifecycle.


## Non-Goals

Do not implement these in the first label slice:

1. scale bars,
2. dimension annotations,
3. callout leader lines,
4. hover tooltips,
5. crosshair guides,
6. legend entries,
7. annotation-object picking.

Those features should be separate slices once label rendering is stable.


## Public API Boundary

Use the installed APIs:

1. `dvz_annotation()`,
2. `dvz_annotation_label()`,
3. `dvz_annotation_destroy()`,
4. `dvz_annotation_set_format()`.

Do not add a new public annotation subtype handle in this slice.


## Retained State

The existing `DvzAnnotation` handle is the current retained semantic object for labels.

Use these fields as source of truth:

1. `scene`,
2. `panel`,
3. `kind`,
4. `text`,
5. `style`,
6. `placement`,
7. `flags`,
8. `has_format`,
9. `format`.

The implementation may internally lower a label into a derived `DvzText`-like contribution, but it
must preserve annotation identity for diagnostics and future picking.


## Dirty Rules

Label invalidation follows text invalidation with annotation-specific triggers:

1. text changes dirty label layout and glyph coverage,
2. style changes dirty layout or upload state as in text,
3. placement changes dirty label geometry,
4. format changes dirty derived display content only when the label is value-backed,
5. annotation destroy removes all derived contributions before the next frame emission.


## Validation

Validate before planning:

1. annotation panel is live and belongs to the annotation scene,
2. annotation kind is supported by this slice,
3. referenced font is NULL or belongs to the same scene,
4. placement coordinates are finite,
5. label text is present or empty text is treated as non-rendered,
6. requested flags do not require unsupported callout, guide, or picking behavior.

Unsupported annotation kinds should be ignored only if explicitly marked non-rendered; otherwise
emit a scene diagnostic.


## FramePlan Contribution

Labels lower into the same panel-local glyph contribution used for text.

The contribution must retain enough metadata to identify:

1. the originating `DvzAnnotation`,
2. the panel,
3. the placement mode,
4. whether it is ordinary text, annotation text, or future value-derived text.

This metadata is for diagnostics and future picking, not for backend execution.


## Tests

Focused scene tests should cover or keep covering:

1. `dvz_annotation_label()` emits a glyph/text contribution,
2. unsupported annotation kinds produce diagnostics or no-op behavior according to flags,
3. destroyed annotations stop emitting,
4. labels respect panel clipping,
5. labels can share font and atlas resources with ordinary `DvzText`,
6. cross-scene font binding is rejected.

Keep at least one offscreen or example smoke with an image or point visual plus a label overlay in
release validation.


## Acceptance

This slice is complete for the first rendered label path when:

1. `dvz_annotation_label()` renders visible text,
2. it uses the same text renderer and atlas resource path as `DvzText`,
3. label lifecycle is covered by focused tests,
4. unsupported annotation kinds are explicit diagnostics or documented no-ops,
5. later callout, readout, and measurement slices can reuse the label placement and formatting path.


## Remaining Work

Follow-up work belongs outside this closed first slice:

1. rendered pinned readout cards,
2. callout leader lines,
3. crosshair guides,
4. dimension annotations,
5. annotation-object picking,
6. richer data/world anchoring and collision behavior.
