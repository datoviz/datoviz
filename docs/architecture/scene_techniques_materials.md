# Scene Techniques And Materials

Date: 2026-05-17

This note records the target architecture for advanced scene visual effects in v0.4. The immediate
goal is not to add another rendering effect. The goal is to keep depth cueing, EDL, AO, outlines,
curvature/cavity shading, and future transparency modes from becoming independent special cases in
`scene_emit.c`, `frame_plan_runtime.c`, and `visual_pipeline.c`.


## Current Position

The active scene stack already has the right low-level route:

```text
retained scene -> FramePlan graph -> DRP2 command stream -> vklite/canvas runtime
```

Current implemented pieces that advanced techniques should reuse:

1. retained visual families: point, pixel, marker, primitive, mesh, path/segment, image, volume,
   and sphere impostors;
2. retained data objects: sampled fields, scene buffers, scales, colormaps, and colorbars;
3. per-panel controllers: panzoom, arcball, and camera;
4. graph-backed FramePlan resources, passes, attachments, reads/writes, dependencies, validation,
   and deterministic dumps;
5. graph-backed opaque-depth, WBOIT, depth-peeling, blended-volume, G-buffer, EDL, AO, AO blur,
   and MSAA paths;
6. request-time point picking and image probing through auxiliary DRP2 streams;
7. built-in GLSL/WGSL shader registry support, with visual-family shader selection centralized in
   `src/scene/visual_pipeline.c`;
8. public `DvzMaterialDesc` material values for primitive, mesh, and sphere visuals, backed by
   shared material shader evaluation.

The missing piece is no longer basic Vulkan capability or the first material/technique foundation.
The remaining work is consistent scene-level policy for the next effects and visual families. There
is no heap-allocated public `DvzMaterial` object today. The closest current concepts are:

1. `DvzAlphaMode` on `DvzVisual`;
2. `DvzMaterialDesc`, `DvzSceneMaterialState`, and `DvzSceneMaterialParams`;
3. `DvzVolumeState` for volume-specific opacity, sampling, render mode, steps, and clipping;
4. `DvzScale` / `DvzColormap` bindings for scalar-to-color mapping;
5. per-family shader and pipeline descriptors in `visual_pipeline.c`.

Those pieces now form the first material system, but they are deliberately narrow. The remaining
policy questions are where material fields apply beyond primitive/mesh/sphere, when G-buffer output
needs material-aware channels, and how selected outlines or scalar modulation should use object-id
and material state without hardcoding domain-specific semantics.


## Target Layering

Keep the public scene API declarative. Users should request retained visuals, materials, alpha
modes, and panel effects. They should not construct render passes, intermediate textures, or
fullscreen resolve shaders in the common path.

Use four internal layers:

```text
Scene retained state
  figures, panels, visuals, fields, scales, materials, interaction

Technique planning
  opaque/depth, transparency, G-buffer, EDL, AO, outlines, picking/probing

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
AO builder
outline builder
object-id / picking builder
volume blend/raymarch policy builder
```

The first implementation slice is complete: graph construction has moved behind builder-like
helpers, and new effects should continue to append generic FramePlan resources and passes rather
than adding scene-private runtime paths.


## Material State

The branch started with internal material state and now has a narrow public value descriptor.
Continue to keep public API typed while the scene lowers old and new setters into one internal
material representation.

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
2. `DvzMaterialDesc` is the public route for lit material fields.
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
  normal/depth and AO receiver when normals are available

path
  color and depth for line/strip first
  tube-style normal/depth belongs to future `tube` rendering, not plain strokes

image
  color and probe
  object-id later for selected image/layer outlines
  generally not AO/EDL

volume
  raymarch color, blended transparency
  depth interaction and depth-aware composition later
```

The descriptor should feed shader selection, pipeline state, and technique eligibility. The
long-term direction is "visual family + material + pass kind -> shader/pipeline/bind descriptor".


## Near-Term Effects

Prioritize remaining effects by infrastructure value and scientific usefulness. Depth cueing,
G-buffer, EDL, AO, sphere impostors, and MSAA now have active slices, so the list below separates
implemented foundations from next policy work.

1. **Depth cueing.** Implemented as material/pipeline shader behavior, not a postprocess.
2. **Depth/normal G-buffer.** Implemented as graph-backed infrastructure for eligible
   primitive/mesh/sphere paths. Object-id output remains deferred until a concrete outline or
   selection effect needs it.
3. **Eye-dome lighting.** Implemented as a panel-local graph-backed technique for opaque
   depth-producing visuals.
4. **AO/GTAO.** Deterministic view-space GTAO consumes the coherent surface record, uses bounded edge-aware denoising, and modulates only eligible ambient or indirect diffuse lighting.
5. **ID-based selected outlines.** Prefer an object/group-id buffer for stable semantic outlines.
   Do not permanently overload pick payloads for this role.
6. **Curvature/cavity scalar modulation.** Keep it generic. Molecular surfaces can use it, but the
   same slot should support any meaningful scalar modulation channel.

Additional high-value visual lanes:

1. sphere impostor texture/equirectangular follow-ups and WGSL parity;
2. future [`tube`](../../spec/scene/visuals/TUBE.md) rendering for streamlines, fibers,
   trajectories, and skeletons;
3. depth-aware labels and annotations;
4. transfer-function improvements for volumes and scalar fields;
5. clip planes shared by meshes, volumes, and point clouds where practical.


## Implementation Order

Original implementation sequence and current status:

1. Done: add internal technique-builder files/types and move current opaque-depth, blended, WBOIT,
   and depth-peeling graph setup behind them without behavior change.
2. Done: add internal material state and route lit primitive material through the shared material path.
3. Done: add material-driven depth cueing for primitive/mesh and point/pixel where supported.
4. Done: add visual pass capability descriptors and tests for existing visual families.
5. Done: add a G-buffer technique builder for normal/depth resources, starting with eligible
   primitive/mesh/sphere visuals.
6. Done: implement EDL through the graph-backed fullscreen pass path.
7. Deferred: implement object-id selected outlines through the same graph path.
8. Done: implement AO using the G-buffer foundation.
9. Deferred: add generic scalar material modulation, then use it for curvature/cavity demos.
10. Current polish: improve standard-material appearance, decide material policy for
    point/pixel/image/volume, and keep graph-backed technique composition explicit.


## Non-Goals For The First Slice

1. No public low-level framegraph API.
2. No shader-snippet system before the fixed built-in shader variants are rationalized.
3. No resource aliasing or transient graph allocator.
4. No replacement of the retained scene API with material nodes.
5. No second renderer or scene-private Vulkan path.
6. No molecule-specific material hardcoding in the core scene layer.
