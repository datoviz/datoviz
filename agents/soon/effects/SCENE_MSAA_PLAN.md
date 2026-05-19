# Scene MSAA Plan

> **Execution Status**
> - **Status:** `PLANNING NOTE`
> - **Updated on:** `2026-05-16`
> - **Purpose:** define how multisample antialiasing should enter the scene -> FramePlan ->
>   DRP2 -> vklite path without making it sphere-specific or bypassing existing render graph
>   infrastructure.


## Decision

MSAA should be a general render-target and pipeline feature. It should not be implemented as a
sphere-only special case.

The sphere visual exposed why MSAA is needed: shader-generated opaque silhouettes need partial
pixel coverage while keeping coherent depth behavior against overlapping geometry. That problem also
exists for markers, wide paths, glyphs, and any future analytic impostor. The right split is:

1. scene/panel techniques request a sample count for one or more render passes;
2. FramePlan graph resources declare multisampled color/depth attachments where needed;
3. DRP2 and vklite create compatible multisampled targets and pipelines;
4. visuals that produce analytic coverage may opt into alpha-to-coverage.


## Target Semantics

Public or semi-public control should eventually be panel-local:

```c
typedef struct DvzMsaaDesc
{
    bool enabled;
    uint32_t sample_count; // 1, 2, 4, or 8, clamped to device support.
    bool alpha_to_coverage;
} DvzMsaaDesc;

DVZ_EXPORT DvzMsaaDesc dvz_msaa_desc(void);
DVZ_EXPORT int dvz_panel_set_msaa(DvzPanel* panel, const DvzMsaaDesc* desc);
```

The first implementation can keep this internal if public API timing is not settled, but the
behavior should already be represented as panel technique state rather than per-visual state.


## Scope Split

MSAA is pass-wide:

1. triangle mesh edges benefit automatically;
2. primitive edges benefit automatically;
3. lines/paths benefit if their rasterization path uses the multisampled pass;
4. shader-generated silhouettes need alpha-to-coverage or sample-aware discard to see a high-quality
   edge improvement.

Alpha-to-coverage is visual or material specific:

1. sphere impostors should output analytic edge coverage;
2. marker/glyph/path shaders can later output coverage for their implicit shapes;
3. ordinary opaque meshes do not need alpha-to-coverage;
4. transparent WBOIT/depth-peel paths should not inherit alpha-to-coverage automatically.


## FramePlan Graph Changes

Extend graph texture resources with a sample count:

```c
uint32_t sample_count;
```

Rules:

1. default sample count is `1`;
2. color and depth attachments in one render pass must use the same sample count;
3. sampled post-process inputs should remain single-sample unless the shader explicitly samples
   multisampled images;
4. multisampled color outputs need a single-sample resolve target before presentation or ordinary
   post-processing.

For a normal scene color pass with MSAA:

```text
opaque_msaa_color:   COLOR_ATTACHMENT, sample_count = N
opaque_msaa_depth:   DEPTH_ATTACHMENT, sample_count = N
rt:                  COLOR_ATTACHMENT | SAMPLED | COPY_SRC, sample_count = 1

opaque pass:
  color attachment = opaque_msaa_color
  color resolve    = rt
  depth attachment = opaque_msaa_depth
```

For graph-backed techniques such as EDL or SSAO, use multisampled attachments only before the pass
that needs geometric edge quality. Downstream post-process passes should consume resolved
single-sample color/depth/normal textures unless a later design deliberately adds multisampled
sampling support.


## DRP2 Contract Changes

DRP2 needs explicit multisample state in texture and pipeline commands:

1. texture creation carries `sample_count`;
2. render-pass color attachments may optionally carry a resolve texture id and resolve mode;
3. depth attachments carry the same sample count as color attachments for a pass;
4. pipeline creation carries rasterization sample count and alpha-to-coverage enable;
5. validation rejects mismatched attachment/pipeline sample counts.

The existing serialization/recording path should include sample count and resolve attachment ids in
portable JSON before examples rely on live replay.


## vklite Runtime Changes

The vklite backend already exposes lower-level sample-count hooks:

1. `dvz_images_samples()`;
2. `dvz_graphics_multisampling()`;
3. dynamic rendering resolve attachment fields.

The missing work is DRP2 runtime plumbing:

1. create multisampled images for graph resources with sample count greater than one;
2. configure dynamic rendering with resolve image views for color attachments;
3. ensure depth attachments are multisampled when required;
4. configure graphics pipelines with the same sample count;
5. enable alpha-to-coverage only for pipelines whose visual/material descriptor requests it.


## Sphere Policy

The sphere visual should not force color blending for opaque antialiased edges. That creates halos
when spheres overlap because depth and color are no longer describing the same coverage.

With MSAA available, sphere should use:

1. opaque depth writes;
2. analytic coverage in the fragment shader;
3. alpha-to-coverage in the pipeline;
4. no source-over blending for ordinary opaque sphere rendering.

This is the correct long-term solution for high-quality sphere silhouettes.


## Implementation Order

Recommended commits:

1. Add sample-count fields to FramePlan graph resources and DRP2 texture/pipeline descriptors,
   defaulting every existing path to sample count `1`.
2. Add DRP2 semantic validation and JSON/recording support for sample counts.
3. Add vklite runtime creation of multisampled graph textures and pipeline sample-count plumbing.
4. Add color resolve attachment support to graph render-pass lowering.
5. Add panel-local internal MSAA technique state and route a simple opaque scene through
   multisampled color/depth plus resolve.
6. Add alpha-to-coverage capability in visual pipeline descriptors.
7. Enable alpha-to-coverage for opaque sphere impostors when panel MSAA is active.
8. Add a GLFW sphere comparison example control for sample count and, later, alpha-to-coverage.


## Validation

Focused validation:

```text
cmake --build build --target dvztest_drp2 dvztest_scene -j 8
./build/testing/dvztest_drp2 test_drp2
./build/testing/dvztest_scene test_scene_visual_pass_capabilities
./build/testing/dvztest_scene test_scene_sphere_emit_glsl_executes
./build/examples/c/hello_sphere_ssao_glfw 2
git diff --check
```

Broader validation before exposing public API:

```text
just test drp2
just test scene
just spec-check
```

