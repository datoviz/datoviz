# FramePlan Graph And Transparency Plan

Date: 2026-05-15

This note records the recommended architecture direction after the first WBOIT mesh smoke. The
short version: dual depth peeling should not be added as another WBOIT-shaped special case. First
generalize FramePlan and DRP2 enough to express typed pass/resource graphs, then implement
transparency techniques as graph expansions.

The durable graph-backed technique contract now lives in
[spec/scene/implementation/GRAPH_TECHNIQUES.md](../../../spec/scene/implementation/GRAPH_TECHNIQUES.md).
Keep this note focused on transparency pickup order and gaps.


## Motivation

The current retained scene path can already emit simple scene render passes and a special WBOIT
multi-pass route:

1. opaque render pass
2. WBOIT accumulation pass
3. WBOIT resolve pass

That route proved the scene -> FramePlan -> DRP2 -> vklite layering can support multi-pass
rendering, intermediate render targets, sampled resolve passes, and reused opaque depth. It also
showed the limitation of encoding each technique directly in the scene emitter. Dual depth peeling,
SSAO, bloom, picking improvements, volume compositing, and future postprocess effects all need the
same generic concepts:

- named per-frame resources,
- render/compute/copy passes,
- explicit pass reads and writes,
- color/depth/stencil attachments with load/store policy,
- stable pass ordering and diagnostics.


## Direction

FramePlan should become a small typed pass/resource graph. This should not be a full optimizing
frame graph engine yet. The first goal is explicit, deterministic, validated multi-pass planning,
not transient resource aliasing, automatic async scheduling, or aggressive pass reordering.

The agreed implementation strategy is deliberately incremental: add the FramePlan graph vocabulary
first as schema, validation, and deterministic debug output with no emitted-command behavior change;
then upgrade the DRP2 attachment/resource contract that every graph-backed technique will need.
The DRP2 work is required foundation, not optional cleanup, but it should be driven by the typed
FramePlan model rather than designed as a broad backend rewrite in isolation.

Use three conceptual layers:

1. Scene intent: panels, visuals, controllers, alpha modes, picking/probing requests.
2. Technique expansion: WBOIT, shell two-pass, depth peeling, SSAO, picking, postprocess builders.
3. Generic FramePlan graph: typed resources, typed passes, dependencies, and commands to emit.

DRP2 emission remains the backend-facing lowering step from this graph into concrete command
streams.


## Generic Resource And Pass Model

Use the resource, pass, and access model in
[GRAPH_TECHNIQUES.md](../../../spec/scene/implementation/GRAPH_TECHNIQUES.md). Transparency
builders should append typed graph resources and passes instead of adding WBOIT-shaped special
cases in the scene emitter.


## Transparency Expansion Examples

WBOIT builder:

1. Ensure opaque pass writes `rt` and opaque depth.
2. Add `accum` and `weight` textures.
3. Add transparent accumulation render pass writing both textures and reading opaque depth.
4. Add resolve render pass sampling both textures and writing `rt`.

Shell two-pass builder:

1. Ensure opaque pass writes `rt` and opaque depth.
2. Add back-face shell render pass over `rt`, depth-tested, depth-write disabled.
3. Add front-face shell render pass over `rt`, depth-tested, depth-write disabled.

Dual depth peeling builder:

1. Ensure opaque pass writes `rt` and opaque depth.
2. Add front/back accumulators and ping/pong depth or min/max textures.
3. Add peel initialization pass.
4. Add a fixed number of peel iteration passes, ping-ponging resources.
5. Add composite pass sampling accumulators and writing `rt`.

## DRP2 Gaps To Close

The shared DRP2 graph gaps are tracked in
[GRAPH_TECHNIQUES.md](../../../spec/scene/implementation/GRAPH_TECHNIQUES.md). For the transparency
lane, the most important remaining pieces are named depth resources, explicit attachment access,
layout transitions from declared graph access, and raster cull/front-face state.


## Scene Gaps To Close

1. Extend alpha-mode policy without making every mode a hard-coded render path.
2. Add technique builders that append generic resources and passes.
3. Preserve the simple opaque path for scenes with no advanced technique.
4. Keep user-facing APIs declarative:

```c
dvz_visual_set_alpha_mode(shell, DVZ_ALPHA_DEPTH_PEEL);
```

Users should not manage pass counts, intermediate textures, resolve shaders, or target names in the
common path. Technique descriptors can expose bounded controls later, for example peel count or
quality policy.


## Recommended Sequence

1. Add the generic FramePlan resource/pass descriptors, validation, JSON/debug output, and tests
   with no emitted-command behavior change.
2. Extend DRP2 with named depth resources and explicit color/depth attachment load/store ops.
3. Generalize DRP2 attachment access and texture/resource layout transitions from declared graph
   access, instead of relying on local WBOIT assumptions.
4. Add DRP2 render-pipeline raster state for cull mode and front-face winding, which shell and
   depth-peeling techniques need.
5. Convert current WBOIT lowering to use the generic resource/pass graph while preserving the
   emitted command shape.
6. Add shell two-pass as the smallest new transparency technique and use it to validate pipeline
   cull/front-face state.
7. Prototype dual depth peeling as a focused DRP2/vklite fixture.
8. Lift dual depth peeling into scene as a technique builder and public alpha mode.
9. Reuse the same graph machinery for SSAO and later postprocess/volume passes.


## Non-Goals For The First Slice

- No resource aliasing or transient memory allocator.
- No async compute scheduling.
- No automatic pass reordering beyond explicit insertion order.
- No public low-level frame graph API yet.
- No replacement of the scene API with graph construction.

The initial target is a small explicit graph that makes advanced scene techniques expressible,
inspectable, testable, and portable across future DRP2 backends.
