# Dual Depth Peeling Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / PARTIAL IMPLEMENTATION`
> - **Updated on:** `2026-05-21`
> - **Purpose:** track the remaining correctness and validation work for the current fixed-iteration
>   depth-peeling path before it can be called complete dual depth peeling for difficult non-convex
>   transparent meshes.


## Current State

Durable contracts live in:

1. [`../../../spec/scene/implementation/TRANSPARENCY_MSAA.md`](../../../spec/scene/implementation/TRANSPARENCY_MSAA.md)
2. [`../../../spec/scene/semantics/TRANSPARENCY.md`](../../../spec/scene/semantics/TRANSPARENCY.md)
3. [`../../../spec/scene/implementation/GRAPH_TECHNIQUES.md`](../../../spec/scene/implementation/GRAPH_TECHNIQUES.md)

Use this file only for execution order, validation, and example guidance. Do not duplicate the
depth-peeling graph contract here.

`DVZ_ALPHA_DEPTH_PEEL` no longer starts from a one-pass placeholder. The active implementation now
has:

1. a fixed internal iteration count, `DVZ_SCENE_DEPTH_PEEL_ITERATIONS = 4`;
2. graph resources for `front_accum`, `back_accum`, `depth_minmax_ping`, and
   `depth_minmax_pong`;
3. `opaque`, `peel.init`, `peel.iter.N`, and `peel.composite` graph passes;
4. DRP2/vklite lowering for sampled ping/pong resources and composite bind groups;
5. unlit and lit GLSL shader variants; and
6. FramePlan, DRP2, GPU smoke, and app/offscreen tests for the landed path.

Do not move this file to `agents/done/` yet. The landed path is a fixed-iteration depth-peel
implementation, but it is not proven as full dual depth peeling for difficult non-convex meshes. The
current shaders still need a correctness pass for true nearest/farthest front/back peeling, min/max
depth update semantics, and order-independent accumulation across multiple shell layers.


## Remaining Dual-Peeling Work

Recommended follow-up commits:

1. Audit the depth-peel shaders against the dual depth peeling algorithm: each iteration should use
   the previous min/max bounds to peel the next nearest front layer and farthest back layer, update
   the next min/max bounds deterministically, and accumulate front/back colors with the intended
   order and alpha convention.
2. Add a focused non-convex or nested-shell regression that fails if the implementation only handles
   the current two-layer/simple-shell cases.
3. Strengthen graph and lowering tests so they check every ping/pong iteration, sampled-read/write
   separation, composite reads, and attachment format matching rather than only counting passes.
4. Keep lit mesh variants on the existing material uniform path, but add a rendered comparison
   covering lit front/back layers once the shader semantics are corrected.
5. Keep the IBL BWM example default on `WBOIT` until depth peeling handles non-convex shells
   reliably; continue exposing `Depth peel` as a manual comparison mode.
6. Keep a retained quality descriptor deferred until the fixed path is correct and stable.


## Validation Targets

1. FramePlan graph tests for fixed iteration resources, alternating ping/pong reads/writes, and
   final accumulator composite reads.
2. DRP2 semantic tests for legal command order, sampled-read/write separation, and pipeline
   attachment format matching.
3. Shader-level or rendered tests that distinguish true dual peeling from the current simple
   front/back-layer behavior.
4. GPU smokes for convex transparent geometry, nested shells, non-convex meshes, resize, and
   descriptor refresh.
5. Real-data smoke with `examples/c/showcase/ibl_brain`, comparing `WBOIT`, `Depth peel`, and
   `Source-over` with shell depth-testing on and off.


## Validation

For docs-only changes, run:

```text
rg for old moved filenames and stale soon/spec links
git diff --check
git status --short
```

For implementation changes, use:

```text
just build
just test scene
just test drp2
```

Run bounded GLFW examples when shader, graph-resource, or runtime descriptor behavior changes.
