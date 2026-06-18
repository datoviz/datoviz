# Datoviz Module Layers

> **Status:** active architecture note
> **Updated on:** 2026-05-27
> **Scope:** source-tree and public-module layering for reusable Datoviz subsystems.


## Purpose

Datoviz v0.4 keeps one aggregate shared library, but its source modules should still express a
clear dependency graph. The scene layer has grown into a high-level visualization subsystem, and
some code currently under `src/scene` is useful without the retained scene graph. Those reusable
pieces should move into scene-independent public modules when they have a clear second consumer
such as VisPy2, app overlays, tests, or future bindings.

The concrete scene split plan is [SCENE_SPLIT_REFACTOR_PLAN.md](SCENE_SPLIT_REFACTOR_PLAN.md).


## Layer Model

The intended source layers are:

1. **Foundation:** `common`, `math`, `thread`, `fileio`.
2. **Domain primitives:** scene-independent visualization primitives such as `geom`, `controller`,
   and future candidates like `color`, `field`, and `text`.
3. **Runtime/backend:** `input`, `window`, `vk`, `vklite`, `canvas`, `stream`, `video`.
4. **Render protocol:** `drp2`.
5. **High-level systems:** `scene`, `app`, and optional GUI integration.

Top-level source directories may remain siblings even when they belong to different layers. The
layering should be enforced by CMake targets, include dependencies, tests, and documentation rather
than by broad buckets such as `utils/` or `runtime/`.


## Promotion Rules

A module should move out of `scene` only when all of these are true:

1. it can be tested without constructing a `DvzScene`,
2. it does not include `_scene.h` or depend on retained scene object ownership,
3. it has a plausible non-scene consumer,
4. its public API does not expose scene handles unless through compatibility wrappers,
5. scene can consume it as a client instead of owning the implementation.

Scene-specific wrappers, panel binding, controller links, invalidation, and retained-object lifetime
remain in `scene`.


## First Extraction: Controller

The first promoted domain primitive is `controller`.

It owns scene-independent camera/controller state machines:

1. `DvzCamera`,
2. `DvzPanzoom`,
3. `DvzArcball`,
4. `DvzFly`,
5. `DvzTurntable`.

The public controller API is available through `datoviz/controller.h` and
`datoviz/controller/*.h`. Existing `datoviz/scene/*.h` controller headers remain compatibility
wrappers for scene-owned constructor APIs such as `dvz_panzoom(DvzScene*, ...)` and
panel-owned camera APIs such as `dvz_panel_set_camera()`.

The implementation boundary is:

1. `controller` owns pure allocation, state mutation, input-event interpretation, matrix/camera
   updates, and destruction for individual controllers,
2. `scene` owns `DvzController*` handles, links between scene-owned controllers, panel binding,
   figure invalidation, and app-facing controller integration,
3. `app` may use scene-owned controllers through the scene API until it needs a separate hosted
   controller path.


## Future Candidates

Likely future promotions from `scene` are:

1. `color`: colormaps, categorical palettes, transfer functions, colorizers, and sparse label color
   lookup helpers.
2. `field`: sampled-field descriptors, regions, sample profiles, scalar/RGBA/label interpretation,
   and update validation.
3. `text`: font defaults, shaping, glyph metrics, atlas generation/cache, and text block layout.
4. `geom`: additional scene-independent geometry generators such as polygon tessellation, arrows,
   tubes, marker shapes, and orientation gizmo axes.

`FramePlan`, visual pipeline descriptors, render contracts, and query semantics should stay in
`scene` until a non-scene producer needs them.
