# Dual Depth Peeling Plan

> **Execution Status**
> - **Status:** `PLANNING`
> - **Updated on:** `2026-05-19`
> - **Purpose:** turn the current scene depth-peeling-shaped path into real dual depth peeling for
>   complex non-convex transparent meshes such as the IBL BWM brain shell.


## Durable Contract

Use the transparency and depth-peeling implementation contract:
[../../../spec/scene/implementation/TRANSPARENCY_MSAA.md](../../../spec/scene/implementation/TRANSPARENCY_MSAA.md).

Public alpha-mode semantics live in
[../../../spec/scene/semantics/TRANSPARENCY.md](../../../spec/scene/semantics/TRANSPARENCY.md).

This file tracks remaining execution order, validation, and example guidance.


## Current Situation

`DVZ_ALPHA_DEPTH_PEEL` currently expands the scene into a depth-peeling-shaped graph:

1. an opaque pass;
2. a front-facing transparent pass;
3. a back-facing transparent pass;
4. a composite pass.

That path exercises graph-backed multi-pass rendering, raster-state changes, intermediate render
targets, sampled resolve, resize handling, and borrowed app-frame execution. It is not yet full
dual depth peeling because it does not iteratively peel all transparent depth layers using previous
min/max depth bounds.

This limitation shows up on non-convex meshes. The IBL BWM brain shell can produce missing or
jagged-looking shell regions in `Depth peel` mode while `WBOIT` appears more stable.


## Scene API And Example Guidance

Keep `DVZ_ALPHA_DEPTH_PEEL` as the opt-in mode, but label the BWM example default as `WBOIT` until
dual depth peeling is corrected. The BWM GUI should keep the technique selector so users can compare
`WBOIT`, `Depth peel`, and `Source-over`.

The `Shell depth-tests clusters` diagnostic checkbox should remain useful:

1. on: transparent shell respects opaque cluster depth;
2. off: shell overlays/tints clusters regardless of cluster depth, useful for judging shell
   coverage independently from point-sprite occlusion.


## Validation Plan

Add tests in layers:

1. FramePlan graph tests:
   - expected resources for `N` peel iterations;
   - alternating ping/pong reads and writes;
   - composite reads the final accumulator resources.
2. DRP2 semantic tests:
   - multi-iteration graph lowers to legal command order;
   - sampled reads never use the texture being written in the same pass;
   - pipeline formats match attachment formats.
3. GPU smoke tests:
   - convex cube with front/back color cards;
   - two or three nested transparent shells;
   - non-convex fixture mesh with overlapping lobes;
   - resize smoke to catch stale sampled descriptors.
4. Real-data smoke:
   - `ibl_bwm_brain_glfw` in bounded or timeout-driven mode;
   - compare WBOIT and depth peel visually with shell depth-test on and off.


## Recommended Sequence

1. Add graph and lowering tests for a fixed two-iteration dual-depth-peel graph without changing
   the existing public mode.
2. Implement DRP2/vklite execution of the fixed graph with minimal unlit shaders.
3. Replace the scene `DVZ_ALPHA_DEPTH_PEEL` graph expansion with the fixed-iteration dual path.
4. Add lit mesh shader variants and use the existing material uniform path.
5. Re-run `hello_mesh_depth_peel_glfw`, `ibl_bwm_brain_glfw`, and scene alpha-mode GPU tests.
6. Add a retained quality descriptor only after the fixed path is correct and stable.
