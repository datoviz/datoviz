# Text Primitive Evaluation

> **Status:** Historical Decision
> **Decided on:** 2026-05-28
> **Scope:** Stage 5 evaluation record from
> [`../../SCENE_SPLIT_REFACTOR_PLAN.md`](../../SCENE_SPLIT_REFACTOR_PLAN.md).


## Context

The scene split refactor plan identifies `text` as a candidate for future scene-independent CPU
primitives. Text has reusable subproblems such as font defaults, glyph metrics, shaping, layout,
and atlas preparation, but the current v0.4 text work is still primarily scene-facing.

The active text contract depends on retained `DvzText` state, panel and figure placement,
annotations, overlay layout, invalidation, atlas resource refresh, and DRP2/runtime upload
scheduling. App overlays and examples can use scene-owned text for the current release slice; there
is not yet a proven non-scene caller that needs standalone shaping or layout before constructing a
scene object.


## Decision

Do not promote `text` during this evaluation pass.

Text remains a scene-owned candidate. Font discovery policy, glyph metrics, shaping, layout, and
CPU atlas preparation may move later only as pure CPU primitives with explicit descriptors and
output buffers.

Retained text visuals, annotation text, axes and colorbar labels, panel-aware overlay layout,
atlas GPU resource ownership, atlas upload scheduling, glyph visual lowering, picking identity,
and scene invalidation remain scene responsibilities.


## Promotion Criteria

Promote `text` only when all of these are true:

1. App overlays, docs/examples, Python/GSP, VisPy2, or external UI need text shaping or layout
   without constructing `DvzScene`, `DvzFigure`, or `DvzPanel`.
2. Tests can validate font selection, glyph metrics, shaping, layout, and CPU atlas preparation
   without rendering a scene.
3. The public API operates on explicit font/layout descriptors, UTF text input, and caller-visible
   output buffers or run records.
4. The primitive API does not mention panels, annotations, visuals, DRP2 resources, GPU textures,
   atlas upload scheduling, or scene registries.
5. Scene can consume the promoted primitives as a client while retaining semantic text ownership,
   invalidation, placement, and runtime resource ownership.


## Consequences

The current text, annotation, axes, labels, colorbar, and overlay work stays on the active
scene-owned path for the feature-freeze window.

Future text extraction should start with a CPU-only shaping/layout proof and tests before moving
any atlas or runtime behavior. Until then, text-related cleanup should split code internally within
`src/scene` only when it clarifies retained text ownership or lowering.

This record intentionally changes documentation only. It does not require source, CMake, header, or
test changes.
