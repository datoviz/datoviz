# WebGPU Example Continuation Plan

Execution Status:

- Status: completed first continuation batch; superseded by `agents/now/STATUS.md`
- Updated on: 2026-06-10
- Scope: remaining `webgpu-planned` examples after the first live gallery promotion batches


## Goal

Continue promoting browser WebGPU live examples without creating a second browser-specific example
path. Every promoted route must use the same canonical C example or portable C scenario used by
native validation.


## Current Live Baseline

The live WebGPU gallery now covers:

1. `features_basic_scene`
2. `features_timer_animation`
3. `features_builtin_shapes_2d`
4. `features_builtin_shapes_3d`
5. `features_isolines`
7. `features_animation_tracks`
8. `features_obj_loading`
9. `features_picking`
10. `features_selection_pixel`
11. `features_selection_sphere`
12. `features_selection_mesh_instances`
13. `features_compute_buffer_animation`
14. `features_image_probe`
15. `features_colorbar`
16. `features_scalebar`
17. `features_scalebar_units`
18. `features_legend_categorical`
19. `features_annotation_readout`
20. `showcases_linked_probe_colorbar`
21. `showcases_scientific_plotting`
22. `visuals_vector`
23. `showcases_wind_field`

The next active handoff is not in this completed file. Use `agents/now/STATUS.md`: promote
`showcases_gpu_particle_smoke` as the browser compute particle proof, then work through planned
example clusters that reuse already-current primitives.


## Completed Plan

1. Add shader coverage for selection examples.

   Implement WGSL support for sphere picking and lit instanced primitive/mesh rendering. The known
   missing shader keys are:

   - `_vs_sphere_pickw_query_u32`
   - `_vs_prim_lit_instw`

   Add shader registry entries and narrow frame-plan/emitter validation for both paths before
   promoting examples.

2. Promote sphere selection.

   Re-enable `features_selection_sphere` in the WASM scenario registry, add the live gallery route,
   and add a direct WASM smoke that creates the scenario, queues a pointer query, emits query
   packets, and verifies active readback.

3. Promote mesh instance selection.

   Make `features_selection_mesh_instances` WASM-safe without creating a second web path, re-enable
   it in the WASM scenario registry, add the live gallery route, and add direct query-packet smoke.

4. Implement WebGPU scene compute.

   Add WebGPU runtime support for scene compute passes, storage buffers, compute bind groups,
   compute dispatch packets, and compute-to-render buffer reuse. Keep this work separate from
   gallery promotion commits.

5. Promote compute buffer animation.

   Add `features_compute_buffer_animation` to the WASM scenario registry, add the live gallery route,
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
2. `features_selection_sphere` promotion.
3. Lit instanced primitive/mesh shader.
4. `features_selection_mesh_instances` promotion.
5. Scene compute runtime.
6. `features_compute_buffer_animation` promotion.
