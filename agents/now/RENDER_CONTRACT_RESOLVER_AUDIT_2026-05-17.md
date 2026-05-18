# Render Contract Resolver Audit

> **Execution Status**
> - **Status:** `AUDIT REPORT`
> - **Updated on:** `2026-05-18`
> - **Scope:** `spec/scene/proposals/RENDER_CONTRACT_RESOLVER.md` and the active scene ->
>   DRP2 -> vklite code path.
> - **Inputs:** local code review plus three parallel subagent audits covering contract semantics,
>   implementation consistency, and automated test coverage.

This report audits whether the proposed render-contract resolver is robust enough to become the
authority for the difficult transparency, depth, volume, occlusion, MSAA, EDL, and SSAO cases.


## Implementation Progress Log

### 2026-05-17: Phase 0 / FramePlan node-index safety

Completed the first immediate safety slice from the improvement plan.

Changes:

1. Replaced long-lived render-node pointers in `src/scene/scene_emit.c` with node indices for panel
   render emission.
2. Added a local mutable-node lookup helper and reacquires `DvzFramePlanNode*` immediately before
   appending visual metadata.
3. Added `test_scene_frame_plan_node_reallocation_safe`, which pre-fills a FramePlan to one slot
   before `DVZ_FRAME_PLAN_INITIAL_NODE_CAPACITY`, then emits a G-buffer panel with two mesh visuals
   to force node-array growth while preserving visual assignments.

Validation:

1. `cmake --build build --target dvztest_scene -j2`
2. `./build/testing/dvztest_scene test_scene_frame_plan_node_reallocation_safe`
3. `./build/testing/dvztest_scene test_scene_gbuffer_runtime_lowering`
4. `git diff --check -- src/scene/scene_emit.c src/scene/tests/scene_graph.c src/scene/tests/test_scene.h`


### 2026-05-17: Phase 0 / Graph failure diagnostics

Completed the graph-emission hardening slice.

Changes:

1. `_scene_emit_panel_render()` now returns a success flag for panel graph emission instead of only
   logging technique-builder failures.
2. `dvz_figure_emit_ex()` now normalizes diagnostics before FramePlan construction, reports graph
   emission failure, propagates contract-validation diagnostics, and returns `NULL` instead of
   asserting after a failed contract check.
3. `_scene_frame_plan_contracts_validate()` now reports graph-backed render roles that do not have
   a matching FramePlan graph pass.
4. Added `test_scene_frame_plan_missing_graph_pass_fails_contract` to lock the missing-pass
   diagnostic behavior.

Validation:

1. `cmake --build build --target dvztest_scene -j2`
2. `./build/testing/dvztest_scene test_scene_frame_plan_missing_graph_pass_fails_contract`
3. `./build/testing/dvztest_scene test_scene_render_contract_validation_errors`
4. `./build/testing/dvztest_scene test_scene_visual_alpha_mode_splits_frame_plan_passes`
5. `./build/testing/dvztest_scene test_scene_gbuffer_runtime_lowering`
6. `git diff --check -- src/scene/_scene_emit.h src/scene/scene.c src/scene/scene_emit.c src/scene/render_contract.c src/scene/tests/scene_graph.c src/scene/tests/test_scene.h`


### 2026-05-17: Phase 0 / Mixed OIT rejection

Completed the immediate mixed-transparency policy slice.

Changes:

1. Added a pre-FramePlan validation step in `dvz_figure_emit_ex()` that rejects a panel containing
   both WBOIT and depth-peeling visuals.
2. The diagnostic names the emitted panel id and the first conflicting visual indices, making the
   rejection explicit instead of falling through to missing graph-pass validation.
3. Added `test_scene_visual_alpha_mode_mixed_oit_rejected`.

Validation:

1. `cmake --build build --target dvztest_scene -j2`
2. `./build/testing/dvztest_scene test_scene_visual_alpha_mode_mixed_oit_rejected`
3. `./build/testing/dvztest_scene test_scene_visual_alpha_mode_depth_peel_frame_plan`
4. `./build/testing/dvztest_scene test_scene_visual_alpha_mode_splits_frame_plan_passes`
5. `./build/testing/dvztest_scene test_scene_visual_alpha_mode_emits_depth_peel_drp2`
6. `./build/testing/dvztest_scene test_scene_visual_alpha_mode_emits_wboit_drp2`
7. `git diff --check -- src/scene/scene.c src/scene/tests/scene_graph.c src/scene/tests/test_scene.h`


### 2026-05-17: Phase 0 / Sampled-depth contract split

Completed the first sampled-depth semantic split.

Changes:

1. Contract validation now separates fixed-function depth attachment needs from shader sampled-depth
   needs; `draw->samples_depth` no longer satisfies or creates the generic depth-attachment
   requirement by itself.
2. Pass contracts now count sampled depth reads and also treat a depth attachment loaded with read
   access as a producer-backed depth resource.
3. Source-over graph emission now creates an explicit producer depth attachment in the opaque graph
   pass when later blended draws need depth but the opaque visuals themselves would not otherwise
   write depth.
4. Added a regression in `test_scene_render_contract_validation_errors` showing that a same-pass
   clear/write depth attachment is not enough proof for sampled-depth semantics.

Validation:

1. `cmake --build build --target dvztest_scene -j2`
2. `./build/testing/dvztest_scene test_scene_render_contract_validation_errors`
3. `./build/testing/dvztest_scene test_scene_blended_mesh_orders_after_volume_slice`
4. `./build/testing/dvztest_scene test_scene_blended_mesh_occlusion_contracts`
5. `./build/testing/dvztest_scene test_scene_visual_alpha_mode_standard_blend`
6. `./build/testing/dvztest_scene test_scene_visual_alpha_mode_splits_frame_plan_passes`
7. `git diff --check -- src/scene/render_contract.h src/scene/render_contract.c src/scene/scene_emit.c src/scene/tests/scene_graph.c`


### 2026-05-17: Phase 0 / Checked runtime key appends

Completed the pipeline-key truncation hardening slice.

Changes:

1. Added checked runtime key append helpers in `src/scene/frame_plan_runtime.c`.
2. Replaced suffix appends for source-over blend, segment coverage blend, disabled depth test,
   depth peel, fixed-controller variants, depth/zwrite variants, MSAA, alpha-to-coverage, and scene
   occlusion.
3. Key suffix truncation now reports a diagnostic and aborts emission instead of silently aliasing a
   pipeline or shader key.

Validation:

1. `cmake --build build --target dvztest_scene -j2`
2. `./build/testing/dvztest_scene test_scene_visual_alpha_mode_standard_blend`
3. `./build/testing/dvztest_scene test_scene_visual_alpha_mode_emits_depth_peel_drp2`
4. `./build/testing/dvztest_scene test_scene_msaa_runtime_lowering`
5. `./build/testing/dvztest_scene test_scene_visual_scene_occlusion_emits_drp2`
6. `./build/testing/dvztest_scene test_scene_sphere_ssao_glsl_executes`
7. `git diff --check -- src/scene/frame_plan_runtime.c`


### 2026-05-17: Phase 0 / DRP2 attachment format classes

Completed the DRP2 format-class validation slice.

Changes:

1. Added an explicit DRP2 depth-format classifier for currently supported depth attachments.
2. Semantic validation now rejects depth formats used as render-pipeline color targets before a
   pipeline object is registered.
3. Render-pass validation now rejects depth textures used as color or resolve attachments, and
   rejects non-depth textures used as named depth attachments.
4. Added focused DRP2 regressions for invalid pipeline color targets, invalid color attachments,
   and invalid named depth attachments.

Validation:

1. `cmake --build build --target dvztest_drp2 -j2`
2. `./build/testing/dvztest_drp2 test_drp2_render_pipeline_rejects_depth_color_target`
3. `./build/testing/dvztest_drp2 test_drp2_render_pass_rejects_attachment_format_classes`
4. `./build/testing/dvztest_drp2 test_drp2_render_pipeline_attachment_validation`
5. `./build/testing/dvztest_drp2 test_drp2_begin_render_pass_named_depth_validation`
6. `./build/testing/dvztest_drp2 test_drp2_wboit_accumulation_resolve_stream`
7. `git diff --check -- src/drp2/semantic.c src/drp2/tests/test_drp2.c src/drp2/tests/test_drp2.h agents/now/RENDER_CONTRACT_RESOLVER_AUDIT_2026-05-17.md`


### 2026-05-17: Phase 1 / Draw-contract resolver matrix

Completed the first authoritative-contract extraction slice.

Changes:

1. Added `DvzSceneDrawFacts` as the explicit resolver input row for visual draw policy.
2. Added `_scene_draw_contract_resolve()`, which maps visual facts plus render-pass role into a
   `DvzSceneDrawContract`.
3. Rewired `_scene_draw_contract_from_visual()` to derive facts from visual pass capabilities and
   delegate policy decisions to the resolver helper.
4. Added `test_scene_draw_contract_resolver_matrix` to cover opaque, source-over volume, WBOIT,
   and scene-occluder contract rows without requiring FramePlan emission.

Validation:

1. `cmake --build build --target dvztest_scene -j2`
2. `./build/testing/dvztest_scene test_scene_draw_contract_resolver_matrix`
3. `./build/testing/dvztest_scene test_scene_visual_pass_capabilities`
4. `./build/testing/dvztest_scene test_scene_render_contract_validation_errors`
5. `./build/testing/dvztest_scene test_scene_blended_mesh_occlusion_contracts`
6. `git diff --check -- src/scene/render_contract.h src/scene/render_contract.c src/scene/tests/scene_graph.c src/scene/tests/test_scene.h agents/now/RENDER_CONTRACT_RESOLVER_AUDIT_2026-05-17.md`


### 2026-05-17: Phase 1 / Explicit draw policy fields

Completed the first draw-contract policy extension slice.

Changes:

1. Extended `DvzSceneDrawContract` with explicit depth-policy, blend-policy, shader-feature, and
   bind-group-layout masks while preserving the existing compatibility booleans.
2. Added internal helpers that resolve depth and blend policy from the facts matrix and pass role.
3. The resolver now populates shader-feature and bind-layout masks from sampled-depth, occlusion,
   material, image, volume, and scene-occlusion requirements.
4. Expanded `test_scene_draw_contract_resolver_matrix` to assert the new policy fields for opaque,
   source-over volume, WBOIT, and scene-occluder rows.

Validation:

1. `cmake --build build --target dvztest_scene -j2`
2. `./build/testing/dvztest_scene test_scene_draw_contract_resolver_matrix`
3. `./build/testing/dvztest_scene test_scene_visual_pass_capabilities`
4. `./build/testing/dvztest_scene test_scene_render_contract_validation_errors`
5. `./build/testing/dvztest_scene test_scene_visual_alpha_mode_splits_frame_plan_passes`
6. `git diff --check -- src/scene/render_contract.h src/scene/render_contract.c src/scene/tests/scene_graph.c agents/now/RENDER_CONTRACT_RESOLVER_AUDIT_2026-05-17.md`


### 2026-05-17: Phase 1 / FramePlan contract metadata

Completed the first contract-correlation slice.

Changes:

1. Added pass-contract ids to FramePlan render nodes.
2. Added draw-contract ids and resolved draw policy masks to `DvzFramePlanVisualMeta`.
3. Scene emission now resolves each visual's draw contract before appending it to the render node,
   stores the policy snapshot in metadata, and avoids leaving a partially appended visual on
   contract-id failure.
4. Extended the source-over volume/slice/mesh fixture to assert pass ids and draw policy snapshots.

Validation:

1. `cmake --build build --target dvztest_scene -j2`
2. `./build/testing/dvztest_scene test_scene_blended_mesh_orders_after_volume_slice`
3. `./build/testing/dvztest_scene test_scene_blended_mesh_occlusion_contracts`
4. `./build/testing/dvztest_scene test_scene_draw_contract_resolver_matrix`
5. `./build/testing/dvztest_scene test_scene_render_contract_validation_errors`
6. `git diff --check -- src/scene/_frame_plan.h src/scene/scene_emit.c src/scene/tests/scene_graph.c agents/now/RENDER_CONTRACT_RESOLVER_AUDIT_2026-05-17.md`


### 2026-05-17: Phase 1 / Transparent depth decisions from contracts

Completed the first fallback-removal slice in scene emission.

Changes:

1. Added a local helper that interprets `DvzSceneDrawContract.depth_policy` as the source of truth
   for transparent depth needs.
2. Replaced transparent depth pre-scan fallback logic with resolved draw contracts for source-over,
   WBOIT, and depth-peel roles.
3. Replaced source-over split/grouping fallback logic with resolved draw contracts; contract
   resolution failure now skips the draw instead of falling back to visual caps.

Validation:

1. `cmake --build build --target dvztest_scene -j2`
2. `./build/testing/dvztest_scene test_scene_visual_alpha_mode_splits_frame_plan_passes`
3. `./build/testing/dvztest_scene test_scene_visual_alpha_mode_standard_blend`
4. `./build/testing/dvztest_scene test_scene_visual_alpha_mode_emits_wboit_drp2`
5. `./build/testing/dvztest_scene test_scene_visual_alpha_mode_depth_peel_frame_plan`
6. `./build/testing/dvztest_scene test_scene_blended_mesh_orders_after_volume_slice`
7. `git diff --check -- src/scene/scene_emit.c agents/now/RENDER_CONTRACT_RESOLVER_AUDIT_2026-05-17.md`


### 2026-05-17: Phase 1 / Capability fallback diagnostics

Completed the first capability-fallback diagnostic slice.

Changes:

1. Added a scene-level pre-emit diagnostic pass that scans FramePlan graph resources against the
   active capability snapshot.
2. Reported MSAA sample-count lowering before runtime DRP2 emission, matching the runtime's
   supported power-of-two lowering policy.
3. Updated `test_scene_msaa_runtime_capability_lowering` to require the diagnostic while still
   asserting that DRP2 textures and pipelines use the lowered sample count.

Validation:

1. `cmake --build build --target dvztest_scene -j2`
2. `./build/testing/dvztest_scene test_scene_msaa_runtime_capability_lowering`
3. `./build/testing/dvztest_scene test_scene_msaa_runtime_lowering`
4. `./build/testing/dvztest_scene test_scene_visual_scene_occlusion_emits_drp2`
5. `./build/testing/dvztest_scene test_scene_render_contract_validation_errors`
6. `git diff --check -- src/scene/scene.c src/scene/tests/scene_graph.c agents/now/RENDER_CONTRACT_RESOLVER_AUDIT_2026-05-17.md`


### 2026-05-17: Phase 2 / DRP2 contract checker

Completed the first scene-level post-emit DRP2 checker slice.

Changes:

1. Runtime FramePlan emission now labels emitted DRP2 render-pass ids with their FramePlan
   pass-contract ids.
2. Added `_scene_frame_plan_drp2_contracts_validate()`, which walks an emitted DRP2 stream,
   correlates labeled render passes to FramePlan render nodes, checks graph pass ordering,
   validates render-pass attachment shape, and compares emitted per-draw pipeline depth/blend
   policy against stored draw-contract metadata when the pipeline is present in the stream.
3. Runtime-mode emission now rejects streams that fail the scene DRP2 contract checker.
4. Added `test_scene_drp2_contract_checker_rejects_pipeline_drift`, which mutates an emitted WBOIT
   accumulation pipeline and verifies the checker catches the mismatch.

Validation:

1. `cmake --build build --target dvztest_scene -j2`
2. `./build/testing/dvztest_scene test_scene_drp2_contract_checker_rejects_pipeline_drift`
3. `./build/testing/dvztest_scene test_scene_visual_alpha_mode_emits_wboit_drp2`
4. `./build/testing/dvztest_scene test_scene_visual_alpha_mode_emits_depth_peel_drp2`
5. `./build/testing/dvztest_scene test_scene_blended_mesh_occlusion_contracts`
6. `./build/testing/dvztest_scene test_scene_ssao_runtime_lowering`
7. `./build/testing/dvztest_scene test_scene_msaa_runtime_lowering`
8. `git diff --check -- src/scene/render_contract.h src/scene/render_contract.c src/scene/frame_plan_runtime.c src/scene/tests/scene_graph.c src/scene/tests/test_scene.h agents/now/RENDER_CONTRACT_RESOLVER_AUDIT_2026-05-17.md`


### 2026-05-17: Phase 2 / DRP2 pipeline state expansion

Completed the next post-emit checker expansion slice.

Changes:

1. The scene DRP2 checker now resolves known emitted render-pass attachment formats and sample
   counts from the command stream.
2. Per-draw pipeline validation now checks color-target count/format, raster sample count, and
   required scene bind-group layouts against stored draw-contract metadata.
3. Draw-contract resolution now mirrors runtime producer-pass layout policy for non-sphere G-buffer
   and scene-occlusion depth passes, avoiding stale material/image layout requirements.
4. `test_scene_drp2_contract_checker_rejects_pipeline_drift` now mutates WBOIT blend, sample-count,
   and bind-group layout state and verifies the checker rejects each drift.

Validation:

1. `cmake --build build --target dvztest_scene -j2`
2. `./build/testing/dvztest_scene test_scene_drp2_contract_checker_rejects_pipeline_drift`
3. `./build/testing/dvztest_scene test_scene_visual_alpha_mode_emits_wboit_drp2`
4. `./build/testing/dvztest_scene test_scene_visual_alpha_mode_emits_depth_peel_drp2`
5. `./build/testing/dvztest_scene test_scene_blended_mesh_occlusion_contracts`
6. `./build/testing/dvztest_scene test_scene_msaa_runtime_lowering`
7. `./build/testing/dvztest_scene test_scene_ssao_runtime_lowering`
8. `./build/testing/dvztest_scene test_scene_alpha_mode_toggle_refreshes_drp2_contracts`
9. `git diff --check -- src/scene/render_contract.c src/scene/tests/scene_graph.c`


### 2026-05-17: Phase 2 / DRP2 sampled-read binding checks

Completed the sampled-resource checker slice.

Changes:

1. The scene DRP2 checker now inspects emitted bind groups when their creation commands are present
   in the current stream.
2. Active graph sampled reads are matched against sampled texture entries by DRP2 object label,
   including scoped runtime resource labels.
3. The checker rejects observed sampled bind groups that fail to cover the active graph pass's
   sampled-read resources while still allowing persistent bind groups created by earlier streams.
4. `test_scene_drp2_contract_checker_rejects_pipeline_drift` now mutates the WBOIT resolve sampled
   bind group so both sampled entries reference the same texture and verifies the checker rejects it.

Validation:

1. `cmake --build build --target dvztest_scene -j2`
2. `./build/testing/dvztest_scene test_scene_drp2_contract_checker_rejects_pipeline_drift`
3. `./build/testing/dvztest_scene test_scene_visual_alpha_mode_emits_wboit_drp2`
4. `./build/testing/dvztest_scene test_scene_visual_alpha_mode_emits_depth_peel_drp2`
5. `./build/testing/dvztest_scene test_scene_blended_mesh_occlusion_contracts`
6. `./build/testing/dvztest_scene test_scene_ssao_runtime_lowering`
7. `./build/testing/dvztest_scene test_scene_alpha_mode_toggle_refreshes_drp2_contracts`


### 2026-05-17: Phase 3 / Central render-role semantics

Completed the first technique-centralization slice.

Changes:

1. Moved render-pass-role to graph-work-label mapping into `_scene_render_role_work_label()`.
2. Moved graph-backed role classification into `_scene_render_role_requires_graph_pass()`.
3. Replaced duplicate role-label helpers in runtime and render-contract validation with the
   centralized technique helpers.
4. Reused the existing centralized alpha-mode helpers in runtime pipeline-key and depth-peel
   decisions.
5. Added `test_scene_role_work_label_mapping_complete` to lock all active render-pass roles to one
   label and graph-required classification.

Validation:

1. `cmake --build build --target dvztest_scene -j2`
2. `./build/testing/dvztest_scene test_scene_role_work_label_mapping_complete`
3. `./build/testing/dvztest_scene test_scene_drp2_contract_checker_rejects_pipeline_drift`
4. `./build/testing/dvztest_scene test_scene_visual_alpha_mode_emits_wboit_drp2`
5. `./build/testing/dvztest_scene test_scene_ssao_runtime_lowering`
6. `git diff --check -- src/scene/_technique.h src/scene/technique.c src/scene/render_contract.c src/scene/frame_plan_runtime.c src/scene/tests/scene_graph.c src/scene/tests/test_scene.h agents/now/RENDER_CONTRACT_RESOLVER_AUDIT_2026-05-17.md`


### 2026-05-17: Phase 4 / Alpha-mode semantic churn

Completed the first semantic runtime toggle slice.

Changes:

1. Added `test_scene_alpha_mode_toggle_refreshes_drp2_contracts`.
2. The test emits and semantically executes a retained primitive scene as source-over, WBOIT, then
   source-over again, destroying each live stream before mutating the retained visual.
3. It asserts the WBOIT accumulation pass appears only for the WBOIT frame and disappears again
   after toggling back to source-over, while the semantic runtime accepts all three emitted streams
   with the post-emit contract checker active.

Validation:

1. `cmake --build build --target dvztest_scene -j2`
2. `./build/testing/dvztest_scene test_scene_alpha_mode_toggle_refreshes_drp2_contracts`
3. `./build/testing/dvztest_scene test_scene_visual_alpha_mode_emits_wboit_drp2`
4. `./build/testing/dvztest_scene test_scene_drp2_contract_checker_rejects_pipeline_drift`
5. `git diff --check -- src/scene/tests/scene_graph.c src/scene/tests/test_scene.h agents/now/RENDER_CONTRACT_RESOLVER_AUDIT_2026-05-17.md`


### 2026-05-17: Phase 4 / WBOIT offscreen order check

Completed the first targeted offscreen visual correctness slice.

Changes:

1. Added `test_app_offscreen_wboit_mesh_order_independent_layers`.
2. Added a small offscreen helper that renders two overlapping WBOIT primitive layers in forward
   and reversed visual order and captures the center pixel.
3. The test compares captured RGB values with a tight tolerance and verifies the blended region is
   nonblank, skipping cleanly when the app/vklite context is unavailable.

Validation:

1. `cmake --build build --target dvztest_scene -j2`
2. `./build/testing/dvztest_scene test_app_offscreen_wboit_mesh_order_independent_layers`
3. `./build/testing/dvztest_scene test_app_offscreen_point_depth_orders_overlap`
4. `./build/testing/dvztest_scene test_scene_alpha_mode_toggle_refreshes_drp2_contracts`
5. `git diff --check -- src/scene/tests/app.c src/scene/tests/test_scene.h agents/now/RENDER_CONTRACT_RESOLVER_AUDIT_2026-05-17.md`


### 2026-05-17: Phase 4 / EDL enabled-pixel check

Completed the targeted EDL offscreen visual correctness slice.

Changes:

1. Added `test_app_offscreen_points_edl_changes_pixels`.
2. Added a deterministic point-scene capture helper that renders the same fixture with EDL disabled
   and enabled while capturing with EDL still active.
3. The test compares captured pixels, requires changed and darkened samples, and asserts the EDL
   frame has lower total RGB luminance than the disabled baseline.

Validation:

1. `cmake --build build --target dvztest_scene -j2`
2. `direnv exec . ./build/testing/dvztest_scene test_app_offscreen_points_edl_changes_pixels`
3. `git diff --check -- src/scene/tests/app.c src/scene/tests/test_scene.h agents/now/RENDER_CONTRACT_RESOLVER_AUDIT_2026-05-17.md`


### 2026-05-17: Phase 4 / Sphere SSAO contact check

Completed the targeted sphere/mesh SSAO offscreen visual correctness slice.

Changes:

1. Added `test_app_offscreen_sphere_ssao_darkens_contact`.
2. The test renders a sphere-impostor cluster over a normal-producing mesh quad, captures a disabled
   SSAO baseline, then enables SSAO and captures the same retained scene.
3. The assertion checks for changed pixels, darkened pixels, and lower total RGB luminance with SSAO
   enabled.

Validation:

1. `cmake --build build --target dvztest_scene -j2`
2. `direnv exec . ./build/testing/dvztest_scene test_app_offscreen_sphere_ssao_darkens_contact`
3. `direnv exec . ./build/testing/dvztest_scene test_app_offscreen_mesh_ssao_changes_pixels`
4. `git diff --check -- src/scene/tests/app.c src/scene/tests/test_scene.h agents/now/RENDER_CONTRACT_RESOLVER_AUDIT_2026-05-17.md`


### 2026-05-17: Phase 4 / Source-over scene-occlusion matrix

Completed the narrow source-over scene-occlusion matrix slice.

Changes:

1. Refactored the scene-occlusion offscreen capture helper so it can render with scene occlusion
   disabled or enabled.
2. Added `test_app_offscreen_source_over_scene_occlusion_matrix`.
3. The test locks the current source-over policy: hidden scene occluders do not attenuate the
   occluded visual, and enabling scene occlusion with the same visible source-over occluder does not
   double-attenuate beyond ordinary source-over composition.

Validation:

1. `cmake --build build --target dvztest_scene -j2`
2. `direnv exec . ./build/testing/dvztest_scene test_app_offscreen_source_over_scene_occlusion_matrix`
3. `direnv exec . ./build/testing/dvztest_scene test_app_offscreen_scene_occlusion_hidden_alpha`
4. `git diff --check -- src/scene/tests/app.c src/scene/tests/test_scene.h agents/now/RENDER_CONTRACT_RESOLVER_AUDIT_2026-05-17.md`


### 2026-05-17: Phase 4 / Depth-peel region readback

Completed the depth-peel offscreen readback hardening slice.

Changes:

1. Added a small RGB channel region-sum helper for app offscreen readback tests.
2. Replaced single-pixel assertions in `test_app_offscreen_depth_peel_mesh_two_layers` with 8x8
   region-channel checks for the red transparent layer, blue transparent layer, and opaque occluder.
3. The test still verifies that both transparent colors contribute where expected and that the
   opaque occluder wins in its region.

Validation:

1. `cmake --build build --target dvztest_scene -j2`
2. `direnv exec . ./build/testing/dvztest_scene test_app_offscreen_depth_peel_mesh_two_layers`
3. `direnv exec . ./build/testing/dvztest_scene test_app_offscreen_source_over_mesh_depth_and_blend`
4. `git diff --check -- src/scene/tests/app.c agents/now/RENDER_CONTRACT_RESOLVER_AUDIT_2026-05-17.md`


### 2026-05-18: Phase 4 / Volume-slice scene-occlusion dimming

Completed the targeted volume-slice scene-occlusion regression slice after the user reported
`test_app_offscreen_volume_slice_scene_occlusion_dimming` as the only `just tests` failure.

Changes:

1. Removed the redundant `sceneOcclusion.params.w` enable gate from the volume-slice
   scene-occlusion shader path. The scene-occlusion shader variant is already selected only for
   scene-occluded draws, so dimming now depends on the sampled occlusion depth, hidden alpha, and
   soft-edge parameters.
2. Invalidated cached set 1 and set 2 bind-group ids when runtime multi-draw emission changes
   pipelines, matching the set 0 cache behavior and avoiding stale bind reuse across incompatible
   pipeline layouts.
3. Replaced the failing app test's final bare luminance assertion with a diagnostic failure block
   that reports disabled, enabled, and threshold sums.

Validation:

1. `cmake --build build --target dvztest_scene -j2`
2. `./build/testing/dvztest_scene test_app_offscreen_volume_slice_scene_occlusion_dimming`
3. `./build/testing/dvztest_scene test_app_offscreen_volume_occlusion_region_delta`
4. `./build/testing/dvztest_scene test_app_offscreen_volume_depth_occluded_by_primitive`
5. `./build/testing/dvztest_scene test_app_offscreen_source_over_scene_occlusion_matrix`
6. `./build/testing/dvztest_scene test_scene_volume_slice_uses_generic_scene_occlusion`
7. `cmake --build build --target dvztest -j2`
8. `direnv exec . just test test_app_offscreen_volume_slice_scene_occlusion_dimming`
9. `direnv exec . just test test_scene_volume_slice_uses_generic_scene_occlusion`
10. `git diff --check`


### 2026-05-18: Phase 4 / Retained scene-occlusion toggle

Completed the brain-showcase regression slice for clicking `Show atlas mesh` with volume and scene
occlusion active.

Changes:

1. Added `test_app_offscreen_volume_slice_mesh_scene_occlusion_toggle`, a minimal retained
   offscreen app fixture that renders volume occlusion first, then toggles a hidden mesh visible as
   a scene occluder while the volume slice samples scene occlusion.
2. Confirmed the new test failed before the fix with `DRP2 sampled bind group misses graph read
   resource` and `emitted runtime DRP2 stream failed scene contract validation`, matching the
   original showcase failure mode.
3. Updated the post-emit DRP2 contract checker so `SetBindGroup` commands that reference persistent
   bind groups created by earlier retained-frame streams can still satisfy graph sampled-read
   coverage by using bind-group label dependency ids.

Validation:

1. `cmake --build build --target dvztest_scene -j2`
2. `direnv exec . ./build/testing/dvztest_scene test_app_offscreen_volume_slice_mesh_scene_occlusion_toggle`
3. `direnv exec . ./build/testing/dvztest_scene test_app_offscreen_volume_slice_scene_occlusion_dimming`
4. `direnv exec . ./build/testing/dvztest_scene test_scene_volume_slice_uses_generic_scene_occlusion`
5. `./build/testing/dvztest_scene test_scene_blended_mesh_occlusion_contracts`
6. `./build/testing/dvztest_scene test_scene_hidden_wboit_mesh_scene_occlusion_two_frames_glsl_executes`
7. `cmake --build build --target dvztest -j2`
8. `direnv exec . just test test_app_offscreen_volume_slice_mesh_scene_occlusion_toggle`
9. `git diff --check`
10. User-reported: the original `./build/examples/c/showcase/brain` GUI repro, focused scene/app
    filters, `just test scene`, and the full `just tests` run passed after the fix.


### 2026-05-18: Phase 4 / DRP2 attachment target validation

Completed the DRP2 semantic hardening slice for pipeline color targets versus render-pass
attachments.

Changes:

1. Render-pipeline validation now rejects unsupported non-depth pipeline color-target formats before
   the stream can reach backend pipeline creation.
2. Expanded `test_drp2_render_pipeline_rejects_depth_color_target` to cover an unsupported
   non-depth color-target format.
3. Expanded `test_drp2_render_pipeline_attachment_validation` to cover pipeline/render-pass
   color-target count mismatch, nonzero color-target format mismatch, and pass depth attachment
   without matching pipeline depth state.
4. Rechecked the WBOIT accumulation/resolve stream so inherited encoder render state remains valid
   when a new pass replaces the pipeline before drawing.

Validation:

1. `cmake --build build --target dvztest_drp2 -j2`
2. `./build/testing/dvztest_drp2 test_drp2_render_pipeline_rejects_depth_color_target`
3. `./build/testing/dvztest_drp2 test_drp2_render_pipeline_attachment_validation`
4. `./build/testing/dvztest_drp2 test_drp2_wboit_accumulation_resolve_stream`
5. `direnv exec . just test drp2`
6. `git diff --check`

Note: `clang-tidy -p build --quiet src/drp2/semantic.c --` is still blocked by the current compile
database/include setup because `semantic.c` cannot resolve `<volk.h>`.


### 2026-05-18: Phase 3 / Central pass-policy table

Completed the first pass-policy centralization slice for render-role semantics.

Changes:

1. Added `DvzSceneTechniquePassPolicy` and `_scene_technique_pass_policy()` in the scene technique
   layer as the single internal source for render-role work labels, graph-required status,
   source-over/WBOIT/depth-peel flags, fullscreen resolve flags, and sampled texture binding counts.
2. Rewired `_scene_render_role_work_label()` and `_scene_render_role_requires_graph_pass()` to read the
   centralized policy table instead of owning separate switch statements.
3. Rewired `_scene_pass_contract_from_render()` to derive passive pass-contract role flags from the
   centralized technique policy rather than re-inferring them locally.
4. Extended `test_scene_role_work_label_mapping_complete` to lock the policy table fields used by
   graph matching and passive contract construction.

Validation:

1. `cmake --build build --target dvztest_scene -j2`
2. `./build/testing/dvztest_scene test_scene_role_work_label_mapping_complete`
3. `./build/testing/dvztest_scene test_scene_drp2_contract_checker_rejects_pipeline_drift`
4. `./build/testing/dvztest_scene test_scene_visual_alpha_mode_emits_wboit_drp2`
5. `./build/testing/dvztest_scene test_scene_visual_alpha_mode_emits_depth_peel_drp2`
6. `direnv exec . just test scene` (`307/307`)


### 2026-05-18: Full-suite retained app regression test cleanup

The full `dvztest` runner initially reported later `stream`, `canvas`, `vk`, and `vklite` failures after
the retained offscreen volume-slice/mesh scene-occlusion toggle test had passed. The failing tests all
used expected-error capture; the new app regression test had installed a direct `log_set_intercept()`
hook and then cleared it with `log_set_intercept(NULL, NULL)`, removing the suite-level test runner
interceptor for subsequent tests.

Changes:

1. Replaced the app regression test's direct log interceptor with `tst_log_capture_begin()` /
   `tst_log_capture_end()`.
2. Added a small local helper that summarizes the suite-owned captured log records and checks for the
   previous scene-occlusion contract failure signatures without mutating global log interception state.

Validation:

1. `cmake --build build --target dvztest_scene -j2`
2. `direnv exec . ./build/testing/dvztest_scene test_app_offscreen_volume_slice_mesh_scene_occlusion_toggle`
3. `cmake --build build --target dvztest -j2`
4. `direnv exec . just test` (`609/609`)


### 2026-05-18: Phase 3 / Shared missing-graph-pass contract preflight

Completed the missing-graph-pass guardrail for both FramePlan and DRP2 contract validation.

Changes:

1. Added a shared render-contract preflight that checks every graph-backed render role has a matching
   graph pass.
2. Reused that preflight from `_scene_frame_plan_contracts_validate()` so the existing plan-level
   missing-pass diagnostic stays centralized.
3. Reused the same preflight from `_scene_frame_plan_drp2_contracts_validate()` so DRP2 contract
   validation cannot pass a broken plan before seeing stream commands.
4. Extended `test_scene_frame_plan_missing_graph_pass_fails_contract` to cover the DRP2 contract
   validator; this assertion failed before the shared preflight was added.
5. Stabilized `test_app_offscreen_records_dvzr_frames` by disabling the app DVZR recording FPS cap for
   that test's `record_start()` call; otherwise two immediate `render_once()` calls can legitimately
   collapse to one recorded scene frame under the developer recording cap.

Validation:

1. `cmake --build build --target dvztest_scene -j2`
2. `./build/testing/dvztest_scene test_scene_frame_plan_missing_graph_pass_fails_contract`
3. `direnv exec . ./build/testing/dvztest_scene test_app_offscreen_records_dvzr_frames`
4. `cmake --build build --target dvztest -j2`
5. `direnv exec . just test scene` (`307/307`)


## Executive Assessment

The direction is correct and worth continuing. The proposal identifies the right failure mode:
blend, depth, sampled resources, pass ordering, and technique state are currently easy to infer in
multiple places, which makes visually subtle regressions likely. The current implementation already
adds useful passive validation in `src/scene/render_contract.*`, and it has meaningful FramePlan and
DRP2-shape tests for source-over, WBOIT, depth peeling, scene occlusion, volume occlusion, MSAA, EDL,
and SSAO.

The contract layer is not yet strong enough to be treated as the source of truth. It mostly observes
an already-emitted FramePlan and graph, while `scene_emit.c`, `technique.c`,
`visual_pipeline.c`, and `frame_plan_runtime.c` still make independent decisions about pass shape,
pipeline state, blend equations, depth state, bind-group layouts, resource suffixes, and capability
fallbacks. That means the current contract can catch some graph-level mistakes, but it cannot yet
prove that the emitted DRP2 stream and runtime pipeline state match the semantic intent.

The highest-risk semantic gaps are:

1. Sampled-depth semantics are conflated with "has a depth attachment".
2. Mixed transparency composition across source-over, WBOIT, and depth peeling is underspecified.
3. Missing graph passes and graph-emission failures can be skipped after logging.
4. Cached `DvzFramePlanNode*` pointers can become stale when the FramePlan node array reallocates.
5. Offscreen readback tests mostly prove "something rendered", not correct ordering, occlusion, or
   compositing for combined cases.


## Audited Areas

Primary proposal:

1. [RENDER_CONTRACT_RESOLVER.md](../../spec/scene/proposals/RENDER_CONTRACT_RESOLVER.md)

Primary implementation paths:

1. [render_contract.h](../../src/scene/render_contract.h)
2. [render_contract.c](../../src/scene/render_contract.c)
3. [scene_emit.c](../../src/scene/scene_emit.c)
4. [technique.c](../../src/scene/technique.c)
5. [visual_pipeline.c](../../src/scene/visual_pipeline.c)
6. [frame_plan.c](../../src/scene/frame_plan.c)
7. [frame_plan_runtime.c](../../src/scene/frame_plan_runtime.c)
8. [semantic.c](../../src/drp2/semantic.c)

Primary test paths:

1. [scene_graph.c](../../src/scene/tests/scene_graph.c)
2. [app.c](../../src/scene/tests/app.c)
3. [test_drp2.c](../../src/drp2/tests/test_drp2.c)
4. [spec/drp2/fixtures/positive](../../spec/drp2/fixtures/positive)


## Current Strengths

The proposal's contract vocabulary is aligned with the real risks. It explicitly calls out ordinary
source-over, WBOIT, depth peeling, sampled occlusion resources, graph-backed passes, and lower-layer
assertions instead of lower-layer inference.

The current contract code already enforces important invariants:

1. Source-over draws must not write normal depth.
2. Depth-capable draws require a depth attachment.
3. Volume-occluded and scene-occluded draws require graph read edges.
4. WBOIT accumulation/resolve and depth-peeling passes have attachment-shape validation.
5. Scene emission validates contracts during `dvz_figure_emit_ex()`.

The tests are stronger than typical graphics plumbing tests at this stage. `scene_graph.c` has
FramePlan/contract coverage for source-over, WBOIT, depth peeling, volume occlusion, scene
occlusion, MSAA, EDL, and SSAO. DRP2 tests cover multi-attachment and runtime paths, and app tests
already exercise offscreen captures for several baseline scene cases.


## Priority Findings

### High: the contract is passive, not authoritative

`_scene_pass_contract_from_render()` derives a contract from an already-created render node and
matching graph pass. That is useful as an audit, but it is not yet the resolver described by the
proposal.

Evidence:

1. `DvzSceneDrawContract` currently stores broad booleans such as `depth_test`, `depth_write`,
   `samples_depth`, and `samples_scene_occlusion`, but not explicit blend policy, compare op,
   attachment lifecycle, shader features, bind-set placement, color target formats, or per-target
   blend equations.
2. `scene_emit.c` still decides pass splitting and which technique graph builders to call.
3. `visual_pipeline.c` still derives depth and pipeline state from visual pass caps.
4. `frame_plan_runtime.c` still mutates pipeline state for WBOIT, depth peel, G-buffer, occlusion,
   source-over, segment coverage, MSAA, alpha-to-coverage, and scene occlusion.

Impact: two layers can disagree and still pass current contract validation if the FramePlan graph
looks plausible. This is especially risky for cases where a wrong blend equation, depth write bit,
sample count, or bind-group layout produces a visually plausible but semantically wrong frame.

Recommendation: make the resolver produce a first-class `DvzSceneResolvedDraw` /
`DvzSceneResolvedPass` contract before FramePlan graph emission, attach stable contract ids to render
nodes, and validate the final DRP2 stream against those contract ids.


### High: sampled depth is conflated with depth attachment presence

The proposal requires sampled resources to have explicit producer/read edges. The current validation
treats `samples_depth` as satisfied by a depth attachment on the same pass:

```text
needs_depth = draw->depth_test || draw->samples_depth || draw->depth_write
```

That checks for a depth attachment, not for a sampled depth resource produced by an earlier pass.
This distinction matters because "the pass has a depth attachment for fixed-function testing" is not
the same contract as "the fragment shader samples a previous depth texture".

Impact: volume plus transparent mesh and volume raymarching cases can become ambiguous. A shader
that samples depth needs a concrete sampled resource id, layout, and producer edge; it should not be
validated only by the presence of a transient depth-test attachment.

Recommendation: split depth into at least these facts:

1. `depth_test_attachment_id`
2. `depth_write_attachment_id`
3. `sampled_depth_resource_id`
4. `depth_attachment_lifecycle`: clear/load/preserve/store
5. `depth_producer_semantics`: none, test-only transient, writes meaningful depth, sampled prior pass


### High: mixed transparency techniques are underspecified

Source-over, WBOIT, and depth peeling are each described, but the composition rules between them are
not explicit enough. Current `scene_emit.c` can create render nodes for more than one transparent
technique. Graph emission then prioritizes WBOIT with an `if`, depth peeling with an `else if`, and
source-over as another branch or auxiliary graph. If a panel contains WBOIT and depth-peel visuals,
the depth-peel render nodes can exist without corresponding graph passes, and current contract
validation skips render nodes that do not match a graph pass.

Impact: mixed OIT modes can silently fall into undefined behavior. Even if the current public API
does not encourage mixing, retained scenes can reach these combinations through per-visual alpha mode
changes.

Recommendation: choose one of two explicit semantics:

1. Reject panels that mix WBOIT and depth peeling, with a diagnostic that names the conflicting
   visuals and panel.
2. Define a total composition order such as opaque -> depth-producing prepasses -> WBOIT
   accumulation/resolve -> depth-peel composite -> source-over, then add contract and offscreen tests
   for that exact order.

The first option is safer and should be the immediate implementation until a visual use case requires
mixed OIT composition.


### High: graph-emission failure reporting remains coarse

Several graph-builder failures in `scene_emit.c` call `log_error()` and return a panel-level failure.
`dvz_figure_emit_ex()` now treats that panel failure as fatal, and missing graph-backed render passes
are reported by both FramePlan and DRP2 contract validation. The remaining weakness is diagnostic
precision: graph-builder failures still mostly surface as a generic "scene FramePlan graph emission
failed" report entry unless the contract validator can infer a more specific missing-pass problem.

Impact: runtime execution is now guarded against incomplete graph-backed render roles, but users and
tests may still need log capture rather than the diagnostic report to identify which graph builder
failed.

Recommendation:

1. Keep the shared missing-graph-pass preflight as a required invariant for all contract validation.
2. Thread graph-builder-specific diagnostic messages into the emit report so callers do not need to
   inspect logs to identify the failing technique.
3. Add targeted graph-builder failure tests when a deterministic non-OOM failure trigger exists.


### High: FramePlan render-node pointers can go stale

`frame_plan.c` stores nodes in a reallocating `plan->nodes` array, and `_append_node()` returns
pointers into that array. `scene_emit.c` caches `DvzFramePlanNode*` values such as `opaque_node`,
`gbuffer_node`, `transparent_node`, `depth_peel_*_node`, and `blended_nodes[]` across later appends.

Impact: once appending render/upload nodes crosses the current capacity, a reallocation can
invalidate cached node pointers. The contract resolver area is likely to add more generated nodes,
so this bug becomes more probable as the graph grows.

Recommendation:

1. Store node indices or stable ids, not `DvzFramePlanNode*`, across any call that can append.
2. Reacquire `&plan->nodes[index]` immediately before mutation.
3. Add a regression that lowers or exceeds `DVZ_FRAME_PLAN_INITIAL_NODE_CAPACITY` with many visuals
   and mixed techniques, then verifies all visuals land in the expected nodes.


### Medium: blend and pipeline policy are not contract-owned

The proposal already lists explicit blend/pipeline fields as an immediate next step. The current code
still encodes blend details in runtime emission:

1. Source-over uses `SRC_ALPHA, ONE_MINUS_SRC_ALPHA`.
2. Segment coverage reuses source-over-like blending.
3. WBOIT accumulation uses additive blending on RGBA16F and R16F targets.
4. Depth peeling uses three RGBA16F targets with role-specific raster state.
5. Occlusion and G-buffer passes set target formats in runtime.

Impact: a future change can alter pipeline blend state without changing the contract, and current
tests may only notice if a broad shape assertion or nonblank readback happens to fail.

Recommendation: add an explicit per-target blend contract:

```c
typedef struct DvzSceneBlendTargetContract
{
    uint32_t target_index;
    VkFormat format;
    bool blend_enabled;
    VkBlendFactor src_color;
    VkBlendFactor dst_color;
    VkBlendOp color_op;
    VkBlendFactor src_alpha;
    VkBlendFactor dst_alpha;
    VkBlendOp alpha_op;
    VkColorComponentFlags write_mask;
} DvzSceneBlendTargetContract;
```

Then compare emitted DRP2 `CreateRenderPipeline` and pipeline-mutator commands against it.


### Medium: occlusion resources are suffix heuristics, not exact draw contracts

Volume and scene occlusion validation currently searches for resource-id suffixes such as
`.volume_occlusion.depth` and `.scene_occlusion.depth`, while emission adds reads to broad visual
pass labels.

Impact: this works for one panel-local occlusion resource, but it will not generalize cleanly to
multiple occluders, layered occlusion maps, custom techniques, or cross-panel resources. It also does
not prove that the draw binds the same resource that the graph read edge declares.

Recommendation: store exact sampled resource ids, producer pass ids, expected bind set/binding, and
shader feature flags in the draw contract. Suffix checks can remain as temporary diagnostics, not as
the authoritative proof.


### Medium: graph ordering is insertion-order, not dependency-derived

The proposal says pass ordering should be derived from dependencies. Current runtime execution uses
stored graph-pass order when graph passes exist, and graph builders are responsible for emitting in a
valid order.

Impact: a new technique builder can append passes in visually wrong order while still using valid
resource names. This is especially easy to get wrong for volume occlusion, scene occlusion, WBOIT
resolve, depth-peel composite, SSAO, EDL, and MSAA resolve.

Recommendation: either document "graph builders must emit topological order" as a temporary
contract, or add a graph scheduler that sorts passes from declared reads/writes and reports cycles or
missing producers.


### Medium: MSAA sample count can drift after validation

Technique graph resources record a requested sample count and the passive contract sees that value.
Runtime later lowers the sample count based on device capability.

Impact: validation may approve a graph that is not exactly what runtime executes. Silent lowering is
sometimes desirable, but it should be represented in diagnostics and tests because it changes
pipeline state and attachments.

Recommendation: resolve sample count before contract validation, or explicitly record a
`requested_sample_count` and `resolved_sample_count` pair in the contract and diagnostics.


### Medium: pipeline cache keys can silently truncate

Runtime appends ABI-changing suffixes such as `_blend`, `_coverage_blend`, `_no_depth_test`,
`_peel_init`, `_peel_iter`, `_fixed`, `_depth`, `_msaaN`, `_a2c`, and `_scene_occ` into fixed-size
shader/pipeline key buffers without checking `dvz_snprintf()` truncation.

Impact: key aliasing can reuse a pipeline with the wrong depth, blend, MSAA, shader variant, or
occlusion layout. This is exactly the kind of visual regression the contract is supposed to prevent.

Recommendation: use checked append helpers or structured pipeline fingerprints and fail emission on
truncation.


### Medium: DRP2 semantic validation should reject attachment format-class mismatches

DRP2 semantic validation checks existence, usage, size, and sample count for render-pass
attachments. It does not appear to reject depth formats used as color attachments or non-depth
formats used as depth attachments before backend creation.

Impact: scene contract mistakes can reach vklite/Vulkan, where failures are later and harder to map
back to semantic intent.

Recommendation: add DRP2-level checks for color-vs-depth format classes and add fixture/runtime
tests for both invalid directions.


### Low: role, label, and alpha-mode predicates are duplicated

Role-to-work-label mapping and alpha-mode predicates exist in more than one scene file. Resource
suffix interpretation also appears in both contract and runtime paths.

Impact: low immediate risk, but high maintenance drag. Contract work will be less effective if the
same semantic mapping remains duplicated.

Recommendation: centralize render-role labels, alpha-mode class predicates, and graph resource-id
construction in scene-internal helpers, then test every `DvzFramePlanRenderPassRole`.


## Semantic Case Assessment

| Case | Current robustness | Main missing proof |
| --- | --- | --- |
| Opaque mesh with depth | Strongest path. Depth-capable rendering and graph-backed opaque passes are covered. | Contract-to-DRP2 pipeline validation and stale-node safety under large plans. |
| Source-over blended mesh | The no-normal-depth-write guardrail is good. | Exact source-over blend policy is not contract-owned; offscreen tests should assert expected mixed pixels and opaque occlusion. |
| WBOIT mesh | FramePlan, DRP2 shape, and runtime execution exist. | Order independence and interaction with source-over, volume, and occlusion are not strongly tested by pixels. |
| Depth-peel mesh | FramePlan, graph shape, and runtime execution exist. | Composition with other transparency modes is not specified; readback only proves nonblank output. |
| Volume plus mesh | Existing tests cover some ordering and contract shape. | Sampled-depth resource semantics are ambiguous; volume + WBOIT/depth-peel matrices need offscreen assertions. |
| Volume occlusion | Graph and contract coverage exist. | Exact sampled resource/bind proof and pixel tests that compare occlusion enabled/disabled regions. |
| Scene occlusion | Good FramePlan and DRP2 shape tests exist. | More semantic readback for hidden occluders, transparent occluders, and combined volume/scene occlusion. |
| MSAA | Graph shape and DRP2-level sample tests exist. | Scene/app readback should verify edge coverage changes and sample-count fallback diagnostics. |
| EDL | Graph-backed path exists. | App readback should compare EDL enabled vs disabled without disabling before capture. |
| SSAO | Mesh app readback is useful. | Sphere/mesh contact darkening and multi-technique interactions need pixel-region assertions. |


## Generalization Direction

The resolver should separate raw visual facts from resolved rendering decisions:

1. `DvzSceneVisualFacts`: visual type, attributes present, material/image/volume resources, alpha
   mode, depth flags, occlusion flags, controller mode, shader family, and capability needs.
2. `DvzSceneDrawContract`: resolved pass role, depth policy, blend policy, color target policy,
   bind-group policy, sampled resources, shader features, and pipeline features.
3. `DvzScenePassContract`: attachments, load/store/access, color/depth formats, sample count,
   draw contracts, pass dependencies, and clear/preserve semantics.
4. `DvzSceneTechniqueContract`: named multi-pass shape for opaque, source-over, WBOIT, depth peel,
   volume occlusion, scene occlusion, MSAA, EDL, SSAO, and G-buffer.
5. `DvzSceneResolvedRenderContract`: per-panel ordered technique composition with capability
   diagnostics and fallback/rejection decisions.

The key distinction is that "can a visual do X?" belongs in visual facts, while "this draw will do X
in this pass, using this attachment/resource/pipeline state" belongs in the resolved contract.


## Improvement Plan

### Phase 0: immediate safety hardening

1. Replace cached `DvzFramePlanNode*` values in scene emission with node indices or stable ids.
2. Make graph-emission failure fatal to FramePlan emission or recorded in diagnostics.
3. Make contract validation report missing graph passes for all graph-backed render roles.
4. Reject mixed WBOIT plus depth-peel panels until a total composition order is specified.
5. Split sampled-depth from depth-attachment semantics in the contract.
6. Add checked pipeline-key append helpers and fail on truncation.
7. Add DRP2 semantic validation for color-vs-depth attachment format classes.

### Phase 1: make contracts authoritative

1. Add a resolver-matrix helper that maps visual facts plus pass role into an explicit draw contract.
2. Extend draw contracts with depth policy, blend policy, target formats, sample count, raster state,
   shader features, pipeline features, and bind-group layout requirements.
3. Attach contract ids or resolved contract snapshots to FramePlan render nodes.
4. Remove fallback decisions in `scene_emit.c` that recompute caps after contract resolution.
5. Treat capability fallback, rejection, and sample-count lowering as contract diagnostics.

### Phase 2: validate emitted DRP2 against contracts

1. Add a scene-level post-emit checker that walks the DRP2 command stream and validates render-pass
   attachments, pipeline color targets, blend equations, depth state, multisampling, bind-group
   layouts, and sampled texture bindings against the resolved contracts.
2. Keep DRP2 semantic validation backend-agnostic, but add enough labels or debug ids for the scene
   checker to correlate pipelines/draws/passes to contract ids.
3. Assert that graph pass ordering is topological, or add a scheduler that executes declared
   dependencies instead of insertion order.

### Phase 3: centralize technique builders

1. Move WBOIT, depth-peel, source-over, volume occlusion, scene occlusion, G-buffer, EDL, SSAO, and
   MSAA pass-contract construction into named builders.
2. Have builders emit both graph passes and expected runtime policy from the same data.
3. Centralize role labels, resource-id construction, and alpha-mode class predicates.
4. Delete dead or confusing source-over depth-write branches once tests prove they are unreachable.

### Phase 4: broaden visual correctness tests

1. Add resolver matrix tests that do not require GPU execution.
2. Add FramePlan/DRP2 shape tests for mixed and rejected cases.
3. Add semantic-only runtime tests for two-frame toggles and descriptor refresh across stable ids.
4. Add offscreen readback tests that compare known pixels or stable region averages for the tricky
   combinations.


## Automated Test Plan

### CPU-only contract and FramePlan tests

Add tests in `src/scene/tests/scene_graph.c` or a new `src/scene/tests/scene_contract_matrix.c`.

1. `test_scene_render_contract_source_over_policy`: source-over mesh requires source-over blending,
   may depth-test, and must not write normal depth.
2. `test_scene_render_contract_wboit_policy`: WBOIT mesh requires two accumulation targets, additive
   blend policy, optional depth test, and no depth write.
3. `test_scene_render_contract_depth_peel_policy`: depth-peel mesh requires init/iter/composite
   passes, exact sampled resources, and three RGBA16F peel targets.
4. `test_scene_render_contract_sampled_depth_requires_producer`: a draw that samples depth must name
   a sampled depth resource and producer pass, not only a pass-local depth attachment.
5. `test_scene_render_contract_mixed_oit_rejected`: WBOIT plus depth-peel in one panel emits a
   diagnostic until composition semantics are specified.
6. `test_scene_frame_plan_missing_graph_pass_fails_contract`: a graph-backed render role without a
   graph pass is reported as invalid.
7. `test_scene_frame_plan_node_reallocation_safe`: many visuals and graph nodes force node-array
   growth while preserving all render-node visual assignments.
8. `test_scene_msaa_resolved_sample_count_diagnostic`: unsupported requested MSAA sample count is
   rejected or lowered with an explicit diagnostic.
9. `test_scene_role_work_label_mapping_complete`: every render pass role has one centralized label
   and graph lookup mapping.
10. `test_scene_pipeline_key_append_checked`: long combinations of suffixes fail safely instead of
   truncating to an aliased key.


### DRP2 semantic and fixture tests

Extend `src/drp2/tests/test_drp2.c` and `spec/drp2/fixtures/positive`.

Recommended positive fixtures:

1. `source_over_blend_depth.json`
2. `wboit_accumulation_resolve.json` extensions for depth-tested accumulation
3. `depth_peeling_ping_pong.json`
4. `msaa_resolve_readback.json`
5. `scene_occlusion_prepass_sample.json`
6. `volume_occlusion_prepass_sample.json`
7. `ssao_gbuffer_composite.json`
8. `edl_depth_resolve.json`

Recommended negative tests:

1. color attachment uses a depth format
2. depth attachment uses a non-depth format
3. pipeline color target format does not match render-pass attachment format
4. WBOIT accumulation pipeline is missing one blend target
5. depth-peel composite samples an unproduced ping/pong resource
6. graph pass reads a resource with no prior writer when one is required


### Scene/runtime semantic tests

These should execute through scene-emitted DRP2 and the semantic runtime where possible.

1. Toggle a visual from `BLENDED -> WBOIT -> BLENDED` across frames and assert descriptors and
   pipelines refresh without stale resources.
2. Toggle `depth_test` on/off on transparent mesh visuals and assert the resolved contract and DRP2
   pipeline state change together.
3. Toggle volume occlusion and scene occlusion on/off across frames and assert sampled resource
   bindings are recreated or removed correctly.
4. Resize a retained offscreen target with WBOIT, depth peeling, volume, and scene occlusion enabled
   and assert stable resource ids refresh descriptors.


### Offscreen readback tests

Add tests in `src/scene/tests/app.c` using small deterministic scenes, stable camera/controller
state, fixed clear colors, and 64x64 or 128x128 targets. Prefer pixel-region averages over single
pixels when edges or MSAA are involved.

1. `test_app_offscreen_source_over_mesh_depth_and_blend`: render an opaque background/card and a
   translucent mesh. Assert the center pixel is a blend of expected colors, and an opaque front card
   occludes the transparent mesh.
2. `test_app_offscreen_wboit_mesh_order_independent_layers`: render two overlapping transparent
   mesh layers in two visual orders. Assert center-region colors are approximately equal.
3. `test_app_offscreen_depth_peel_mesh_two_layers`: render two translucent crossing layers and an
   opaque occluder. Assert both transparent colors contribute where expected and the occluder wins
   where expected.
4. `test_app_offscreen_volume_slice_transparent_mesh_occlusion_matrix`: loop over `BLENDED`,
   `WBOIT`, and `DEPTH_PEEL`, and over occlusion modes `none`, `volume`, `scene`, and `both`.
   Assert transparent mesh contribution remains visible and occluded volume/slice regions dim only
   where expected.
5. `test_app_offscreen_scene_occlusion_hidden_alpha`: compare hidden, nearly transparent, and opaque
   occluders. Hidden or below-threshold occluders must not darken the target; opaque occluders must.
6. `test_app_offscreen_volume_occlusion_region_delta`: compare volume occlusion enabled/disabled
   and assert only the occluded region changes beyond tolerance.
7. `test_app_offscreen_msaa_edge_changes_coverage`: render a diagonal primitive/mesh with MSAA
   disabled/enabled and assert edge-region coverage changes while interior pixels remain stable.
8. `test_app_offscreen_points_edl_changes_pixels`: capture with EDL enabled and disabled, without
   disabling EDL before capture, and assert depth-edge darkening.
9. `test_app_offscreen_sphere_ssao_darkens_contact`: render a small sphere/mesh contact scene and
   assert SSAO darkens contact/cavity pixels relative to the disabled baseline.


## Subagent Collation

The semantic audit emphasized that the resolver design is sound, but current contracts are too coarse
for sampled depth, blend equations, exact occlusion resources, mixed transparency composition, and
dependency-derived graph ordering.

The implementation audit identified concrete robustness risks beyond semantics: stale render-node
pointers across FramePlan reallocation, graph-emission failures that only log, skipped missing graph
passes during contract validation, MSAA sample-count drift after validation, pipeline-key truncation,
and missing DRP2 format-class checks.

The testing audit found that shape coverage is relatively strong, but offscreen visual correctness is
not yet strong enough. Existing WBOIT and depth-peel runtime tests mostly check nonblank pixels, while
the crucial volume/mesh/occlusion/transparency matrices are not covered by deterministic readback
assertions.


## Suggested Acceptance Criteria

This area should be considered solid when all of the following are true:

1. Every scene draw that reaches DRP2 has one resolved draw contract.
2. Every graph-backed render node has one resolved pass contract and one matching graph pass.
3. DRP2 pipeline color targets, blend state, depth state, multisampling, raster state, and bind-group
   layouts are checked against the scene contract.
4. Mixed WBOIT/depth-peel/source-over composition is either explicitly rejected or explicitly ordered.
5. Sampled depth and depth attachments are represented as separate contracts.
6. Offscreen readback tests cover source-over, WBOIT, depth peeling, volume plus transparent mesh,
   volume occlusion, scene occlusion, MSAA, EDL, and SSAO.
7. Capability fallbacks such as MSAA lowering are reported and tested.
8. FramePlan and graph storage cannot invalidate retained pointers during emission.


## Validation Performed For This Report

The initial audit report was read-only. Later implementation-log entries record the focused build,
runtime, and diff-hygiene validation for each follow-up slice.
