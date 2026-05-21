# Dual Depth Peeling Implementation

> **Execution Status**
> - **Status:** `DONE`
> - **Completed on:** `2026-05-21`
> - **Purpose:** record the fixed-iteration dual depth peeling upgrade for the active
>   scene -> FramePlan graph -> DRP2 -> vklite path.


## Summary

`DVZ_ALPHA_DEPTH_PEEL` now uses the fixed internal graph-backed dual depth peeling contract:

1. `peel.init` reduces all transparent fragments into the first nearest/farthest bounds;
2. each `peel.iter.N` samples the previous bounds, peels the current nearest and farthest layers,
   accumulates front/back colors, and writes the next bounds;
3. front accumulation uses front-to-back premultiplied blending;
4. back accumulation uses back-to-front source-over blending;
5. depth bounds are encoded as `(-near, far)` and reduced with max blending in the RG channels;
6. `peel.composite` samples the front/back accumulators and blends the result onto `rt`.

The implementation keeps the existing four per-panel resources:

1. `<panel>.peel.depth_minmax_ping`;
2. `<panel>.peel.depth_minmax_pong`;
3. `<panel>.peel.front_accum`;
4. `<panel>.peel.back_accum`.

No new public alpha mode, retained quality descriptor, render-pass role, or bind-group contract was
needed for this slice.


## Implementation Notes

The upgrade changed:

1. depth-peel GLSL shader semantics for init, iteration, and lit iteration variants;
2. depth-peel graph clear values for the max-reduced `(-near, far)` bounds attachment;
3. runtime blend state for front, back, and bounds targets;
4. render-contract blend/raster expectations and diagnostics;
5. FramePlan graph assertions for all ping/pong iterations;
6. scene DRP2 lowering assertions for bounds blend and no-cull raster state;
7. app/offscreen coverage with a three-layer transparent regression.

The depth-bound epsilon is intentionally `1e-3` in the iteration shaders because the bounds texture
uses `VK_FORMAT_R16G16B16A16_SFLOAT`.


## Validation

Passed:

```text
just build
just test test_frame_plan_graph_depth_peeling_shape
just test test_scene_visual_alpha_mode_depth_peel_frame_plan
just test test_scene_visual_alpha_mode_emits_depth_peel_drp2
just test test_scene_drp2_contract_checker_rejects_raster_drift
direnv exec . just test test_app_offscreen_depth_peel_mesh_two_layers
direnv exec . just test test_app_offscreen_depth_peel_mesh_three_layers
just test scene
just test drp2
direnv exec . ./build/examples/c/techniques/depth_peel 3
git diff --check
```

Attempted but blocked by missing local data:

```text
direnv exec . ./build/examples/c/showcase/ibl_brain 1
```

The IBL BWM smoke reported missing files under `build/local_data/ibl_bwm` and asked to run
`python examples/c/showcase/prepare_ibl_bwm.py`.


## Residual Follow-Ups

1. Run the IBL BWM real-data comparison once the local prepared arrays are available.
2. Add a retained quality descriptor only if users need to choose the fixed iteration count.
