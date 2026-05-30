# Transparency And MSAA Implementor Notes

Status: implementation-facing notes for graph-backed transparency and multisample antialiasing in
the active scene stack. Public alpha-mode semantics remain in
[`../semantics/TRANSPARENCY.md`](../semantics/TRANSPARENCY.md); generic graph technique rules remain
in [GRAPH_TECHNIQUES.md](GRAPH_TECHNIQUES.md).

## Scope

Transparency and MSAA must use the active route:

```text
retained scene state
  -> technique planning
  -> FramePlan graph resources and passes
  -> DRP2 command stream
  -> vklite/canvas runtime
```

Do not add a parallel renderer, scene-private Vulkan path, or public framegraph API for these lanes.

## Transparency Contract

Visual alpha mode selects the transparent rendering path. Per-item or material alpha controls
opacity, but it does not by itself select WBOIT, depth peeling, or source-over blending.

Installed modes:

1. `DVZ_ALPHA_OPAQUE`;
2. `DVZ_ALPHA_BLENDED`;
3. `DVZ_ALPHA_WBOIT`;
4. `DVZ_ALPHA_DEPTH_PEEL`;
5. `DVZ_ALPHA_MASK`.

The scene plans transparent passes after opaque passes. Opaque visuals must not render after a
transparent accumulation stage in the same panel. Panels without transparent visuals should not pay
for transparent accumulation, peeling, or resolve passes.

## WBOIT Expansion

WBOIT is the active approximate OIT path. Its graph expansion should:

1. ensure the opaque pass writes the final target and opaque depth;
2. allocate explicit accumulation and reveal/transmittance textures;
3. add a transparent accumulation pass that reads opaque depth when present;
4. add a resolve pass that samples accumulation textures and writes the final target;
5. validate floating-point target formats, color attachment count, blending support, and sampled
   render-target support.

WBOIT should stay explicit in FramePlan and DRP2 streams. It should not become hidden runtime
knowledge below the scene contract.

## Depth Peeling Contract

`DVZ_ALPHA_DEPTH_PEEL` is the explicit higher-quality OIT path. It is appropriate when WBOIT
approximation errors are unacceptable and the runtime can afford extra passes.

The current scene path uses the fixed internal graph contract below, but the remaining correctness
target for real dual depth peeling is:

1. render opaque scene color and depth first;
2. initialize per-pixel nearest and farthest transparent depth bounds;
3. iteratively peel front and back transparent layers using ping-pong min/max depth textures;
4. accumulate peeled front layers front-to-back and back layers back-to-front;
5. composite the accumulated transparent result over the final target;
6. keep all rendering graph-backed and DRP2-lowered.

Recommended graph resources per panel:

1. `<panel>.peel.depth_minmax_ping`;
2. `<panel>.peel.depth_minmax_pong`;
3. `<panel>.peel.front_accum`;
4. `<panel>.peel.back_accum`;
5. optional per-iteration temporary front/back color targets;
6. `<panel>.depth.opaque` when transparent visuals depth-test against opaque visuals.

Recommended pass structure:

1. `opaque`: writes `rt` and optional opaque depth;
2. `peel.init`: initializes min/max transparent depth and clears accumulators;
3. `peel.iter.N`: reads previous min/max, peels the next front/back pair, writes next min/max, and
   accumulates colors;
4. `peel.composite`: samples front/back accumulators and writes `rt`.

The first real dual-depth-peeling implementation can use a fixed iteration count such as `4` or
`8`. A retained quality descriptor can come after correctness is stable.

The first native implementation uses this fixed internal contract:

1. `DVZ_SCENE_DEPTH_PEEL_ITERATIONS = 4`;
2. depth bounds use normalized Vulkan `gl_FragCoord.z`, where smaller values are nearer;
3. peel intermediates use `VK_FORMAT_R16G16B16A16_SFLOAT`;
4. `front_accum` and `back_accum` are persistent per-frame accumulators;
5. `depth_minmax_ping` and `depth_minmax_pong` are ping-ponged by iteration;
6. depth bounds are encoded as `(-near, far)` and reduced with max blending in the RG channels;
7. init writes initial accumulators and the first min/max bounds;
8. each `peel.iter.N` samples the previous min/max bounds and writes the opposite min/max target;
9. composite samples only `front_accum` and `back_accum`;
10. iteration shaders sample peel resources from bind group set 3 so common, material/image/volume,
   and scene-occlusion bindings keep their existing set positions.

## Depth Peeling Runtime Requirements

Depth peeling relies on:

1. graph-created intermediate textures;
2. multiple color attachments;
3. sampled texture bind groups;
4. per-pipeline raster state;
5. explicit color blending;
6. graph attachment load/store handling;
7. graph-access-driven ordering and layout transitions.

Runtime validation should reject sampled reads from a texture that is written in the same pass,
pipeline formats that do not match graph attachments, and mismatched pass dependencies.

Trace diagnostics should expose peel iteration count, ping/pong resource ids, and sampled
dependency ids when `DVZ_DRP2_TRACE=full` is enabled.

## MSAA Contract

MSAA is a general render-target and pipeline feature, not a sphere-only special case.

The durable split is:

1. scene or panel technique state requests a sample count;
2. FramePlan graph resources declare multisampled color/depth attachments where needed;
3. DRP2 and vklite create compatible multisampled targets and pipelines;
4. visuals that produce analytic coverage may opt into alpha-to-coverage.

MSAA is pass-wide. Triangle mesh edges, primitive edges, and compatible line/path rasterization
benefit automatically when rendered into a multisampled pass. Shader-generated silhouettes such as
sphere impostors, markers, glyphs, and future analytic shapes need alpha-to-coverage or another
sample-aware coverage strategy.

Alpha-to-coverage is visual or material specific:

1. sphere impostors should output analytic edge coverage;
2. marker, glyph, and path shaders can later output implicit-shape coverage;
3. ordinary opaque meshes do not need alpha-to-coverage;
4. transparent WBOIT and depth-peel paths should not inherit alpha-to-coverage automatically.

## MSAA FramePlan Rules

Graph texture resources carry a sample count. Rules:

1. default sample count is `1`;
2. color and depth attachments in one render pass must use the same sample count;
3. sampled post-process inputs should remain single-sample unless the shader explicitly samples
   multisampled images;
4. multisampled color outputs need a single-sample resolve target before presentation or ordinary
   post-processing.

Example graph shape for a normal scene color pass:

```text
opaque_msaa_color: COLOR_ATTACHMENT, sample_count = N
opaque_msaa_depth: DEPTH_ATTACHMENT, sample_count = N
rt:                COLOR_ATTACHMENT | SAMPLED | COPY_SRC, sample_count = 1

opaque pass:
  color attachment = opaque_msaa_color
  color resolve    = rt
  depth attachment = opaque_msaa_depth
```

Graph-backed effects such as EDL or SSAO should use multisampled attachments only before the pass
that needs geometric edge quality. Downstream postprocess passes should consume resolved
single-sample color, depth, and normal textures unless a later design deliberately supports
multisampled sampling.

## DRP2 And vklite Requirements

DRP2 must carry explicit multisample state:

1. texture creation carries `sample_count`;
2. render-pass color attachments may carry a resolve texture id and resolve mode;
3. depth attachments carry the same sample count as color attachments for a pass;
4. pipeline creation carries rasterization sample count and alpha-to-coverage enable;
5. validation rejects mismatched attachment and pipeline sample counts.

The vklite runtime should:

1. create multisampled images for graph resources with sample count greater than one;
2. configure dynamic rendering with resolve image views for color attachments;
3. create multisampled depth attachments where required;
4. configure graphics pipelines with the same sample count;
5. enable alpha-to-coverage only for pipelines whose visual/material descriptor requests it.

Serialization and fixture output should include sample count and resolve attachment ids before
examples rely on live replay.

## Sphere Policy

Opaque sphere impostors should not use ordinary source-over blending for antialiased edges. With
MSAA available, the preferred path is:

1. opaque depth writes;
2. analytic coverage from the fragment shader;
3. alpha-to-coverage in the pipeline;
4. no source-over blending for ordinary opaque sphere rendering.

This avoids overlap halos where depth and color coverage disagree.

## Validation Expectations

Focused coverage should include:

1. depth-peel graph resources for a fixed iteration count;
2. ping/pong read/write alternation;
3. DRP2 command order for multi-iteration peeling;
4. WBOIT and depth-peel format/capability diagnostics;
5. sample-count serialization and semantic validation;
6. multisampled graph texture creation and resolve lowering;
7. alpha-to-coverage pipeline state for opaque sphere impostors;
8. resize smoke tests for sampled intermediate descriptors.

## Remaining MSAA Pressure

The next MSAA slice should keep the work broad enough to avoid a sphere-only path:

1. serialize sample count and resolve attachment ids before example replay depends on them;
2. validate sample-count compatibility at the FramePlan, DRP2, and pipeline layers;
3. cover vklite multisampled graph textures and color resolves in focused tests;
4. gate alpha-to-coverage on runtime capability and visual/material descriptor state;
5. keep opaque sphere impostors on depth-writing alpha-to-coverage instead of source-over edges;
6. expose public panel-level state through `DvzMsaaDesc` and `dvz_panel_set_msaa()` only after the
   graph/runtime contract is proven;
7. add a GLFW sphere comparison with sample-count controls once diagnostics can show the active
   sample count and resolve route.
