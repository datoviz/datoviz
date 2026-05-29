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
7. [SCENE_CODE_SPLIT_ROADMAP.md](SCENE_CODE_SPLIT_ROADMAP.md): implementation-facing roadmap for
   splitting scene frame-plan, runtime, core, annotation, domain, query, and visual helper code.
8. [SCENE_ARCHITECTURE_COMPLETION_PLAN.md](SCENE_ARCHITECTURE_COMPLETION_PLAN.md): remaining
   end-to-end plan for the final scene architecture, visual-family registry, untyped descriptor
   removal, coarse reusable CMake layers, and architecture done criteria.
9. [SCENE_VISUAL_BOUNDARY_GUARDRAILS.md](SCENE_VISUAL_BOUNDARY_GUARDRAILS.md): next major
   visual-architecture phase for keeping generic scene code registry-driven and confining
   visual-specific behavior to family folders or explicit shared visual subsystems.

## Current priorities

1. Keep `just shader-abi-check` green when moving shader files or bind layouts.
2. Advance WGSL parity one visual family at a time; see `VISUAL_SHADER_REFACTOR.md`.
3. Keep runtime emission descriptor-driven rather than adding visual-family policy there.
4. Keep text execution plans in `agents/soon/` focused on order and validation while durable text
   resource contracts live here or in `../semantics/TEXT.md`.
5. Keep graph-backed effects and transparency plans in `agents/soon/` focused on pickup order while
   durable technique implementation rules live in `GRAPH_TECHNIQUES.md`.
6. Keep SSAO and occlusion execution plans in `agents/soon/` focused on remaining slices while
   durable occlusion implementation rules live in `OCCLUSION_EFFECTS.md`.
7. Keep transparency, depth-peeling, and MSAA execution plans in `agents/soon/` focused on
   remaining slices while durable implementation rules live in `TRANSPARENCY_MSAA.md`.
8. Keep broad scene source splits staged through `SCENE_CODE_SPLIT_ROADMAP.md`, and use
   `SCENE_ARCHITECTURE_COMPLETION_PLAN.md` for the final architecture target: explicit visual
   metadata, registry-driven visual-family operations, descriptor-driven runtime emission, and
   coarse reusable CMake targets.
9. Use `SCENE_VISUAL_BOUNDARY_GUARDRAILS.md` before adding new root-level visual switches,
   family-private includes from generic code, or family-specific fields to generic retained visual
   state.
