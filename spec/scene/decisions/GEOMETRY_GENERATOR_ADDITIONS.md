# Geometry Generator Additions

> **Status:** Historical Decision
> **Decided on:** 2026-05-28
> **Scope:** Stage 6 evaluation record from
> [`../../architecture/SCENE_SPLIT_REFACTOR_PLAN.md`](../../architecture/SCENE_SPLIT_REFACTOR_PLAN.md).


## Context

The split refactor plan identifies `geom` as an existing scene-independent module that may absorb
more pure CPU generators as visual families need them. Current geometry utilities already provide
the active baseline for owned geometry buffers, simple mesh generators, bounds, transforms, normal
work, edge derivation, contour segments, and scene mesh upload from `DvzGeometry`.

Future visual families and examples may need arrows, tubes, ribbons, vector glyph geometry,
orientation gizmo meshes, polygon triangulation, curve tessellation, cut surfaces, and contour
stitching. Those algorithms are good fits for `geom` only when they are CPU data-preparation
helpers rather than retained scene objects.


## Decision

Do not add a broad geometry-generator expansion during this evaluation pass.

Keep `geom` as the home for pure CPU geometry generation, but add new generators only when an
active visual family, example, or validation slice needs them. Each addition should be narrow,
tested, and usable without constructing `DvzScene`, `DvzFigure`, `DvzPanel`, or a retained visual.

Scene visual constructors, panel attachment, attribute uploads, material decisions, depth policy,
controller binding, generated visual ownership, and DRP2/runtime resource scheduling remain scene
responsibilities.


## Addition Criteria

Add a geometry generator to `geom` only when all of these are true:

1. A concrete vector, tube, ribbon, contour, cut-plane, gizmo, polygon, annotation, or example
   workflow needs the generated CPU geometry.
2. Tests can validate topology, bounds, indices, normals, provenance, and error handling without
   scene or GPU setup.
3. The public API operates on plain descriptors, arrays, `DvzGeometry`, or Datoviz-owned output
   buffers.
4. The generator does not create visuals, attach to panels, choose scene styles, allocate GPU
   resources, or schedule uploads.
5. The dependency choice is settled enough for the default build, or the backend remains explicitly
   optional and does not widen the release surface unexpectedly.


## Consequences

The existing scene and mesh utility surface remains stable while v0.4 hardening continues.

Future generator work should be feature-driven: implement the smallest CPU helper that unlocks the
active visual or example, then lower its output through existing scene resource and visual paths.
Until a concrete workflow requires a generator, keep the design in scene specs or proposals rather
than adding unused public API.

This record intentionally changes documentation only. It does not require source, CMake, header, or
test changes.
