# Dual Depth Peeling Follow-Up

> **Execution Status**
> - **Status:** `ACTIVE / FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** track the remaining work to turn the current depth-peeling-shaped path into real
>   fixed-iteration dual depth peeling for difficult non-convex transparent meshes.


## Current State

Durable contracts live in:

1. [`../../../spec/scene/implementation/TRANSPARENCY_MSAA.md`](../../../spec/scene/implementation/TRANSPARENCY_MSAA.md)
2. [`../../../spec/scene/semantics/TRANSPARENCY.md`](../../../spec/scene/semantics/TRANSPARENCY.md)
3. [`../../../spec/scene/implementation/GRAPH_TECHNIQUES.md`](../../../spec/scene/implementation/GRAPH_TECHNIQUES.md)

Use this file only for execution order, validation, and example guidance. Do not duplicate the
depth-peeling graph contract here.

`DVZ_ALPHA_DEPTH_PEEL` currently expands to a depth-peeling-shaped graph with opaque,
front-facing, back-facing, and composite passes. That path exercises graph-backed multi-pass
rendering, but it does not yet iteratively peel all transparent layers using previous min/max depth
bounds.


## Remaining Dual-Peeling Work

Recommended follow-up commits:

1. Add graph and lowering tests for a fixed-iteration dual-depth-peel graph without changing the
   public alpha mode.
2. Validate ping/pong min/max depth resources, front/back accumulators, and composite reads.
3. Implement DRP2/vklite execution of the fixed graph with minimal unlit shaders first.
4. Replace the scene `DVZ_ALPHA_DEPTH_PEEL` graph expansion with the fixed-iteration dual path.
5. Add lit mesh shader variants through the existing material uniform path.
6. Keep a retained quality descriptor deferred until the fixed path is correct and stable.
7. Keep the IBL BWM example default on `WBOIT` until dual depth peeling handles non-convex shells
   reliably.


## Validation Targets

1. FramePlan graph tests for fixed iteration resources, alternating ping/pong reads/writes, and
   final accumulator composite reads.
2. DRP2 semantic tests for legal command order, sampled-read/write separation, and pipeline
   attachment format matching.
3. GPU smokes for convex transparent geometry, nested shells, non-convex meshes, and resize.
4. Real-data smoke with `ibl_bwm_brain_glfw`, comparing `WBOIT`, `Depth peel`, and `Source-over`
   with shell depth-testing on and off.


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
