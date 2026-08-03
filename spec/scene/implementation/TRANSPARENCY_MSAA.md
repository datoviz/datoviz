# Transparency And MSAA Implementor Notes

Status: normative implementation contract for graph-backed transparency and multisample antialiasing in the active scene stack. Public alpha-mode semantics remain in [`../semantics/TRANSPARENCY.md`](../semantics/TRANSPARENCY.md); generic graph technique rules remain in [GRAPH_TECHNIQUES.md](GRAPH_TECHNIQUES.md).

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

The scene plans transparency in `transparent_shading` after AO-aware opaque shading and EDL, and before volume, overlay, and presentation work. Opaque visuals must not render after transparent accumulation in the same panel. Panels without transparent visuals allocate no transparent products or passes.

Source-over authored order is preserved independently of technique phase order. Transparent and volume visuals depth-test against compatible opaque surface products where required but do not produce or consume ambient visibility in RC3.

## WBOIT Expansion

WBOIT is the active approximate OIT path. Its graph expansion must:

1. consume the current typed `scene_color` version and compatible `surface_depth`;
2. allocate explicit accumulation and reveal/transmittance textures;
3. add a transparent accumulation pass that reads opaque depth when present;
4. add a resolve pass that samples accumulation textures and produces the successor `scene_color` version;
5. validate floating-point target formats, color attachment count, blending support, and sampled render-target support.

WBOIT products carry explicit premultiplication and transmittance semantics. WBOIT stays explicit in FramePlan and DRP2 streams and never becomes hidden effect-family runtime knowledge.

## Depth Peeling Contract

`DVZ_ALPHA_DEPTH_PEEL` is the explicit higher-quality OIT path. It is appropriate when WBOIT approximation errors are unacceptable and the runtime can afford extra passes.

The fixed dual-depth-peeling expansion is:

1. render opaque scene color and depth first;
2. initialize per-pixel nearest and farthest transparent depth bounds;
3. iteratively peel front and back transparent layers using versioned typed min/max depth products;
4. accumulate peeled front layers front-to-back and back layers back-to-front;
5. composite the accumulated transparent result over the final target;
6. keep all rendering graph-backed and DRP2-lowered.

The native implementation uses this fixed internal contract:

1. `DVZ_SCENE_DEPTH_PEEL_ITERATIONS = 4`;
2. every transparent depth value is finite positive linear view depth reconstructed from the active projection, for both perspective and orthographic cameras;
3. depth bounds are encoded as `(-near, far)` in typed `transparent_peel_depth` products, with `(-1, -1)` denoting no valid layer;
4. each iteration samples the preceding depth-product version and writes a distinct successor version, so ping/pong is an optional physical aliasing decision rather than semantic identity;
5. front and back branches are typed `transparent_accumulation` products stored as `VK_FORMAT_R16G16B16A16_SFLOAT`;
6. the three MRTs remain front compound, back compound, and depth bounds: each accumulation compound stores premultiplied RGB and accumulated opacity in alpha, and `1 - alpha` is the corresponding branch transmittance;
7. init explicitly clears both accumulation compounds and the first depth bounds, every iteration explicitly `LOAD`s the prior front/back compounds and clears the successor bounds, and all three outputs use explicit `STORE`;
8. composite samples only the final front/back typed versions and explicitly `LOAD`s the current `scene_color` attachment before producing its successor;
9. iteration shaders sample peel bounds from bind group set 3 so common, material/image/volume, and scene-occlusion bindings keep their existing set positions;
10. noncontiguous peel runs are separate technique instances with independent product versions and runtime bindings, while their init/iteration/composite bundles remain in authored transparency order.

The opaque `D32_SFLOAT` attachment used solely for raster depth testing is explicit physical raster state, not a semantic `surface_depth` consumer. Source-over, WBOIT, and depth peeling share the panel forward-depth attachment when opaque occlusion is required; no fake sampled linear-depth edge is added for this hardware test.

EDL uses a separate typed contract: opaque shading writes `scene_color` and standalone linear-view `surface_depth` as color MRTs, while ordinary forward `D32_SFLOAT` remains the hardware depth attachment. The EDL fullscreen pass samples the typed color and R32 depth products and presentation consumes the resulting scene-color version.

## Depth Peeling Runtime Requirements

Depth peeling relies on:

1. graph-created intermediate textures;
2. multiple color attachments;
3. sampled texture bind groups;
4. per-pipeline raster state;
5. explicit color blending;
6. graph attachment load/store handling;
7. graph-access-driven ordering and layout transitions.

Runtime validation rejects sampled reads from a texture that is written in the same pass, pipeline formats that do not match graph attachments, and mismatched pass dependencies.

Full trace diagnostics expose typed product versions, technique-instance identity, declared dependencies, and scoped resource identities. Trace normalization is generic: it normalizes declared numeric and scope identifiers without interpreting effect names, pass labels, or resource suffixes.

## MSAA Contract

MSAA is a general render-target and pipeline feature, not a sphere-only special case.

The durable split is:

1. scene or panel technique state requests a sample count;
2. FramePlan graph resources declare multisampled color/depth attachments where needed;
3. DRP2 and vklite create compatible multisampled targets and pipelines;
4. visuals that produce analytic coverage may opt into alpha-to-coverage.

MSAA is pass-wide. Triangle mesh edges, primitive edges, and compatible line/path rasterization benefit automatically when rendered into a multisampled pass. Shader-generated silhouettes such as sphere impostors, markers, glyphs, and future analytic shapes need alpha-to-coverage or another sample-aware coverage strategy.

Alpha-to-coverage is visual or material specific:

1. sphere impostors should output analytic edge coverage;
2. marker, glyph, and path shaders can later output implicit-shape coverage;
3. ordinary opaque meshes do not need alpha-to-coverage;
4. transparent WBOIT and depth-peel paths should not inherit alpha-to-coverage automatically.

## MSAA FramePlan Rules

Every product declares its sample domain and resolve policy. Graph texture resources carry the resolved concrete sample count. Rules:

1. default sample count is `1`;
2. color and depth attachments in one render pass must use the same sample count;
3. sampled post-process inputs should remain single-sample unless the shader explicitly samples
   multisampled images;
4. multisampled products consumed by single-sample work require an explicit product-specific resolve;
5. linear scene color uses a compatible linear-color resolve;
6. linear surface depth selects the nearest valid covered surface under the declared depth convention;
7. surface normal belongs to the selected surface sample, or uses an explicitly declared coverage-weighted reconstruction followed by normalization;
8. surface coverage resolves to the declared covered fraction or binary winning-surface validity;
9. object IDs select one winning covered sample and are never averaged, interpolated, or normalized-filtered;
10. ambient visibility is normally evaluated after coherent surface resolve.

Physical graph shape for a normal scene color pass:

```text
opaque_msaa_color: COLOR_ATTACHMENT, sample_count = N
opaque_msaa_depth: DEPTH_ATTACHMENT, sample_count = N
rt:                COLOR_ATTACHMENT | SAMPLED | COPY_SRC, sample_count = 1

opaque pass:
  color attachment = opaque_msaa_color
  color resolve    = rt
  depth attachment = opaque_msaa_depth
```

Graph-backed effects such as EDL or ambient visibility use multisampled attachments only before the pass that needs geometric edge quality. Downstream consumers use a coherently resolved single-sample surface record unless a later contract deliberately supports multisampled sampling.

## DRP2 And vklite Requirements

DRP2 must carry explicit multisample state:

1. texture creation carries `sample_count`;
2. render-pass color attachments may carry a resolve texture id and resolve mode;
3. depth attachments carry the same sample count as color attachments for a pass;
4. pipeline creation carries rasterization sample count and alpha-to-coverage enable;
5. validation rejects mismatched attachment and pipeline sample counts.

The vklite runtime must:

1. create multisampled images for graph resources with sample count greater than one;
2. configure dynamic rendering with resolve image views for color attachments;
3. create multisampled depth attachments where required;
4. configure graphics pipelines with the same sample count;
5. enable alpha-to-coverage only for pipelines whose visual/material descriptor requests it.

Serialization and fixture output include sample count and resolve attachment ids.

## Sphere Policy

Opaque sphere impostors should not use ordinary source-over blending for antialiased edges. With
MSAA available, the preferred path is:

1. opaque depth writes;
2. analytic coverage from the fragment shader;
3. alpha-to-coverage in the pipeline;
4. no source-over blending for ordinary opaque sphere rendering.

This avoids overlap halos where depth and color coverage disagree. Sphere surface capture must use the analytic ray hit, corrected fragment depth, generated normal, and coverage belonging to the same visible fragment.

## Validation Expectations

Focused coverage should include:

1. depth-peel typed graph resources for a fixed iteration count;
2. versioned depth and accumulation predecessor/successor relationships;
3. DRP2 command order for multi-iteration peeling;
4. WBOIT and depth-peel format/capability diagnostics;
5. sample-count serialization and semantic validation;
6. multisampled graph texture creation and resolve lowering;
7. alpha-to-coverage pipeline state for opaque sphere impostors;
8. resize smoke tests for sampled intermediate descriptors.

## Completed MSAA Boundary

FramePlan, DRP2, vklite, scene pipelines, and public panel state carry explicit sample counts and resolve targets. `DvzMsaaDesc` and `dvz_panel_set_msaa()` are the public control surface. Product-specific resolve semantics remain mandatory; later visual families may add analytic coverage, but they may not introduce sphere-only multisample ownership or implicit averaging of semantic surface products.
