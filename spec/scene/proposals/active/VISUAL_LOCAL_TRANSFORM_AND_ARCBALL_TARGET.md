# Visual Local Transform And Camera Navigation

> **Status:** superseded for v0.4 RC
> **Updated on:** 2026-06-28
> **Implementation:** visual-local transforms landed; the separate camera-orbit controller is
> deferred until its interaction semantics are settled.


## Landed Decision

Visual-local transforms are retained visual state. They are composed through the normal MVP path,
affect rendering and queries consistently, and avoid reuploading geometry for pure placement,
rotation, or animation changes.

`DvzArcball` remains an object/model controller. `DvzTurntable` is the stable-up camera orbit
controller for object or scene inspection. `DvzFly` covers free camera navigation and pivot-style
camera gestures.


## Deferred Work

A roll-capable camera-orbit controller may return later, but it should not ship until the controller
has a clear name, explicit target semantics, pole behavior, and documented sci-viz use cases that
are not already covered by turntable or fly navigation.


## Validation Baseline

```sh
direnv exec . just test scene
direnv exec . just example-c textured_planet 3
git diff --check
```
