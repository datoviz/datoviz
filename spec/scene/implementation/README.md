# Scene Implementation Notes

Concise implementor documentation for active scene internals. These notes describe the current
`src/scene` vertical slice and should not override public API or DRP2 protocol specs.

Keep implementation notes concrete and short: current boundary, constraints, internal data flow,
failure/lifetime/performance cases, validation, and known gaps. Link to semantic specs for public
behavior instead of restating it here.

## Files

1. [VISUAL_SHADER_REFACTOR.md](VISUAL_SHADER_REFACTOR.md): shader ABI, visual-family and shader-variant
   extension steps, descriptor checklist, and validation checklist.
2. [TEXT_SHAPING_ATLAS.md](TEXT_SHAPING_ATLAS.md): implementation-facing text shaping, layout,
   atlas, cache, and DRP2 emission contract.
3. [TEXT_BLOCK_BACKENDS.md](TEXT_BLOCK_BACKENDS.md): implementation-facing CPU-raster text-block
   backend contract for rich paragraphs, formatted annotations, and bitmap math.
4. [GRAPH_TECHNIQUES.md](GRAPH_TECHNIQUES.md): implementation-facing graph technique, resource,
   pass, material capability, and runtime guardrail contract.
5. [OCCLUSION_EFFECTS.md](OCCLUSION_EFFECTS.md): implementation-facing SSAO, scene occlusion,
   volume occlusion, shader feature, and validation contract.
6. [TRANSPARENCY_MSAA.md](TRANSPARENCY_MSAA.md): implementation-facing WBOIT, depth peeling,
   MSAA, alpha-to-coverage, and validation contract.
7. [COLOR_MANAGEMENT_IMPLEMENTATION_PLAN.md](COLOR_MANAGEMENT_IMPLEMENTATION_PLAN.md):
   landed implementation checklist for explicit texture roles, linear rendering, one final sRGB
   encode, and remaining export/readback audit points.
8. [FRAME_ARTIFACT_REFACTOR_PLAN.md](FRAME_ARTIFACT_REFACTOR_PLAN.md): active plan to make
   `DvzSceneFrameArtifact` the single scene emission product while preserving DRP2 streams as the
   artifact-owned execution IR.
9. [PANEL_VIEW_ARCHITECTURE_PLAN.md](PANEL_VIEW_ARCHITECTURE_PLAN.md): active plan for panel 2D
   view ownership, release-candidate API naming, and resolver-derived fitted domains.
10. [PLOT_VIEWPORT_DRAW_CONTRACT_PLAN.md](PLOT_VIEWPORT_DRAW_CONTRACT_PLAN.md): implemented
   native contract for separating per-draw viewport and scissor selection, viewport uniforms, and
   CPU pixel-basis rules for plot-clipped draws.
11. [SCENE_CODE_SPLIT_ROADMAP.md](SCENE_CODE_SPLIT_ROADMAP.md): retired pointer for the completed
   broad scene source split; durable visual-boundary rules live in
   [`../visuals/BOUNDARY_CONTRACT.md`](../visuals/BOUNDARY_CONTRACT.md).
12. [SCENE_ARCHITECTURE_COMPLETION_PLAN.md](SCENE_ARCHITECTURE_COMPLETION_PLAN.md): retired pointer
   for the completed architecture queue.

## Current priorities

1. Keep `just shader-abi-check` green when moving shader files or bind layouts.
2. Advance WGSL parity one visual family at a time; see `VISUAL_SHADER_REFACTOR.md`.
3. Keep runtime emission descriptor-driven rather than adding visual-family policy there.
4. Keep current execution order in `agents/now/STATUS.md`; durable text resource contracts live here
   or in `../semantics/TEXT.md`.
5. Keep graph-backed effects and transparency implementation rules in `GRAPH_TECHNIQUES.md`.
6. Keep SSAO and occlusion implementation rules in `OCCLUSION_EFFECTS.md`.
7. Keep transparency, depth-peeling, and MSAA implementation rules in `TRANSPARENCY_MSAA.md`.
8. Use `COLOR_MANAGEMENT_IMPLEMENTATION_PLAN.md` as the color-management audit checklist before
   changing sampled-field texture roles, shader color linearization, render-target color formats, or
   screenshot/readback color encoding.
9. Use `FRAME_ARTIFACT_REFACTOR_PLAN.md` before changing scene emission ownership, WASM packet
   payloads, JSON emission, app/runtime handoff, or `dvz_figure_emit_frame()` semantics.
10. Use [`../visuals/BOUNDARY_CONTRACT.md`](../visuals/BOUNDARY_CONTRACT.md) before adding new
   root-level visual switches, family-private includes from generic code, or family-specific fields
   to generic retained visual state.
11. Use `PANEL_VIEW_ARCHITECTURE_PLAN.md` before changing panel 2D view API names, panel view code,
   or equal-aspect resolver ownership.
12. Use `PLOT_VIEWPORT_DRAW_CONTRACT_PLAN.md` before changing runtime viewport/scissor emission,
   plot-clipped draw routing, viewport uniforms, or CPU-expanded pixel geometry for axis grids.
13. Treat `SCENE_CODE_SPLIT_ROADMAP.md` and `SCENE_ARCHITECTURE_COMPLETION_PLAN.md` as retired
   compatibility links, not active queues.
