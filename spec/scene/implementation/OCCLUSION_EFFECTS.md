# Occlusion Effects Implementor Notes

Status: implementation-facing notes for graph-backed occlusion techniques in the active scene
stack. This file records durable implementation contracts for SSAO, scene occlusion, and volume
occlusion. Current execution order belongs in `agents/now/STATUS.md`.

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

## SSAO Pipeline Contract

SSAO is a panel-local technique. It is retained on panel technique state, not on individual visuals,
because it depends on composed panel depth and normal information.

The minimum graph-backed SSAO pipeline is:

```text
GBUFFER PASS
  outputs:
    base color texture
    normal texture
    linear depth texture
  depth:
    graph-declared depth attachment for depth testing

SSAO PASS
  inputs:
    normal texture
    linear or reconstructable depth texture
    kernel and parameter data
  output:
    raw ambient-occlusion texture

OPTIONAL BLUR PASS
  inputs:
    raw AO texture
    depth texture
    normal texture
  output:
    blurred AO texture

COMPOSITE PASS
  inputs:
    base color texture
    raw or blurred AO texture
  output:
    final panel color target
```

Recommended graph resources for a panel:

1. `<panel>.ssao.color`;
2. `<panel>.ssao.normal`;
3. `<panel>.ssao.linear_depth` or equivalent sampled depth source;
4. `<panel>.ssao.depth`;
5. `<panel>.ssao.ao`;
6. `<panel>.ssao.blur`, only when blur is enabled;
7. borrowed final target `rt`.

Recommended formats:

1. color: `RGBA8_UNORM` unless a higher precision color target is required;
2. normal: `RGBA16_SFLOAT`;
3. linear depth: `R32_SFLOAT`;
4. depth attachment: `D32_SFLOAT`;
5. AO: `R8_UNORM` or `R16_SFLOAT`, chosen deliberately and covered by fixtures.

For the first robust path, use primitive, mesh, and sphere visuals that can produce valid normals
and depth. Points, images, text, volume, and overlay/fixed-controller visuals can remain outside
the SSAO G-buffer until their participation policy is explicit.

## SSAO Quality Contract

The quality target is stable, smooth, depth-aware SSAO suitable for dense scientific scenes:

1. sphere clouds and mesh cavities show contact or cavity darkening;
2. occlusion strength does not collapse merely because the camera zooms or the scene moves deeper
   in clip space;
3. blur smooths sample noise without bleeding across depth or normal discontinuities;
4. controls have predictable units and effect.

Preferred shader model:

1. reconstruct view-space position from depth using inverse projection and viewport coordinates;
2. decode or read the current normal;
3. build a tangent frame around the normal;
4. rotate a hemisphere sample kernel per pixel using a deterministic hash or small noise texture;
5. project sample positions back to screen coordinates;
6. compare sampled scene depth or reconstructed position against the projected sample depth;
7. accumulate occlusion with range falloff and bias.

Parameter semantics:

1. `radius`: view-space sampling radius;
2. `bias`: minimum view-space separation before a sample can occlude;
3. `strength`: final occlusion contrast/intensity;
4. `sample_count`: number of hemisphere samples, preferably one of `8`, `16`, `32`, or `64`;
5. `blur`: optional bilateral smoothing.

Bilateral blur should use AO, depth, and normal inputs. It must avoid large depth and normal
discontinuities and should be a separate graph pass:

```text
GBUFFER -> SSAO_RAW -> SSAO_BILATERAL_BLUR -> COMPOSITE
```

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

Use per-panel graph resources:

1. `<panel>.scene_occlusion.mesh_depth`;
2. `<panel>.scene_occlusion.volume_depth`;
3. `<panel>.scene_occlusion.depth`.

The merged depth texture should use `R32_SFLOAT` and store normalized or linear front depth. Linear
view depth is preferred for stable soft-edge behavior.

Frame graph integration:

1. detect active occluders and occludees per panel;
2. emit occlusion prepass resources and passes before regular rendering;
3. merge mesh and volume depth sources when more than one producer is active;
4. add graph reads from occluded render passes to `<panel>.scene_occlusion.depth`;
5. preserve normal render ordering: opaque, blended, WBOIT, depth peeling, postprocess.

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
3. enough color attachments for the SSAO G-buffer path;
4. sampled texture bindings and bind groups for fullscreen and visual-family consumers;
5. descriptor refresh after graph texture recreation;
6. explicit diagnostics when required formats or attachment counts are unavailable.

Sampler configuration remains a specific risk for SSAO noise textures. If the first SSAO quality
upgrade avoids nearest/repeat noise sampling, record that as a deliberate simplification; otherwise
extend DRP2 sampler descriptors narrowly and add fixtures.

## Validation Expectations

Focused tests should cover:

1. graph-shape resources and dependencies for SSAO and scene/volume occlusion;
2. default-off behavior;
3. opt-in DRP2 stream shape;
4. graph texture recreation with stable ids and live bind groups;
5. nonblank GPU/offscreen smoke where the environment supports it;
6. disabled-versus-enabled image differences;
7. multi-panel viewport/scissor correctness;
8. shader/pipeline keys differing for occluded versus non-occluded variants.


## Quality And Consumer Backlog

SSAO quality work should stay on the same graph path:

1. add inverse projection or reconstruction parameters to SSAO uniforms;
2. reconstruct view-space position from depth;
3. replace fixed 2D kernels with normal-oriented hemisphere kernels;
4. add deterministic per-pixel kernel rotation;
5. harden bilateral blur as an explicit optional graph pass;
6. expose radius, bias, strength, sample count, blur, and quality preset controls in the tuning
   example.

Non-volume occlusion consumers should start with primitive or unlit mesh. Contract tests should
verify the volume-occlusion pass, depth resource, graph read, selected shader/pipeline variant, and
that the embedded draw is absent from the occlusion prepass unless it is the volume occluder. Pixel
tests should compare disabled and enabled captures.
