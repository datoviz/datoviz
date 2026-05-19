# Scene Implementation Notes

Concise implementor documentation for active scene internals. These notes describe the current
`src/scene` vertical slice and should not override public API or DRP2 protocol specs.

## Files

1. [VISUAL_SHADER_REFACTOR.md](VISUAL_SHADER_REFACTOR.md): shader ABI, visual-family and shader-variant
   extension steps, descriptor checklist, and validation checklist.
2. [TEXT_SHAPING_ATLAS.md](TEXT_SHAPING_ATLAS.md): implementation-facing text shaping, layout,
   atlas, cache, and DRP2 emission contract.
3. [GRAPH_TECHNIQUES.md](GRAPH_TECHNIQUES.md): implementation-facing graph technique, resource,
   pass, material capability, and runtime guardrail contract.
4. [OCCLUSION_EFFECTS.md](OCCLUSION_EFFECTS.md): implementation-facing SSAO, scene occlusion,
   volume occlusion, shader feature, and validation contract.

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
