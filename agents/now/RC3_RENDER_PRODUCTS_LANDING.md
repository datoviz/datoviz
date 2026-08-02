# RC3 Render Products Landing Manifest

Status: R1-R9 implementation complete and validated on `refactor/rc3-render-products`; final evidence checkpoint and push are pending. Updated: 2026-08-03.

## Revision Boundary

- Campaign merge base: `2751887de1b01d96ea14e8d005120cc7a51e4939` (`origin/v0.4-dev` at base freeze).
- Render implementation parent after approved integration prerequisites: `c657845f1`.
- Validated implementation head through R9: `c0eada723`.
- Final integration head: pending this evidence-only checkpoint.

Ordered render commits after `c657845f1`:

1. `a20596828` — add typed semantic render products.
2. `8fd629493` — centralize panel technique composition.
3. `5cf2c8bcf` — make runtime emission transactional.
4. `0fb26465e` — declare typed render work.
5. `3224b95be` — lower typed composition graphs.
6. `721919411` — persist graph realizations.
7. `2a4a3ff04` — cut over typed composition lowering.
8. `64bfdad9a` — resolve typed runtime graph resources.
9. `1090601fa` — gate native scatter input on app support.
10. `fd2463899` — resolve runtime work by typed provider.
11. `fbaf2d9c3` — bind typed provider uniforms.
12. `cdb049791` — unify runtime technique work records.
13. `663b9427b` — select render policy by work provider.
14. `42c4e25bb` — lower composed render products generically.
15. `662076f5b` — make technique coordinates panel local.
16. `2ecea32e2` — expose panel-local work geometry.
17. `3f583e912` — realize technique products per panel.
18. `21dc2bf4a` — make surface records coherent across MSAA.
19. `c39a7ae9a` — migrate effects to typed render products.
20. `f7fcb4233` — replace SSAO with material-aware GTAO.
21. `5b6820b4d` — replace the SSAO public API with semantic AO.
22. `c0eada723` — remove legacy technique composition paths and promote final specifications.

## Exact Changed Scope

The authoritative implementation path set is `git diff --name-only c657845f1..c0eada723`. It is exhaustively partitioned below; the evidence-only checkpoint changes only the two landing records.

| Scope | Exact paths |
| --- | --- |
| Public API and ABI | `include/datoviz/scene.h`, `include/datoviz/scene/enums.h`, `include/datoviz/scene/frame_plan.h`, `include/datoviz/scene/types.h`, `symbols.map`, `spec/api/C_API_REFERENCE_POLICY.yaml`, `datoviz/_ctypes.py` |
| Scene frame-plan products | `src/scene/frame_plan/ascii.c`, `composition.c`, `core.c`, `emit.h`, `frame_plan.h`, `graph/helpers.c`, `graph/validation.c`, `internal.h`, `json.c`, `nodes.c`, `products.c`, `realizations.c` |
| Scene composition and retained state | `src/scene/core/_scene.h`, `core/panel_geometry.c`, `core/scene.c`, `interaction/core.c`, `scene_emit/composition.c`, `scene_emit/composition_graph.c`, `scene_emit/internal.h`, `scene_emit/panel.c`, `scene_emit/panel_render_plan.c`, `scene_emit/panel_render_plan.h`, `scene_emit/upload_support.c` |
| Render contract | every file under `src/scene/render_contract/` |
| Generic runtime lowering | `src/scene/runtime/_frame_plan_runtime_internal.h`, `_scene_common_bindings.h`, `bind_groups.c`, `common_bindings.c`, `frame_plan.c`, `graph_resources.c`, `render_emit_draws.c`, `render_emit_passes.c`, `render_emit_prepare.c`, `resolved_shader.c`, `state.c`, `technique_targets.c` |
| Technique state and removed legacy builders | `src/scene/techniques/_technique.h`, `state.c`; deleted `graph_depth_peel.c`, `graph_gbuffer.c`, `graph_ssao.c`, and `graph_wboit.c` |
| Visual capability and pipeline registry | `src/scene/visuals/_visual_pipeline.h`, `_visual_pipeline_internal.h`, `bind_desc.c`, `registry/pass_caps.c`, `registry/pipeline_desc.c`, `registry/registry.c`, `registry/registry.h`, `registry/shader_desc.c` |
| Shader registry and build | `src/scene/CMakeLists.txt`, `src/scene/shaders/_shader_registry.h`, `src/scene/shaders/registry.c` |
| Active GLSL/WGSL shaders | `src/scene/shaders/glsl/common.glsl`, `depth_peel_back.frag`, `depth_peel_back_lit.frag`, `depth_peel_composite.frag`, `depth_peel_front.frag`, `depth_peel_front_lit.frag`, `edl_resolve.frag`, `fullscreen.vert`, `gbuffer_normal.frag`, `gtao.frag`, `gtao_denoise.frag`, `gtao_visibility_present.frag`, `marker.frag`, `marker_bitmap.frag`, `marker_distance.frag`, `mesh_textured.frag`, `path.frag`, `pixel.frag`, `pixel_cue.frag`, `pixel_item_state.frag`, `point.frag`, `point_cue.frag`, `point_cue_style.frag`, `point_item_state.frag`, `point_style.frag`, `primitive.frag`, `primitive_lit.frag`, `scene_material.glsl`, `scene_occlusion.glsl`, `scene_occlusion_depth.frag`, `segment.frag`, `sphere.frag`, `sphere.vert`, `sphere_a2c.frag`, `sphere_analytic.glsl`, `sphere_gbuffer.frag`, `sphere_gbuffer.vert`, `sphere_item_state.vert`, `sphere_pick.frag`, `sphere_query_u32.frag`, `sphere_query_u32.vert`, `sphere_vertex.glsl`, `splat.frag`, `surface_depth.glsl`, `surface_resolve.frag`, `volume_composite.frag`, `volume_labels_sint_composite.frag`, `volume_labels_sint_slice.frag`, `volume_labels_uint_composite.frag`, `volume_labels_uint_slice.frag`, `volume_mip.frag`, `volume_occlusion_depth.frag`, `volume_slice.frag`, `wboit_resolve.frag`, and `src/scene/shaders/wgsl/scene_material.wgsl` |
| Removed legacy shader paths | Deleted `src/scene/shaders/glsl/ssao.frag`, `ssao_blur.frag`, `ssao_composite.frag`, `src/vklite/tests/shaders/ssao.frag`, and `src/vklite/tests/shaders/ssao_depth.frag` |
| App trace | `src/app/trace.c`, `src/app/tests/test_app.c` |
| Scene tests and guards | `src/scene/tests/app.c`, `axis.c`, `frame_plan.c`, `frame_plan_emit.c`, `interaction.c`, `scene_graph.c`, `scene_graph_utils.c`, `scene_graph_utils.h`, `scene_interaction_graph.c`, `scene_techniques.c`, `test_scene.h`, `visuals/families_2d.c`, `visuals/geometry.c`, `visuals/runtime.c`, `visuals/state.c`, `testing/test_scene_architecture_source_guard.py` |
| Native GPU test support | `src/vklite/tests/test_techniques.c`, `test_vklite.c`, `test_vklite.h`, and `testing/components/dvztest_vk.c` |
| DRP2 and WebGPU contract | `spec/drp2/COMMANDS.md`, `spec/drp2/schema/commands/BeginRenderPass.json`, `spec/drp2/fixtures/positive/scene_image_wgsl_from_c.json`, `scene_point_wgsl_from_c.json`, `scene_primitive_wgsl_from_c.json`, `spec/scene/pipeline/FRAME_PLAN_SERIALIZATION.md` |
| Examples and authored callers | `examples/c/CMakeLists.txt`, `MANIFEST.yaml`, `example_gui_controls.c`, `example_gui_controls.h`, `example_tuner.c`, `example_tuner.h`, `features/README.md`, `features/technique_ao.c`, `lab/protein_viewer.c`, `legacy/visuals/sphere.c`, `showcases/protein.c`, `start/scatter.c`, `examples/python/gallery/features/technique_ao.py`, `examples/python/gallery/showcases/protein.py`, `examples/webgpu/live_examples.js`, `tools/check_example_manifests.py`; deleted `examples/c/legacy/tools/frame_plan_graph_debug.c` |
| Generated/public documentation | `docs/reference/c-api/{app,drp2,frame-plan,runtime-utilities,runtime-vklite,runtime-vulkan,scene,techniques,types,visuals}.md`, `docs/examples/capabilities.json`, `examples.json`, `features.md`, `navigation.yaml`, `validation-gallery.md`, `webgpu-matrix.md`, `docs/examples/gallery/features/features_technique_ao.md`, `features_technique_edl.md`, `features_technique_msaa.md`, `docs/examples/gallery/showcases/showcases_protein.md`, `docs/architecture/pbr_materials_roadmap.md`, `scene_techniques_materials.md`, `docs/how-to/depth-blending.md`, `spec/docs/EXAMPLE_COVERAGE.md` |
| Authoritative scene specifications and proposal history | `spec/scene/examples/TECHNIQUES.md`, `spec/scene/implementation/GRAPH_TECHNIQUES.md`, `OCCLUSION_EFFECTS.md`, `README.md`, `TRANSPARENCY_MSAA.md`, `spec/scene/semantics/EFFECTS.md`, `spec/scene/validation/RENDER_CONFORMANCE.md`, `spec/scene/proposals/active/README.md`, `spec/scene/proposals/promoted/README.md`, and the move of `RENDER_PRODUCTS_AND_TECHNIQUE_COMPOSITION.md` from `proposals/active/` to `proposals/promoted/` |
| Active execution records | `agents/now/HANDOFF_RC3_RENDER_QA_ORCHESTRATION.md`, `HANDOFF_RENDER_PRODUCTS_REFACTOR.md`, `RELEASE.md`, `START.md`, `STATUS.md`, this manifest, and `RC3_RENDER_PRODUCTS_AFFECTED_QA.md` |

The exact R9 delta committed by `c0eada723` was:

```text
M  agents/now/HANDOFF_RC3_RENDER_QA_ORCHESTRATION.md
M  agents/now/HANDOFF_RENDER_PRODUCTS_REFACTOR.md
M  agents/now/RELEASE.md
M  agents/now/START.md
M  agents/now/STATUS.md
A  agents/now/RC3_RENDER_PRODUCTS_AFFECTED_QA.md
A  agents/now/RC3_RENDER_PRODUCTS_LANDING.md
D  examples/c/legacy/tools/frame_plan_graph_debug.c
M  spec/scene/implementation/GRAPH_TECHNIQUES.md
M  spec/scene/implementation/README.md
M  spec/scene/implementation/TRANSPARENCY_MSAA.md
M  spec/scene/proposals/active/README.md
D  spec/scene/proposals/active/RENDER_PRODUCTS_AND_TECHNIQUE_COMPOSITION.md
M  spec/scene/proposals/promoted/README.md
A  spec/scene/proposals/promoted/RENDER_PRODUCTS_AND_TECHNIQUE_COMPOSITION.md
M  src/scene/frame_plan/ascii.c
M  src/scene/frame_plan/composition.c
M  src/scene/frame_plan/frame_plan.h
M  src/scene/frame_plan/json.c
M  src/scene/render_contract/core.c
M  src/scene/render_contract/drp2.c
M  src/scene/runtime/_frame_plan_runtime_internal.h
M  src/scene/runtime/render_emit_passes.c
M  src/scene/runtime/resolved_shader.c
M  src/scene/runtime/technique_targets.c
M  src/scene/scene_emit/composition.c
M  src/scene/scene_emit/composition_graph.c
M  src/scene/scene_emit/internal.h
M  src/scene/scene_emit/panel.c
M  src/scene/scene_emit/upload_support.c
M  src/scene/shaders/_shader_registry.h
A  src/scene/shaders/glsl/gtao.frag
A  src/scene/shaders/glsl/gtao_denoise.frag
A  src/scene/shaders/glsl/gtao_visibility_present.frag
D  src/scene/shaders/glsl/ssao.frag
D  src/scene/shaders/glsl/ssao_blur.frag
D  src/scene/shaders/glsl/ssao_composite.frag
M  src/scene/shaders/registry.c
M  src/scene/techniques/_technique.h
D  src/scene/techniques/graph_depth_peel.c
D  src/scene/techniques/graph_gbuffer.c
D  src/scene/techniques/graph_ssao.c
D  src/scene/techniques/graph_wboit.c
M  src/scene/techniques/state.c
M  src/scene/tests/app.c
M  src/scene/tests/frame_plan.c
M  src/scene/tests/frame_plan_emit.c
M  src/scene/tests/scene_graph.c
M  src/scene/tests/scene_techniques.c
M  src/scene/tests/test_scene.h
M  src/scene/tests/visuals/state.c
D  src/vklite/tests/shaders/ssao.frag
D  src/vklite/tests/shaders/ssao_depth.frag
M  src/vklite/tests/test_techniques.c
M  src/vklite/tests/test_vklite.c
M  src/vklite/tests/test_vklite.h
M  testing/components/dvztest_vk.c
M  testing/test_scene_architecture_source_guard.py
```

The legacy `features/technique_ssao.c`, Python counterpart, and gallery page are renamed to `technique_ao`; Git records these as delete/add pairs until similarity detection. Historical release evidence and the protected `data` submodule retain their original SSAO-labelled artifact names.

## Contract Deltas

- Ownership and identity: typed product versions, provider keys, and declared accesses replace effect names, labels, suffixes, and render-pass roles as runtime identity.
- Frame state and recovery: emission is transactional; persistent realizations survive unchanged plans and refresh safely after resize or descriptor invalidation.
- Resource layout: intermediates are panel-relative, carry local origin/extent transforms, and may alias only across non-overlapping compatible lifetimes.
- Surface semantics: depth, signed view normal, coverage, and color describe one winning opaque or masked fragment, including analytic sphere hits, clipping, corrected depth, and alpha-to-coverage.
- Format, samples, and resolve: every product declares format class, sample domain, and resolve policy; semantic depth/normal/coverage/ID products never use an implicit arithmetic color resolve.
- Composition: AO-aware opaque shading precedes EDL; source-over/WBOIT/depth peeling preserve authored transparency order; volume precedes overlay; every color transform consumes one scene-color version and produces a successor.
- AO: deterministic full-resolution GTAO consumes the coherent surface record, uses bounded edge-aware denoising, and modulates only eligible ambient or indirect diffuse light. The public API is semantic `DvzAoDesc`/`dvz_ao_desc()`/`dvz_panel_set_ao()`.
- Trace: normalization is generic and scope-aware; effect-name and resource-suffix interpretation is absent from the active contract.
- DRP2: render-pass attachment metadata carries explicit resolve and load/store facts required by typed lowering. WebGPU fixtures and preflight consume the same declared semantics.
- Errors: missing producers, incompatible product/resource semantics, illegal aliasing, sample mismatches, read-before-produce, and unsupported capabilities produce exact diagnostics instead of silent fallback or black composition.

## Validation Evidence And Limitations

Exact-head validation of `c0eada723` passed the full native build; 1,080/1,114 native tests with 0 failures and 34 display-only skips; 566/566 scene CPU tests; 159/160 scene GPU tests with 0 failures and one GLFW skip; 91/98 runtime/vklite tests with 0 failures and seven GLFW skips; 94/94 DRP2 contract tests; all 125 DRP2 fixtures; shader ABI; specifications and source guards; regenerated ctypes, ABI/policy checks, raw smoke, and 50 Python binding tests; and API/generated/status/build documentation checks. GPU evidence ran on NVIDIA GeForce RTX 5090 with Vulkan validation enabled. GTAO evidence covered perspective/orthographic projection, the issue-137 zoom sweep, stationary redraw, MSAA 1x/4x, unequal panels, resize round-trip, background validity, mesh ambient-only material integration, and analytic spheres; EDL, source-over, WBOIT, depth peeling, volume occlusion, overlays, and queries also passed their focused native GPU cases.

Known landing limitations:

- The protected `data` submodule still contains historical `features_technique_ssao` gallery media; the render landing does not stage or update its gitlink.
- `just check-example-manifests` fails only because that protected historical media remains named `data/gallery/v0.4/features/features_technique_ssao.png`; changing it requires a separately approved data-submodule update.
- `just webgpu-check` builds the WASM scene target and passes its loader/progress/overlay checks, then stops because the protected data checkout lacks the prepared US-state choropleth bundle. DRP2/WebGPU fixture preflight and runner smoke pass independently.
- GLFW, GUI, canvas swapchain, external-surface, and vklite presentation cases are unavailable because this headless runner has no `DISPLAY`; all offscreen Vulkan paths pass.
- Physical Windows, macOS, AMD, Intel, and WebGPU browser hardware evidence is outside this local NVIDIA/Linux worktree and remains governed by the RC3 release matrix.
- Historical release evidence keeps the labels captured at the time; it is not renamed to imply a different artifact.

## Unaffected Claims

The render commits after `c657845f1` do not modify `math`, `window`, `stream`, or `video` source. Their isolated CPU algorithms remain textually unaffected. Their integration conclusions are not automatically preserved where they consume public scene headers, DRP2 streams, canvas presentation, or frame lifecycle; those boundary slices are explicitly rerun by the affected-QA manifest.
