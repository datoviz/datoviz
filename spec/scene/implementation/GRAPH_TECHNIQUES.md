# Graph-Backed Technique Implementor Notes

Status: implementation-facing notes for scene techniques that expand retained panel state into
FramePlan graph resources and passes. Public semantics remain in the scene semantics and proposal
documents; this file records the implementation contract shared by transparency, EDL, SSAO, MSAA,
screen-space effects, and future graph-backed techniques.

## Stack Boundary

Graph-backed techniques must follow the active scene stack:

```text
retained scene state
  -> technique planning
  -> FramePlan graph resources and passes
  -> DRP2 command stream
  -> vklite/canvas runtime
```

Do not add a parallel renderer, visual-private postprocess path, scene-private Vulkan execution
path, or public framegraph API for these lanes.

## Current Foundation

The active graph-backed paths include:

1. opaque depth;
2. blended transparency;
3. WBOIT;
4. depth peeling;
5. blended volume composition;
6. G-buffer depth/normal resources;
7. EDL;
8. SSAO and optional SSAO blur;
9. MSAA resolve.

Technique activation is routed through retained scene or panel technique state and remains
default-off unless the technique is part of the baseline opaque path. Graph-backed runtime dispatch
should stay generic; effect-specific runtime code should be limited to shader, pipeline, bind-group,
and fullscreen draw or dispatch preparation.

## Resource Model

FramePlan resources should be ordinary typed graph nodes, not hard-coded fields for one technique.

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

1. `rt`: borrowed final target;
2. `panel0.depth.opaque`: per-frame depth texture;
3. `panel0.wboit.accum`: per-frame WBOIT accumulation texture;
4. `panel0.peel.depth_ping`: per-frame depth-peeling texture;
5. `panel0.ssao.normal`: per-frame normal texture;
6. `panel0.outline.mask`: per-frame outline mask or object-id texture.

Technique code owns names and creation policy. Core FramePlan validation should only require typed
resources, declared usage, deterministic ownership, and valid read/write relationships.

## Pass Model

Graph passes should describe logical work and attachment/resource dependencies.

Minimum descriptor:

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

Pass tags and roles are diagnostic and policy-facing. The core execution model is typed
resources, attachments, access, and explicit ordering. Dependency-derived ordering can come later;
the first priority is deterministic insertion order with validation.

## Technique Expansion Rules

Technique builders append resources and passes to the generic graph.

WBOIT:

1. ensure the opaque pass writes the final target and opaque depth;
2. add accumulation and reveal textures;
3. add transparent accumulation pass reading opaque depth;
4. add resolve pass sampling accumulation/reveal textures and writing the final target.

Depth peeling:

1. ensure the opaque pass writes the final target and opaque depth;
2. add front/back accumulators and ping/pong peel-depth resources;
3. add initialization and fixed-count peel iteration passes;
4. add composite pass sampling peel accumulators and writing the final target.

SSAO:

1. add or reuse G-buffer depth/normal resources;
2. add SSAO evaluation pass;
3. add optional blur pass;
4. add composite pass.

Screen-space outline:

1. add a panel-local mask or object-id resource;
2. add an outline source pass for selected, hovered, or explicitly highlighted targets;
3. add edge/dilation pass if needed;
4. composite after bloom by default so outlines remain crisp.

Screen-space edge enhancement:

1. reuse G-buffer depth/normal resources when available;
2. add edge mask or composite resource roles;
3. run after SSAO or EDL composite by default;
4. keep panel viewport/scissor boundaries authoritative.

Bloom:

1. sample the resolved panel color;
2. add bright-pass extraction;
3. add separable blur or mip-chain blur resources;
4. composite before outlines.

## Current Material And Capability Policy

Do not assume every material feature has a public object model. The active retained material policy
is an internal compatibility layer split across:

1. `DvzAlphaMode` on visuals;
2. internal `DvzMaterialState` and `DvzSceneMaterialParams` on visuals;
3. `DvzMaterialDesc` and `dvz_visual_set_material()` for typed material fields;
4. `DvzVolumeState`;
5. scale and colormap bindings;
6. family-specific shader, pipeline, and bind descriptors in `src/scene/visual_pipeline.c`.

Technique eligibility must come from explicit visual pass capabilities resolved from visual family,
material state, attributes, alpha mode, and controller mode. New technique conditionals should use
capability checks instead of open-coded visual-family tests.

## Runtime Guardrails

1. Graph resources should drive texture creation and usage flags.
2. Graph passes should drive pass ordering and sampled reads.
3. Descriptor refresh must reuse the existing graph-resource and texture-recreation path.
4. Borrowed canvas frame targets remain borrowed and must not be destroyed by scene or runtime
   technique code.
5. If an effect needs a new DRP2 feature, write the DRP2 spec change and fixtures before lowering
   scene work to backend-specific commands.
6. Keep examples and tests in lockstep with each new technique.
7. Do not hardcode domain semantics such as molecular rendering into core material or technique
   state.

## DRP2 Gaps To Track

The DRP2/vklite path already supports intermediate textures, multi-color render targets, sampled
textures, storage buffers, compute pipelines, color blend state, depth compare/write state, and
multi-pass execution.

Graph-backed techniques still depend on continued hardening of:

1. named depth attachments/resources instead of only implicit transient depth;
2. explicit attachment load/store operations in the command stream;
3. explicit attachment access for depth read, depth write, and read/write usage;
4. texture/resource layout transitions driven from declared graph access;
5. pipeline raster state such as cull mode and front-face winding;
6. validation of pipeline target formats against render-pass attachment formats;
7. capability facts for sampled depth, storage images, required formats, max attachments, blend
   ops, and independent blend.


## Technique Backlog

1. Keep technique-builder cleanup behavior-preserving: do not change graph names, pass order, or
   stream output while only reducing local clutter.
2. Extend visual pass-capability tests as each family joins G-buffer, EDL, SSAO, outline, or other
   screen-space effect paths.
3. Decide whether EDL becomes a generic postprocess that can compose after selected transparent,
   volume, or SSAO branches.
4. Add object-id or mask resources only when outline or selection semantics require them.
5. Keep scalar material modulation for curvature, cavity, accessibility, uncertainty, and similar
   channels deferred until retained scalar slots are represented in material or visual state.
6. Keep full PBR, light objects, shadows, and ray-tracing-forward policies outside this generic
   technique layer until the shared material contract is stable.
