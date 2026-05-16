# Screen-Space Scene Occlusion Plan

## Goal

Add a generic screen-space occlusion system so ordinary visuals can be visually embedded inside
volumes and surface shells without relying on volume-specific shader paths.

This is a pragmatic scientific-visualization approximation, not a physically correct unified
volume/geometry renderer. It should make cases such as Allen atlas shells hiding internal slices,
small meshes embedded in translucent volumes, and image planes inside volumes look coherent.

## Non-goals

- Do not implement physically based volume/geometry integration.
- Do not require every visual to use one monolithic renderer.
- Do not duplicate GLSL files for occluded and non-occluded variants.
- Do not make WBOIT behave like true opaque rendering.

## Visual model

Each visual can independently opt into two roles:

- `occluder`: contributes front depth to a per-panel occlusion texture.
- `occluded`: samples that occlusion texture and attenuates or hides its fragments behind it.

Examples:

- Allen atlas mesh: `occluder = true`.
- Full brain volume: `occluder = true`.
- Slice plane: `occluded = true`.
- Embedded cube/image/mesh: `occluded = true`.

## Expected behavior

The system should handle:

- Opaque surface shells hiding interior slices or objects.
- Volumes hiding embedded visuals based on first visible volume depth.
- Multiple occluder families contributing to a single nearest-depth field.
- Occluded visuals rendering through their normal opaque/blended/WBOIT/depth-peel path, with final
  alpha attenuated by scene occlusion.

Known limitations:

- Only the nearest occluding layer per pixel is represented.
- Transparent occluders are approximate.
- Multiple volume layers, scattering, absorption, and refraction are not physically modeled.
- Embedded visuals are composited in screen space rather than raymarched as part of the volume.

## Architecture

### 1. Scene state

Add retained scene/panel state for scene occlusion.

Suggested structures:

```c
typedef struct DvzSceneOcclusionDesc
{
    bool enabled;
    float depth_bias;
    float soft_edge;
    float hidden_alpha;
} DvzSceneOcclusionDesc;
```

Visual flags:

```c
bool scene_occluder;
bool scene_occluded;
```

Public API candidates:

```c
int dvz_visual_set_scene_occluder(DvzVisual* visual, bool enabled);
int dvz_visual_set_scene_occluded(DvzVisual* visual, bool enabled);
int dvz_panel_set_scene_occlusion(DvzPanel* panel, const DvzSceneOcclusionDesc* desc);
```

### 2. Occlusion depth resources

Use per-panel frame-graph resources:

- `panel.scene_occlusion.mesh_depth`
- `panel.scene_occlusion.volume_depth`
- `panel.scene_occlusion.depth`

Initial resource format:

- `R32_SFLOAT`
- stores normalized or linear front depth
- cleared to far depth

Preferred long-term representation:

- linear view depth for stable soft-edge behavior
- helper functions for converting fragment depth consistently

### 3. Occluder prepasses

Mesh/primitive/sphere occluders:

- Render a prepass before normal visual passes.
- Use depth test/write so the nearest surface wins.
- Fragment shader writes front depth to `R32_SFLOAT`.

Volume occluders:

- Reuse the current raymarched front-depth logic.
- Convert it from volume-specific panel state into the generic scene-occlusion producer path.

### 4. Depth merge

If more than one occluder depth source exists, merge to one texture:

```glsl
scene_depth = min(mesh_depth, volume_depth);
```

First implementation can skip a merge pass if only one producer is active. If both mesh and volume
are active, prefer a merge pass rather than making every consumer sample multiple textures.

### 5. Occlusion consumer bind group

Add an optional common occlusion bind group for visuals marked `scene_occluded`:

```glsl
layout(set = N, binding = 0) uniform sampler sceneOcclusionSampler;
layout(set = N, binding = 1) uniform texture2D sceneOcclusionDepth;
layout(set = N, binding = 2) uniform SceneOcclusionParams
{
    vec4 params; // depth_bias, soft_edge, hidden_alpha, enabled
} sceneOcclusion;
```

The set index should be assigned consistently by the scene pipeline builder. Existing visual-specific
set usage must remain stable.

### 6. Shader feature plumbing

Do not create duplicated `_occluded.frag` files.

Use one GLSL file per visual and compile feature variants with defines:

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

The renderer still creates distinct compiled shader modules and pipeline layouts, but source files
remain shared.

Pipeline/shader keys must include the feature:

```text
primitive_lit | glsl | wboit=0 | scene_occlusion=1
volume_slice  | glsl | scene_occlusion=1
```

### 7. Shader preprocessing

Add or extend a small Datoviz-side GLSL preprocessing step:

- inject `#define DVZ_SCENE_OCCLUSION 1` when needed
- resolve local `#include` files from `src/scene/glsl`
- feed resolved GLSL to glslang/shaderc

This avoids depending on compiler-specific include behavior while still using standard GLSL
preprocessor semantics for `#ifdef`.

### 8. Pipeline layout generation

The same feature bit must drive both shader compilation and pipeline layout creation.

When `scene_occluded = true`:

- compile shader with `DVZ_SCENE_OCCLUSION`
- add occlusion bind group layout
- create/bind occlusion bind group before draw

When `scene_occluded = false`:

- compile shader without the define
- do not add the layout
- do not bind the occlusion group

Mismatches must be treated as bugs.

### 9. Frame graph integration

For each panel:

1. Detect active occluders and occludees.
2. Emit occlusion prepass resources and passes before regular rendering.
3. If needed, emit depth merge pass.
4. Add graph reads from all occluded render passes to `panel.scene_occlusion.depth`.
5. Preserve existing render technique ordering:
   - opaque
   - blended
   - WBOIT accumulation/resolve
   - depth peeling
   - post-processing

The system must not route ordinary blended passes through WBOIT target ownership.

### 10. Allen example migration

First user-visible target:

- Atlas mesh contributes scene occlusion depth.
- Full volume contributes scene occlusion depth.
- Slice samples generic scene occlusion depth.
- Existing volume-specific slice occlusion controls are mapped to generic scene occlusion controls.
- Mesh alpha `1` continues to render in the opaque path.
- Partially transparent mesh regions continue to render via WBOIT, but only contribute occlusion if
  explicitly enabled.

### 11. Tests

Add focused coverage:

- Hidden/visible occluder toggles do not produce invalid runtime streams.
- Mesh occluder depth pass appears before occluded visual passes.
- Occluded visual pass declares graph read on scene occlusion depth.
- Mixed WBOIT and blended passes remain valid.
- Fully opaque atlas mesh uses opaque alpha mode in the Allen example.
- Shader/pipeline feature keys differ for occluded vs non-occluded variants.

### 12. Implementation phases

Phase 1: Design scaffolding

- Add scene occlusion flags and descriptors.
- Add frame-graph resource/pass naming helpers.
- Add tests for graph emission only.

Phase 2: Mesh occlusion producer

- Add mesh/primitive/sphere depth prepass.
- Write `R32_SFLOAT` front depth.
- Wire Allen atlas mesh as occluder.

Phase 3: Generic occlusion consumer for volume slice

- Add shared occlusion bind group.
- Add `scene_occlusion.glsl`.
- Compile `volume_slice.frag` with `DVZ_SCENE_OCCLUSION` when needed.
- Route Allen slice through generic occlusion.

Phase 4: Volume producer migration

- Move current volume front-depth prepass into generic scene occlusion producer path.
- Add merge pass for mesh + volume depth.
- Remove or deprecate volume-specific occlusion plumbing once equivalent behavior is covered.

Phase 5: Additional consumers

- Add occlusion support to primitive/mesh/image/sphere/point shader paths.
- Keep non-occluded visuals zero-cost.

Phase 6: GUI and polish

- Expose scene occlusion controls in Allen GUI.
- Reorganize controls into clear sections.
- Keep advanced producer controls hidden unless needed.

## Recommended first commit sequence

1. Add this plan document.
2. Add retained scene occlusion flags/API with graph-only tests.
3. Add mesh occlusion prepass emission.
4. Add shader preprocessing support for feature defines/includes.
5. Add generic occlusion bind group and volume-slice consumer.
6. Update Allen example to use mesh + volume scene occlusion.
7. Migrate current volume occlusion path into generic scene occlusion.
