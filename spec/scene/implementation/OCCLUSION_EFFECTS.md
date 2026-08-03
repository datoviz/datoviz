# Occlusion Effects Implementor Notes

Status: normative implementation contract for graph-backed occlusion techniques in the active scene stack. This file defines durable GTAO, scene-occlusion, and volume-occlusion behavior; release sequencing belongs in `agents/now/STATUS.md`.

These notes refine the shared graph technique contract in
[GRAPH_TECHNIQUES.md](GRAPH_TECHNIQUES.md).

## Scope

Occlusion techniques must use the active route:

```text
retained scene state
  -> technique planning
  -> FramePlan graph resources and passes
  -> DRP2 command stream
  -> vklite/canvas runtime
```

Do not add a parallel renderer, presentation layer, visual-private postprocess path, or ad-hoc
Vulkan path for occlusion features.

## Ambient Visibility Pipeline Contract

Ambient occlusion is a panel-local technique retained on panel state. The RC3 implementation produces a semantic `ambient_visibility` product from the coherent opaque or masked surface record and consumes that product inside eligible material lighting.

The required composition is:

```text
surface_capture
  -> coherent surface_depth + surface_normal + surface_coverage
  -> deterministic GTAO-family evaluation
  -> optional edge-aware reconstruction/denoise
  -> ambient_visibility
  -> opaque_shading material ambient/indirect term
```

This sequence is a required prepass when AO is enabled: eligible opaque and masked geometry is captured first and redrawn for forward opaque shading after visibility exists. Piggybacking the surface record onto the same opaque-shading pass that consumes AO would create a frame-graph cycle and is invalid. Without AO, compatible shading MRT output may serve later consumers such as EDL.

The black source-over composite is invalid and must be removed. Ordinary AO must not darken direct diffuse, specular, emissive, unlit, transparent, volume, overlay, query, or presentation contributions. A debug presentation may display `ambient_visibility` without mutating ordinary lighting.

Opaque and masked visuals participate only when their capture path matches shaded clipping, discard, deformation, culling, depth bias, coverage, generated normal, and corrected fragment depth semantics. Analytic sphere impostors are a required first-class producer and conformance target. Transparent, volume, unlit, overlay, and query work neither produces nor consumes AO in RC3.

`surface_depth`, `surface_normal`, and `surface_coverage` must describe the same winning sample after semantic resolve. Background validity is explicit. Unsupported visual families are diagnosed or excluded; they do not emit approximate normals or depth.

The concrete depth, normal, coverage, and visibility formats are capability-resolved and recorded in diagnostics. A fallback may reduce visibility resolution or quality but may not restore black composition, reinterpret normals or depth, or silently disable enabled AO.

## Ambient Visibility Quality Contract

The RC3 estimator is a deterministic horizon-based view-space method in the GTAO family. It must satisfy:

1. every declared direction or sample remains in the normalization domain, with background and rejected samples contributing unoccluded visibility;
2. radius, thickness, falloff, and bias use view-space or another explicit physical scene scale;
3. fixed inputs use deterministic directions without temporal jitter or visible unmatched per-pixel random rotation;
4. perspective and orthographic reconstruction use declared camera parameters and one canonical linear view-depth convention;
5. visibility is finite and bounded in `[0, 1]` before artistic intensity or contrast mapping;
6. silhouettes, borders, invalid background, alpha-to-coverage, and degenerate projection inputs preserve coherent validity;
7. stationary redraws are stable, while cross-GPU and cross-backend conformance uses documented tolerances rather than bit identity.

Quality selects a fixed complete sampling domain: low uses 2 directions by 3 steps, medium uses 3 by 4, high uses 4 by 6, and ultra uses 6 by 8. Direction `i` uses `theta = pi * (i + 0.5) / direction_count`; every declared step remains in the denominator even when it reaches invalid background, falls outside the panel, or is rejected by thickness and bias. The implementation is full-resolution in RC3. Any future reduced-resolution mode requires depth-aware and normal-aware reconstruction and an explicit product-extent contract.

For a valid center sample, reconstruction yields a view-space position `P` from the canonical positive linear depth and the inverse projection. Each accepted candidate `Q` contributes a bounded horizon term derived from `max(dot(N, normalize(Q - P)), 0)`, view-space falloff, coherent coverage, thickness, and bias. Visibility is `1 - sum(max_horizon_per_ray) / (2 * direction_count)`, clamped to `[0, 1]` before intensity and minimum-visibility mapping. Perspective and orthographic paths use the same positive linear-depth convention; projected sampling radius is derived from the projection matrix and panel-local product extent, not the canvas origin or an unqualified pixel radius.

The bounded denoise is separable and edge-aware. Its support is derived from view-space radius and projection, and its weights reject incompatible canonical depth, signed view normal, and invalid coverage. Background reconstructs to visibility one and must not borrow foreground occlusion across silhouettes or panel borders. A fixed unqualified pixel blur radius is not a public or internal semantic contract.

The public `DvzAoDesc` exposes semantic radius, intensity, thickness, minimum visibility, quality, and debug mode through `dvz_ao_desc()` and `dvz_panel_set_ao()`. Raw sample count, blur enable, pixel blur radius, blur sigma, and kernel arrays are not public controls, and no runtime behavior may depend on the legacy hit-count estimator or fixed-pixel blur.

## Scene Occlusion Model

Generic scene occlusion lets one set of visuals contribute a screen-space occlusion depth field and
another set of visuals sample it. It is a pragmatic scientific-visualization approximation, not a
physically correct unified geometry/volume renderer.

Each visual can opt into two roles:

1. `occluder`: contributes front depth to a per-panel occlusion texture;
2. `occluded`: samples the occlusion texture and attenuates or hides fragments behind it.

Expected behavior:

1. opaque surface shells can hide interior slices or objects;
2. volumes can hide embedded visuals based on first visible volume depth;
3. multiple occluder families can contribute to one nearest-depth field;
4. occluded visuals keep their normal opaque, blended, WBOIT, or depth-peel path and sample
   occlusion as an additional input.

Known limitations:

1. only the nearest occluding layer per pixel is represented;
2. transparent occluders are approximate;
3. scattering, absorption, refraction, and multiple volume layers are not physically modeled;
4. embedded visuals are composited in screen space.

## Scene Occlusion Resources And Passes

The typed panel-local `scene_occlusion_depth` product represents the merged nearest scene-occluder field. It is distinct from `surface_depth` and `volume_first_hit_depth`, even when their physical formats or depth conventions are compatible.

The current physical resource realization may use:

1. `<panel>.scene_occlusion.mesh_depth`;
2. `<panel>.scene_occlusion.volume_depth`;
3. `<panel>.scene_occlusion.depth`.

The `scene_occlusion_depth` product stores the nearest positive finite linear view depth in the same camera-space convention as canonical `surface_depth`. Its validity is carried explicitly by coverage or equivalent product metadata; when coverage is false, the numeric depth texel is ignored. Producers with normalized device depth, reversed depth, or another physical encoding must convert to this canonical value before the nearest-depth merge. The current normalized-depth textures are legacy physical inputs, not an alternative semantic encoding of the product.

Frame graph integration:

1. detect active occluders and occludees per panel;
2. emit occlusion prepass resources and passes before regular rendering;
3. merge mesh and volume depth sources when more than one producer is active;
4. add typed `scene_occlusion_depth` reads to occluded render passes, realized by `<panel>.scene_occlusion.depth` in the current graph;
5. preserve semantic ordering: coherent surface capture, surface analysis, AO-aware opaque shading, EDL, transparent composition, volume composition, scene postprocessing, overlay, then presentation; a transparency technique may internally choose ordinary blending, WBOIT, or depth peeling without changing that semantic order.

The scene-occlusion depth field is distinct from the coherent `surface_depth` product consumed by ambient visibility and EDL. A volume first-hit depth producer may feed scene occlusion, but it does not make volume rendering an AO surface producer.

The system must not route ordinary blended passes through WBOIT target ownership.

## Scene Occlusion Shader Contract

Consumers should use one shader source per visual family and feature variants, not duplicated
`_occluded.frag` files.

Preferred pattern:

```glsl
#ifdef DVZ_SCENE_OCCLUSION
#include "scene_occlusion.glsl"
#endif

void main()
{
    vec4 color = compute_visual_color();
#ifdef DVZ_SCENE_OCCLUSION
    color.a *= scene_occlusion_visibility(...);
#endif
    outColor = color;
}
```

The same feature bit must drive shader compilation, pipeline layout creation, graph reads, and bind
group binding. Pipeline and shader keys must include the feature state.

Recommended occlusion bind group contents:

```glsl
layout(set = N, binding = 0) uniform sampler sceneOcclusionSampler;
layout(set = N, binding = 1) uniform texture2D sceneOcclusionDepth;
layout(set = N, binding = 2) uniform SceneOcclusionParams
{
    vec4 params; // depth_bias, soft_edge, hidden_alpha, enabled
} sceneOcclusion;
```

## Volume Occlusion Contract

Volume occlusion is the first concrete scene-occlusion producer/consumer path. It lets selected
visuals be partially hidden by dense volume material in front of them:

```text
front volume density attenuates embedded visual fragments
embedded visual remains visible where front volume is sparse
back volume stays contextual through normal volume compositing
```

Use a transient sampled color texture rather than a hardware depth attachment.

Recommended first format:

```text
R32_SFLOAT
```

Stored value:

```text
0.0 = no meaningful front volume hit for this pixel
>0  = first-hit linear view depth
```

The volume prepass raymarches front-to-back, accumulates alpha using active volume transfer
controls, and writes the first ray position where accumulated alpha crosses a threshold. Embedded
visual shaders sample that texture in screen space, compare fragment linear view depth against the
stored volume depth, and apply soft attenuation.

Volume occlusion defaults should prefer soft attenuation, not hard discard. Hard discard is useful
as a debug mode but is brittle for normal viewing.

Initial public or internal descriptor shape:

```c
typedef struct DvzVolumeOcclusionDesc
{
    bool enabled;
    float alpha_threshold;
    float fade_distance;
    float occluded_alpha;
} DvzVolumeOcclusionDesc;
```

Suggested default values:

```text
enabled = true
alpha_threshold = 0.08
fade_distance = 0.08
occluded_alpha = 0.20
```

Validation rules:

1. the panel occluder must be a volume visual;
2. the occluder must belong to the same panel;
3. only one volume occluder per panel is supported initially;
4. unsupported target visual families should return an error, warning, or explicit no-op according
   to the current scene API style.

## Volume Occlusion Rollout

Do not update every visual family in the first implementation.

Recommended target order:

1. volume slice visual, because it is the highest-value Allen example target and closest to the
   existing volume shader family;
2. primitive or unlit mesh visual;
3. point visual;
4. image or textured quad visual.

For primitive and mesh consumers, `volume_occluded = true` should imply:

1. the draw samples the panel volume-occlusion depth resource;
2. the draw needs the occlusion bind layout;
3. the shader feature mask selects a volume-occluded variant;
4. the graph pass read is recorded only when a panel volume occluder exists;
5. the pipeline layout ordering remains compatible with material/image/scene occlusion sets.

## DRP2 And Capability Requirements

Required capabilities:

1. graph-created sampled render targets;
2. `R32_SFLOAT` or chosen depth/occlusion color target support;
3. enough compatible attachments for the coherent surface-record capture path;
4. sampled texture bindings and bind groups for fullscreen and visual-family consumers;
5. descriptor refresh after graph texture recreation;
6. explicit diagnostics when required formats or attachment counts are unavailable.

The deterministic GTAO-family estimator does not use the legacy random-rotation noise texture. Any later stochastic or temporal estimator requires a separately declared determinism, sampling, and history contract.

## Validation Expectations

Focused tests should cover:

1. graph-shape products, resources, and dependencies for ambient visibility and scene/volume occlusion;
2. default-off behavior;
3. opt-in DRP2 stream shape;
4. graph texture recreation with stable ids and live bind groups;
5. deterministic numeric or image metrics for zoom sweeps, isolated and dense sphere impostors, meshes, silhouettes, perspective and orthographic projection, stationary redraws, and background or panel borders;
6. MSAA off/on and alpha-to-coverage coherence for depth, normal, coverage, and visibility;
7. unequal multi-panel origin/extent, resize, HiDPI, reduced-resolution reconstruction, disabled, and explicit fallback paths;
8. proof that direct, specular, emissive, unlit, transparent, volume, overlay, query, and scalar color-mapping contributions remain unchanged by ordinary AO;
9. shader/pipeline keys differing for occluded versus non-occluded variants.


## Quality And Consumer Backlog

The old hit-count-normalized SSAO kernel, random rotation, fixed-pixel blur, optional blur toggle, and black-overlay composite have no conforming runtime role. The semantic AO product path above and the public AO API are authoritative.

Non-volume occlusion consumers should start with primitive or unlit mesh. Contract tests should
verify the volume-occlusion pass, depth resource, graph read, selected shader/pipeline variant, and
that the embedded draw is absent from the occlusion prepass unless it is the volume occluder. Pixel
tests should compare disabled and enabled captures.
