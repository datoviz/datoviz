# Color And Field Primitive Evaluation

> **Status:** Historical Decision
> **Decided on:** 2026-05-28
> **Scope:** Stage 3 and Stage 4 evaluation records from
> [`../../architecture/SCENE_SPLIT_REFACTOR_PLAN.md`](../../architecture/SCENE_SPLIT_REFACTOR_PLAN.md).


## Context

The scene split refactor plan identifies `color` and `field` as candidates for future
scene-independent modules. Both areas contain reusable CPU logic, but the current implementation
uses them through scene-owned retained resources, visual bindings, panel semantics, and runtime
upload scheduling.

The v0.4 branch currently needs stable scene behavior more than another public module boundary.
No concrete non-scene consumer has appeared that requires standalone color conversion or sampled
field interpretation before handing data to `DvzScene`.


## Decision

Do not promote `color` or `field` during this evaluation pass.

`color` remains a scene-owned candidate. Colormap definitions, categorical palettes, scalar-to-color
conversion, label color lookup, and transfer-table helpers may move later only when a non-scene
consumer needs them through descriptors, arrays, and output buffers.

`field` remains a scene-owned candidate. Sampled-field descriptors, dimension and format validation,
scalar/RGBA/label interpretation, CPU subregion checks, and value/category-domain helpers may move
later only as CPU-only primitives. Retained `DvzField` ownership, field-to-visual binding, texture
allocation, upload scheduling, dirty ranges, live-stream guards, and cross-scene rejection remain
scene responsibilities.


## Promotion Criteria

Promote `color` only when all of these are true:

1. GSP, VisPy2, Python bindings, examples, field conversion, or another non-scene caller needs a
   standalone color API.
2. Tests can validate colorizers, colormap lookup, categorical palettes, and label color lookup
   without constructing `DvzScene`, `DvzFigure`, or `DvzPanel`.
3. The public API operates on plain descriptors, arrays, and output buffers, with scene wrappers
   left under `datoviz/scene/*.h` only where scene ownership is required.
4. The new module target does not depend on `scene` and creates no CMake dependency cycle.

Promote `field` only when all of these are true:

1. Non-scene code needs to validate, describe, or interpret sampled arrays before handing them to
   scene.
2. Tests can run as CPU-only validation and interpretation tests without retained scene objects.
3. The public primitive does not mention panels, visuals, DRP2 resources, GPU textures, or scene
   registries.
4. Scene can consume the promoted primitives as a client while retaining GPU/resource ownership.
5. The initial split moves CPU descriptors and validation first, not retained resource behavior.


## Consequences

Scene scale, colorbar, legend, field, sampled visual, and upload behavior stays stable for the
feature-freeze window.

Future color or field extraction should start from the promotion criteria above and the general
promotion test in the split plan. Until then, changes in these areas should split code internally
within `src/scene` only when it clarifies scene ownership.

This record intentionally changes documentation only. It does not require source, CMake, header, or
test changes.
