# FramePlan Graph And Transparency Plan

Date: 2026-05-15

This note records the recommended architecture direction after the first WBOIT mesh smoke. The
short version: dual depth peeling should not be added as another WBOIT-shaped special case. First
generalize FramePlan and DRP2 enough to express typed pass/resource graphs, then implement
transparency techniques as graph expansions.


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

Use three conceptual layers:

1. Scene intent: panels, visuals, controllers, alpha modes, picking/probing requests.
2. Technique expansion: WBOIT, shell two-pass, depth peeling, SSAO, picking, postprocess builders.
3. Generic FramePlan graph: typed resources, typed passes, dependencies, and commands to emit.

DRP2 emission remains the backend-facing lowering step from this graph into concrete command
streams.


## Generic Resource Model

FramePlan resources should be ordinary graph nodes, not hard-coded WBOIT or peeling fields.

Minimum descriptor:

```text
Resource {
    id
    kind: texture | buffer | external_target
    format
    extent: figure | panel | fixed | resource_ref
    usage: render_attachment | depth_attachment | sampled | storage | copy_src | copy_dst
    lifetime: borrowed | per_frame | persistent
}
```

Examples:

- `rt`: borrowed final target.
- `panel0.depth.opaque`: per-frame depth texture.
- `panel0.wboit.accum`: per-frame color texture.
- `panel0.peel.depth_ping`: per-frame color/depth-like texture.
- `panel0.ssao.normal`: per-frame normal texture.

Technique code owns names and creation policy; core FramePlan only sees typed resources.


## Generic Pass Model

Minimum pass descriptor:

```text
Pass {
    id
    kind: render | compute | copy | readback | clear
    panel_id
    viewport
    scissor
    reads: ResourceAccess[]
    writes: ResourceAccess[]
    color_attachments: Attachment[]
    depth_attachment: Attachment?
    stencil_attachment: Attachment?
    work: draws | dispatches | copies | readback
}

Attachment {
    resource_id
    load_op: clear | load | dont_care
    store_op: store | dont_care
    clear_value
    access: read | write | read_write
}

ResourceAccess {
    resource_id
    usage: sampled | storage_read | storage_write | color_attachment |
           depth_attachment_read | depth_attachment_write | copy_src | copy_dst
}
```

Pass tags or roles should remain diagnostic and policy-facing, not the core execution model.
Ordering can stay explicit at first, with validation checking that read/write dependencies make
sense. Dependency-derived ordering can come later.


## Technique Expansion Examples

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

SSAO builder:

1. Add gbuffer resources for depth/normal as needed.
2. Add SSAO pass.
3. Add optional blur pass.
4. Add composite pass.


## DRP2 Gaps To Close

The current DRP2/vklite path already supports many pieces: intermediate textures, multi-color
render targets, sampled textures, storage buffers, compute pipelines, color blend state, depth
compare/write state, and multi-pass execution.

Missing or underspecified pieces for a generic graph:

1. Named depth attachments/resources instead of only implicit transient depth.
2. Explicit attachment load/store operations in the command stream.
3. Explicit attachment access: depth read, depth write, depth read/write.
4. Texture/resource layout transitions driven from declared access, not only local WBOIT assumptions.
5. Pipeline raster state such as cull mode and front-face winding.
6. Better validation of pipeline target formats versus actual render-pass attachment formats.
7. Capability facts for sampled depth, storage images, required formats, max attachments, blend ops,
   and independent blend.

Storage textures already exist as a binding type, but they should be validated and exercised before
being treated as available for graph techniques.


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

1. Add the generic FramePlan resource/pass descriptors and tests with no behavior change.
2. Extend DRP2 with named depth resources and explicit attachment load/store ops.
3. Convert current WBOIT lowering to use the generic resource/pass graph while preserving the
   emitted command shape.
4. Add shell two-pass as the smallest new transparency technique and use it to validate pipeline
   cull/front-face state.
5. Prototype dual depth peeling as a focused DRP2/vklite fixture.
6. Lift dual depth peeling into scene as a technique builder and public alpha mode.
7. Reuse the same graph machinery for SSAO and later postprocess/volume passes.


## Non-Goals For The First Slice

- No resource aliasing or transient memory allocator.
- No async compute scheduling.
- No automatic pass reordering beyond explicit insertion order.
- No public low-level frame graph API yet.
- No replacement of the scene API with graph construction.

The initial target is a small explicit graph that makes advanced scene techniques expressible,
inspectable, testable, and portable across future DRP2 backends.
