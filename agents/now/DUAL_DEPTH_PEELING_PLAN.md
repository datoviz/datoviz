# Dual Depth Peeling Plan

> **Execution Status**
> - **Status:** `PLANNING`
> - **Updated on:** `2026-05-16`
> - **Purpose:** turn the current scene depth-peeling path into a real dual-depth peeling
>   implementation suitable for complex non-convex transparent meshes such as the IBL BWM brain
>   shell.


## Current Situation

`DVZ_ALPHA_DEPTH_PEEL` currently expands the scene into a depth-peeling-shaped graph:

1. an opaque pass,
2. a front-facing transparent pass,
3. a back-facing transparent pass,
4. a composite pass.

That path is useful for exercising graph-backed multi-pass scene rendering, raster-state changes,
intermediate render targets, sampled resolve, resize handling, and borrowed app-frame execution.
It is not yet full dual depth peeling. The current front/back shaders write the first selected face
set into accumulation textures, but they do not iteratively peel all transparent depth layers using
previous min/max depth bounds.

This limitation shows up on non-convex meshes. The IBL BWM brain shell can produce missing or
jagged-looking shell regions in `Depth peel` mode while `WBOIT` appears more stable, because WBOIT
approximately accumulates all fragments that pass the opaque-depth test.


## Target Behavior

For a `DVZ_ALPHA_DEPTH_PEEL` visual, the native path should:

1. render opaque scene color and depth first;
2. initialize per-pixel nearest and farthest transparent depth bounds;
3. iteratively peel front and back transparent layers using ping-pong min/max depth textures;
4. accumulate peeled front layers front-to-back and back layers back-to-front;
5. composite the accumulated transparent result over the existing final target;
6. keep all rendering through scene -> FramePlan graph -> DRP2 -> vklite/canvas.

The common public API should remain declarative:

```c
dvz_visual_set_alpha_mode(mesh, DVZ_ALPHA_DEPTH_PEEL);
```

Technique controls such as peel count or quality may be added later, but users should not manage the
intermediate textures or graph passes directly.


## FramePlan Graph Shape

Use explicit graph resources per panel:

- `panel.peel.depth_minmax_ping`
- `panel.peel.depth_minmax_pong`
- `panel.peel.front_accum`
- `panel.peel.back_accum`
- optional per-iteration temporary front/back color targets if accumulation cannot happen in-place
- `panel.depth.opaque` when transparent visuals should depth-test against opaque visuals

Use explicit passes:

1. `opaque`: writes `rt` and optional opaque depth.
2. `peel.init`: initializes min/max transparent depth and clears accumulators.
3. `peel.iter.N`: reads previous min/max, peels the next front/back pair, writes next min/max, and
   accumulates colors.
4. `peel.composite`: samples front/back accumulators and writes `rt`.

The first implementation can use a fixed compile-time iteration count, for example 4 or 8. A later
slice can expose quality policy through a retained scene technique descriptor.


## DRP2 And Runtime Requirements

The current DRP2/vklite runtime already has most required primitives:

- graph-created intermediate textures,
- multiple color attachments,
- sampled texture bind groups,
- per-pipeline raster state,
- explicit color blending,
- graph attachment load/store handling.

Likely additions or hardening:

1. Ensure render-pass reads from previous ping textures and writes to pong textures are ordered and
   transitioned by graph access, not by technique-local assumptions.
2. Support the exact blend modes needed for front and back accumulation.
3. Validate pipeline color-target formats against graph attachment formats.
4. Add diagnostics that print peel iteration count, ping/pong resource ids, and sampled dependency
   ids in `DVZ_DRP2_TRACE=full`.


## Shader Work

Replace the current `depth_peel_front*` and `depth_peel_back*` behavior with real peel shaders:

1. Init shader:
   - render transparent geometry;
   - write nearest and farthest depths into a min/max texture;
   - optionally accumulate the first front/back layers.

2. Iteration shader:
   - sample previous min/max depth;
   - discard fragments outside the unpeeled interval;
   - update next nearest/farthest depth;
   - output front/back color contributions only for the peeled layers.

3. Composite shader:
   - combine front and back premultiplied accumulators;
   - source-over composite the result onto `rt`.

Depth encoding must use a format and comparison convention that is stable under Vulkan clip depth.
Start with normalized `gl_FragCoord.z`, document the convention, and add small shader tests that
make near/far ordering obvious.


## Scene API And Example Guidance

Keep `DVZ_ALPHA_DEPTH_PEEL` as the opt-in mode, but label the BWM example default as `WBOIT` until
dual depth peeling is corrected. The BWM GUI should keep the technique selector so users can compare
`WBOIT`, `Depth peel`, and `Source-over`.

The `Shell depth-tests clusters` diagnostic checkbox added to the BWM example should remain useful:

- on: transparent shell respects opaque cluster depth;
- off: shell overlays/tints clusters regardless of cluster depth, useful for judging shell
  coverage independently from point-sprite occlusion.


## Validation Plan

Add tests in layers:

1. FramePlan graph tests:
   - expected resources for N peel iterations;
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
   - compare WBOIT and depth peel visually with the shell depth-test checkbox on and off.


## Recommended Sequence

1. Add graph and lowering tests for a fixed two-iteration dual-depth-peel graph without changing the
   existing public mode.
2. Implement DRP2/vklite execution of the fixed graph with minimal unlit shaders.
3. Replace the scene `DVZ_ALPHA_DEPTH_PEEL` graph expansion with the fixed-iteration dual path.
4. Add lit mesh shader variants and use the existing material uniform path.
5. Re-run `hello_mesh_depth_peel_glfw`, `ibl_bwm_brain_glfw`, and scene alpha-mode GPU tests.
6. Add a retained quality descriptor only after the fixed path is correct and stable.
