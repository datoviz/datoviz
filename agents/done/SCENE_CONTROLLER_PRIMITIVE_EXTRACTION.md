# Scene Controller Primitive Extraction

> **Status:** done
> **Created on:** 2026-05-27
> **Scope:** promote camera and controller state machines from `scene` into a public
> scene-independent `controller` module.


## Result

The extraction landed as a public `controller` module:

1. `src/controller` owns the standalone `DvzCamera`, `DvzPanzoom`, `DvzArcball`, `DvzFly`, and
   `DvzTurntable` implementations.
2. `include/datoviz/controller.h` and `include/datoviz/controller/*.h` expose the scene-independent
   public API.
3. `include/datoviz/scene/{camera,panzoom,arcball,fly,turntable}.h` remain scene wrapper headers for
   scene-owned constructors and panel camera APIs.
4. `scene` consumes controller primitives through `datoviz_controller` and keeps ownership of
   `DvzController*`, panel binding, controller links, invalidation, and scene/app integration.
5. `testing/components/dvztest_controller.c` and `src/controller/tests/test_controller.c` cover
   controller-only construction without scene ownership.


## Goal

Make Datoviz camera/controller primitives reusable without requiring the retained scene graph. This
is the first concrete extraction from the broader module-layering direction in
[`../../../spec/MODULE_LAYERS.md`](../../../spec/MODULE_LAYERS.md) and the broader scene split plan
in [`../../../spec/SCENE_SPLIT_REFACTOR_PLAN.md`](../../../spec/SCENE_SPLIT_REFACTOR_PLAN.md).


## Boundary

Move to `controller`:

1. pure `DvzCamera` creation, mutation, MVP emission, and destruction,
2. pure `DvzPanzoom`, `DvzArcball`, `DvzFly`, and `DvzTurntable` state machines,
3. direct pointer/keyboard/input-router event handling for those primitives.

Keep in `scene`:

1. scene-owned `DvzController*` handles,
2. `dvz_panzoom(DvzScene*, ...)`, `dvz_arcball(DvzScene*, ...)`,
   `dvz_fly(DvzScene*, ...)`, and `dvz_turntable(DvzScene*, ...)`,
3. panel camera ownership through `dvz_panel_set_camera()` and `dvz_panel_camera()`,
4. controller links, panel binding, figure invalidation, and scene/app integration.


## Compatibility

Existing `datoviz/scene/{camera,panzoom,arcball,fly,turntable}.h` headers remain available as
wrappers. New scene-independent users should include `datoviz/controller.h` or the focused
`datoviz/controller/*.h` headers.


## Validation

For each extraction slice:

1. `git diff --check`
2. `just build`
3. `just test scene`

Add controller-only tests once the first scene-independent API slice is stable.
