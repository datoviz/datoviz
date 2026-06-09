# WebGPU Example Continuation Plan

Execution Status:

- Status: completed on 2026-06-09
- Updated on: 2026-06-09
- Scope: remaining `webgpu-planned` examples after the first live gallery promotion batches


## Goal

Continue promoting browser WebGPU live examples without creating a second browser-specific example
path. Every promoted route must use the same canonical C example or portable C scenario used by
native validation.


## Current Live Baseline

The live WebGPU gallery already covers:

1. `feature_basic_scene`
2. `feature_timer_animation`
3. `feature_triangulation_polygon`
4. `feature_builtin_shapes_2d`
5. `feature_builtin_shapes_3d`
6. `feature_isolines`
7. `feature_animation_tracks`
8. `feature_obj_loading`
9. `feature_picking`
10. `feature_selection_pixel`
11. `feature_selection_sphere`
12. `feature_selection_mesh_instances`
13. `feature_compute_buffer_animation`
14. `image_probe`


## Completed Plan

1. Add shader coverage for selection examples.

   Implement WGSL support for sphere picking and lit instanced primitive/mesh rendering. The known
   missing shader keys are:

   - `_vs_sphere_pickw_query_u32`
   - `_vs_prim_lit_instw`

   Add shader registry entries and narrow frame-plan/emitter validation for both paths before
   promoting examples.

2. Promote sphere selection.

   Re-enable `feature_selection_sphere` in the WASM scenario registry, add the live gallery route,
   and add a direct WASM smoke that creates the scenario, queues a pointer query, emits query
   packets, and verifies active readback.

3. Promote mesh instance selection.

   Make `feature_selection_mesh_instances` WASM-safe without creating a second web path, re-enable
   it in the WASM scenario registry, add the live gallery route, and add direct query-packet smoke.

4. Implement WebGPU scene compute.

   Add WebGPU runtime support for scene compute passes, storage buffers, compute bind groups,
   compute dispatch packets, and compute-to-render buffer reuse. Keep this work separate from
   gallery promotion commits.

5. Promote compute buffer animation.

   Add `feature_compute_buffer_animation` to the WASM scenario registry, add the live gallery route,
   and add smoke evidence for compute dispatch and visible point-buffer updates.

6. Update tracking docs.

   Regenerate the WebGPU matrix through `tools/build_gallery.py`. Update
   `docs/reference/webgpu-subset.md` when new supported features become official. Update
   `spec/scene/integration/WASM_WEBGPU_PARITY_PLAN.md` if the RC support boundary changes.


## Validation Loop

For each promotion:

1. Compile the native example target.
2. Run `python3 tools/build_gallery.py`.
3. Run `node --check` for touched JavaScript.
4. Run `just wasm-scene-smoke`.
5. Run `just webgpu-browser-smoke`.
6. Run `uv run --with mkdocs-material --with 'mkdocstrings[python]' mkdocs build`.
7. Run `git diff --check`.
8. Check `git status --short` and `git diff --cached --stat` before committing.


## Preferred Order

1. Sphere picking shader.
2. `feature_selection_sphere` promotion.
3. Lit instanced primitive/mesh shader.
4. `feature_selection_mesh_instances` promotion.
5. Scene compute runtime.
6. `feature_compute_buffer_animation` promotion.
