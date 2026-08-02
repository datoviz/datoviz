# Graph-Backed Technique Implementor Notes

Status: normative implementation contract for scene techniques that compose retained panel state into semantic render products and lower them through FramePlan, DRP2, and vklite. Public semantics remain in the scene semantics documents.

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

## Approved v0.4 Composition Model

The scene composer owns one deterministic transaction per panel:

```text
panel visuals + retained technique state + target capabilities
  -> visual-layer classification
  -> required semantic product closure
  -> compatible producer and capability resolution
  -> technique expansion
  -> graph validation and transient lifetime analysis
  -> immutable FramePlan
```

Every draw belongs to exactly one semantic layer: `surface_opaque`, `surface_masked`, `transparent`, `volume`, `overlay`, or `query`. Authored visual order remains independent from technique phase order and must be preserved within layers where source-over, annotation, label, or fixed-controller order is visible.

The default semantic phase order is `surface_capture`, `surface_analysis`, `opaque_shading`, `surface_postprocess`, `transparent_shading`, `volume_shading`, `scene_postprocess`, `overlay`, `presentation`, and independent `query`. GTAO evaluation and reconstruction run in `surface_analysis`. Product dependencies refine this partial order; effect names, resource suffixes, insertion order, and work labels do not.

A technique is an immutable internal descriptor declaring typed inputs and outputs, phase constraints, participating layers, capability requirements and fallbacks, and a declarative pass expansion. Technique activation remains retained panel state and default-off unless it is part of baseline opaque rendering.

## Semantic Product And Resource Model

Render products and physical graph resources are separate contracts. A product defines meaning and legal producer-consumer relationships; a resource defines allocation, format realization, access, and lifetime. Format compatibility alone never proves semantic compatibility.

The minimum RC3 product descriptor carries:

```text
RenderProductContract {
    id: typed plan-local product version
    kind: scene_color | surface_depth | surface_normal | surface_coverage | object_id |
          ambient_visibility | scene_occlusion_depth | transparent_accumulation |
          volume_first_hit_depth | presentation_color
    domain: panel | view | scene | query | presentation
    extent: absolute | panel_relative | source_relative
    format_class
    sample_domain_and_resolve_policy
    coordinate_space_and_encoding
    alpha_coverage_and_validity
    required_accesses
    lifetime
    producer_and_consumers
}
```

`surface_depth`, `surface_normal`, and `surface_coverage` form one coherent surface record for the same winning opaque or masked fragment. A consumer may not select them independently. Product IDs are typed and plan-local; human names remain diagnostics only.

When `ambient_visibility` is enabled, its coherent surface record is produced by a `surface_capture` prepass, ambient visibility is evaluated and reconstructed, and `opaque_shading` then redraws eligible geometry while consuming that visibility. An opaque-shading MRT may produce surface products only for consumers that do not feed the same shading pass, such as EDL when AO is disabled; the composer must reject a product cycle rather than disguise it through attachment aliasing.

Every product is panel-local by default and carries panel pixel origin, local extent, render scale, rounding policy, and local-to-target transform. Shaders sample panel-local products in local coordinates. Physical pooling or aliasing is permitted only for non-overlapping lifetimes with compatible format, extent, samples, access, and ownership.

FramePlan validation rejects missing or ambiguous producers, incompatible product semantics, cross-panel reuse, phase cycles, read-before-produce, undefined background reads, implicit sample changes, and incompatible product-resource realization.

A color-transforming technique consumes one typed `scene_color` version and produces a distinct successor version. Versioned products make ordering and ownership explicit; a pass never reads and writes the same semantic product version as hidden feedback.

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

Pass tags, roles, and labels are diagnostic only. Declarative work records the pipeline-provider key, work class, product bindings, attachments, clear/load/store policy, sample and resolve policy, viewport and panel-local transform, ordered draw filter, and diagnostics. Generic runtime lowering must not recover any of those facts by scanning a name or technique-family enum.

## Technique Expansion Rules

Technique descriptors expand semantic products into the generic graph.

WBOIT:

1. consume opaque `scene_color` and compatible `surface_depth`;
2. produce explicit transparent accumulation and transmittance products;
3. preserve authored transparent ordering where the selected algorithm requires it;
4. resolve within `transparent_shading` before volume and overlay work.

Depth peeling:

1. consume opaque `scene_color` and compatible `surface_depth`;
2. declare every peel depth, color, and transmittance product explicitly;
3. retain fixed iteration and ping-pong semantics through product dependencies;
4. resolve within `transparent_shading` without contributing to or consuming AO in RC3.

SSAO:

1. consume the coherent `surface_depth`, `surface_normal`, and `surface_coverage` record;
2. run in `surface_analysis` and produce deterministic `ambient_visibility` with declared view-space scale and reconstruction policy;
3. reconstruct or denoise with depth-aware, normal-aware, projection-derived support;
4. bind ambient visibility into eligible opaque or masked material lighting rather than compositing black over scene color.

EDL consumes canonical `surface_depth` plus AO-aware opaque `scene_color` and produces `scene_color` in `surface_postprocess`, before transparency and volume composition.

Screen-space outline:

1. add a panel-local mask or object-id resource;
2. add an outline source pass for selected, hovered, or explicitly highlighted targets;
3. add edge/dilation pass if needed;
4. composite after bloom by default so outlines remain crisp.

Screen-space edge enhancement:

1. reuse G-buffer depth/normal resources when available;
2. add edge mask or composite products;
3. run in `scene_postprocess` after transparent and volume composition by default;
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

1. Product-linked graph resources drive texture creation and usage flags.
2. Product dependencies and graph passes drive pass ordering and sampled reads.
3. Descriptor refresh must reuse the existing graph-resource and texture-recreation path.
4. Borrowed canvas frame targets remain borrowed and must not be destroyed by scene or runtime
   technique code.
5. If an effect needs a new DRP2 feature, write the DRP2 spec change and fixtures before lowering
   scene work to backend-specific commands.
6. Keep examples and tests in lockstep with each new technique.
7. Do not hardcode domain semantics such as molecular rendering into core material or technique
   state.
8. Do not allocate effect-family target buckets or match `(panel_id, work_label, ordinal)` in generic runtime code.
9. Do not keep legacy role-driven and product-driven composition as parallel completion paths.

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


## Migration Boundary

The current role enums, string resource IDs, effect graph builders, target buckets, and trace normalization remain temporary legacy implementation details only while R1-R9 migrate their consumers. They are not architectural authority and must be deleted by the final migration checkpoint.

Temporal products, public technique plugins, user-editable graphs, a full deferred renderer, display HDR/color management, ray tracing, and broad new effects remain outside RC3. Object-ID outlines and bloom may reuse products later but are not promoted by this refactor.
