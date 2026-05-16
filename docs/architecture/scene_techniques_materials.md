# Scene Techniques And Materials

Date: 2026-05-16

This note records the target architecture for advanced scene visual effects in v0.4. The immediate
goal is not to add another rendering effect. The goal is to keep depth cueing, EDL, SSAO, outlines,
curvature/cavity shading, and future transparency modes from becoming independent special cases in
`scene_emit.c`, `frame_plan_runtime.c`, and `visual_pipeline.c`.


## Current Position

The active scene stack already has the right low-level route:

```text
retained scene -> FramePlan graph -> DRP2 command stream -> vklite/canvas runtime
```

Current implemented pieces that advanced techniques should reuse:

1. retained visual families: point, pixel, primitive, mesh, path-as-line/strip, image, and volume;
2. retained data objects: sampled fields, scene buffers, scales, colormaps, and colorbars;
3. per-panel controllers: panzoom, arcball, and camera;
4. graph-backed FramePlan resources, passes, attachments, reads/writes, dependencies, validation,
   and deterministic dumps;
5. graph-backed opaque-depth, WBOIT, depth-peeling, and blended-volume paths;
6. request-time point picking and image probing through auxiliary DRP2 streams;
7. built-in GLSL/WGSL shader registry support, with visual-family shader selection centralized in
   `src/scene/visual_pipeline.c`.

The missing piece is not Vulkan capability. The missing piece is a consistent scene-level policy
layer. There is no `DvzMaterial` object today. The closest current concepts are:

1. `DvzAlphaMode` on `DvzVisual`;
2. `DvzPrimitiveShadingDesc` and the internal primitive shading uniform buffer;
3. `DvzVolumeState` for volume-specific opacity, sampling, render mode, steps, and clipping;
4. `DvzScale` / `DvzColormap` bindings for scalar-to-color mapping;
5. per-family shader and pipeline descriptors in `visual_pipeline.c`.

Those pieces are useful, but they are not a material system. They also do not describe whether a
visual can participate in a depth-only pass, normal/depth G-buffer pass, object-id pass, EDL source
pass, SSAO receiver path, or selected-outline pass.


## Target Layering

Keep the public scene API declarative. Users should request retained visuals, materials, alpha
modes, and panel effects. They should not construct render passes, intermediate textures, or
fullscreen resolve shaders in the common path.

Use four internal layers:

```text
Scene retained state
  figures, panels, visuals, fields, scales, materials, interaction

Technique planning
  opaque/depth, transparency, G-buffer, EDL, SSAO, outlines, picking/probing

FramePlan graph
  typed resources, typed passes, dependencies, render/copy/readback/compute work

DRP2 emission
  concrete textures, buffers, samplers, bind groups, pipelines, draws, dispatches
```

Technique planning is the new architectural layer. A technique planner reads retained scene intent
and appends generic FramePlan graph resources and passes. DRP2 emission should lower graph intent
to commands without needing to know why the pass exists beyond its typed role, attachments,
resources, and draw/dispatch work.


## Technique Builders

Add internal technique builders incrementally. They should live near scene planning, not in the
runtime backend.

Initial target shape:

```c
typedef struct DvzSceneTechniqueContext
{
    DvzFigure* figure;
    DvzPanel* panel;
    uint32_t panel_index;
    const char* figure_id;
    const char* panel_id;
    DvzFramePlan* plan;
    DvzMVP panel_apply_mvp;
    DvzSceneViewportUniform panel_viewport;
} DvzSceneTechniqueContext;
```

Candidate builders:

```text
opaque/depth builder
transparent blended builder
WBOIT builder
depth-peeling builder
G-buffer builder
EDL builder
SSAO builder
outline builder
object-id / picking builder
volume blend/raymarch policy builder
```

The first implementation slice should be behavior-preserving: move the current opaque-depth,
blended, WBOIT, and depth-peeling graph construction behind builder-like helpers without changing
the emitted DRP2 command shape.


## Material State

Start with internal material state, not a broad public API. Public API can stay typed while the
scene lowers old and new setters into one internal material representation.

Target internal shape:

```c
typedef enum
{
    DVZ_MATERIAL_KIND_UNLIT = 0,
    DVZ_MATERIAL_KIND_LIT,
    DVZ_MATERIAL_KIND_SCIENTIFIC,
    DVZ_MATERIAL_KIND_VOLUME,
} DvzMaterialKind;

typedef enum
{
    DVZ_DEPTH_CUE_NONE = 0,
    DVZ_DEPTH_CUE_FADE_TO_BACKGROUND,
    DVZ_DEPTH_CUE_DESATURATE,
    DVZ_DEPTH_CUE_DARKEN,
} DvzDepthCueMode;

typedef struct DvzMaterialState
{
    DvzMaterialKind kind;
    DvzAlphaMode alpha_mode;

    float opacity;
    float ambient;
    float diffuse;
    float specular;
    float shininess;

    bool depth_cue_enabled;
    DvzDepthCueMode depth_cue_mode;
    float depth_cue_near;
    float depth_cue_far;
    float depth_cue_strength;

    bool scalar_modulation_enabled;
    char scalar_slot[32];
    float scalar_scale;
    float scalar_bias;

    uint64_t version;
} DvzMaterialState;
```

Migration rules:

1. `DvzAlphaMode` remains the public alpha policy for now, but internally belongs to material state.
2. `DvzPrimitiveShadingDesc` becomes a compatibility setter for lit material fields.
3. `DvzVolumeState` remains volume-specific at first; later, common opacity/depth cue fields can
   move into material state while raymarch-specific state stays volume-owned.
4. Scale/colormap bindings remain separate data mapping objects. A material may reference scalar
   modulation slots, but scales continue to own domain and color mapping policy.
5. Curvature, cavity, accessibility, uncertainty, density, and saliency should be generic scalar
   material channels rather than hardcoded molecular features.


## Visual Pass Capabilities

Every visual family should advertise which technique passes it can provide. This avoids growing
conditionals such as "primitive or mesh with normals" throughout the planner and runtime emitter.

Target internal descriptor:

```c
typedef struct DvzSceneVisualPassCaps
{
    bool color;
    bool depth;
    bool normal_depth;
    bool object_id;
    bool pick;
    bool edl_depth;
    bool transparent_accum;
    bool volume_raymarch;
} DvzSceneVisualPassCaps;
```

Initial capability policy:

```text
point / pixel
  color, object-id later, pick, EDL depth source after depth output exists

primitive / mesh
  color, depth, object-id, pick later, transparent accumulation
  normal/depth and SSAO receiver when normals are available

path
  color and depth for line/strip first
  tube-style normal/depth later when tube rendering exists

image
  color and probe
  object-id later for selected image/layer outlines
  generally not SSAO/EDL

volume
  raymarch color, blended transparency
  depth interaction and depth-aware composition later
```

The descriptor should feed shader selection, pipeline state, and technique eligibility. The
long-term direction is "visual family + material + pass kind -> shader/pipeline/bind descriptor".


## Near-Term Effects

Prioritize effects by infrastructure value and scientific usefulness.

1. **Depth cueing.** Implement as material/pipeline shader behavior, not a postprocess. It is cheap,
   broadly useful for 3D scientific scenes, and a good first test of material state.
2. **Depth/normal/object-id G-buffer.** Treat this as infrastructure. It enables EDL, SSAO,
   outlines, depth-aware labels, and richer picking/highlighting.
3. **Eye-dome lighting.** Implement before SSAO for point-heavy scenes. It only needs depth and is
   robust for point clouds, particles, and sparse 3D samples.
4. **ID-based selected outlines.** Prefer an object/group-id buffer for stable semantic outlines.
   Do not permanently overload pick payloads for this role.
5. **SSAO/GTAO.** Target primitive/mesh/sphere-impostor-like visuals that provide meaningful
   normals and depth.
6. **Curvature/cavity scalar modulation.** Keep it generic. Molecular surfaces can use it, but the
   same slot should support any meaningful scalar modulation channel.

Additional high-value visual lanes:

1. sphere impostors with analytic depth and normals for atoms and dense particles;
2. tube/path rendering for streamlines, fibers, trajectories, and skeletons;
3. depth-aware labels and annotations;
4. transfer-function improvements for volumes and scalar fields;
5. clip planes shared by meshes, volumes, and point clouds where practical.


## Implementation Order

Recommended implementation sequence:

1. Add internal technique-builder files/types and move current opaque-depth, blended, WBOIT, and
   depth-peeling graph setup behind them without behavior change.
2. Add internal `DvzMaterialState` to `DvzVisual`, initialize defaults, and route
   `dvz_visual_set_primitive_shading()` through material-backed state while preserving current
   emitted uniforms.
3. Add material-driven depth cueing for primitive/mesh first, then point/pixel and volume where
   appropriate.
4. Add visual pass capability descriptors and tests for the existing visual families.
5. Add a G-buffer technique builder for depth, normal, and object id resources. Start with
   primitive/mesh normals and depth.
6. Implement EDL through the graph-backed fullscreen pass path.
7. Implement object-id selected outlines through the same graph path.
8. Implement SSAO using the existing SSAO plan and the G-buffer foundation.
9. Add generic scalar material modulation, then use it for curvature/cavity demos.


## Non-Goals For The First Slice

1. No public low-level framegraph API.
2. No shader-snippet system before the fixed built-in shader variants are rationalized.
3. No resource aliasing or transient graph allocator.
4. No replacement of the retained scene API with material nodes.
5. No second renderer or scene-private Vulkan path.
6. No molecule-specific material hardcoding in the core scene layer.
