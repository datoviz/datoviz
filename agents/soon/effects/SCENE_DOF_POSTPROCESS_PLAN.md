# Datoviz v0.4-dev: High-quality DoF / post-processing technique plan

Status: draft implementation note for a future agent  
Scope: optional high-quality depth-of-field / tilt-shift effect, designed to fit the v0.4-dev `scene -> FramePlan -> DRP2 -> vklite/Vulkan` architecture.  
Related effect family: MSAA, EDL, SSAO, WBOIT, tone mapping, bloom, future post-processing techniques.

## 1. Motivation

The gallery style guide proposes a cinematic but data-first visual identity for selected v0.4 examples. A mild high-quality depth-of-field effect can improve showcase visuals for 3D scenes such as molecular arcball, LiDAR, brain/mesh, volume, and particle/sphere examples.

This effect should **not** become a default scientific-visualization style. It is appropriate for showcase screenshots/videos, but should remain disabled for baseline visual-family comparisons, quantitative 2D plots, axes-heavy examples, colorbar/annotation-heavy panels, and reference screenshots.

The implementation should be treated as the first general-purpose **post-processing technique** in the scene/FramePlan stack. It should force the same infrastructure needed later by EDL, SSAO, tone mapping, bloom, and other optional effects:

- transient render targets,
- sampled render targets,
- fullscreen post-processing passes,
- deterministic pass ordering,
- capability adaptation and diagnostics,
- late overlays/annotations after post-processing.

## 2. Architectural fit

The current v0.4-dev architecture is a good fit for this feature if it is implemented as a `FramePlan`/DRP2 technique, not as an ad hoc Vulkan-only path.

The intended stack is:

```text
Scene / Panel / Visual state
  -> FramePlan
  -> scene-to-DRP2 converter
  -> DvzDrp2Runtime
  -> vklite / Vulkan backend
```

The scene layer should remain backend-independent. It should describe logical work and dependencies, while the runtime owns backend execution details.

The current `FramePlan` model already has the right conceptual pieces:

- logical color/depth/picking/offscreen/transient targets,
- `UploadNode`, `ComputeNode`, `RenderNode`, `CopyNode`, `ReadbackNode`,
- explicit ordering/dependencies,
- logical resources and read/write sets,
- deterministic frame-level planning.

DoF should therefore be represented as a deterministic expansion of panel-level technique state into additional targets and nodes.

## 3. Design principle

DoF should be implemented as a **panel-scoped optional technique**.

Do not expose a low-level user-editable render graph initially. Instead, expose simple declarative API state, then let planning expand it into an internal mini-graph.

Suggested high-level model:

```c
typedef enum
{
    DVZ_EFFECT_NONE = 0,
    DVZ_EFFECT_MSAA = 1 << 0,
    DVZ_EFFECT_EDL  = 1 << 1,
    DVZ_EFFECT_SSAO = 1 << 2,
    DVZ_EFFECT_DOF  = 1 << 3,
} DvzEffectFlags;
```

Suggested public API shape:

```c
DVZ_EXPORT void dvz_panel_effects(DvzPanel* panel, DvzEffectFlags flags);
DVZ_EXPORT void dvz_panel_set_dof(DvzPanel* panel, const DvzDofParams* params);
```

Alternative, if a retained technique handle is preferred:

```c
DvzTechnique* tech = dvz_panel_technique(panel, DVZ_TECHNIQUE_DOF);
dvz_technique_set_enabled(tech, true);
dvz_technique_dof_params(tech, &params);
```

Initial recommendation: use the simpler panel API first.

## 4. Effect semantics

The first implementation should support true **depth-of-field** from the depth buffer. A screen-space tilt-shift mode can be added later as a simpler stylized variant.

Recommended modes:

```c
typedef enum
{
    DVZ_DOF_OFF = 0,
    DVZ_DOF_DEPTH,
    DVZ_DOF_TILT_SHIFT,
    DVZ_DOF_HYBRID,
} DvzDofMode;
```

Recommended quality levels:

```c
typedef enum
{
    DVZ_DOF_QUALITY_FAST = 0,    // half-res separable blur
    DVZ_DOF_QUALITY_MEDIUM,      // half-res gather, no tile max
    DVZ_DOF_QUALITY_HIGH,        // tile max + bokeh gather
    DVZ_DOF_QUALITY_ULTRA,       // more samples, foreground/background split
} DvzDofQuality;
```

Recommended parameter struct:

```c
typedef struct DvzDofParams
{
    DvzDofMode mode;
    DvzDofQuality quality;

    float focus_distance;     // eye-space units
    float focus_range;        // optional dead zone around focus distance
    float aperture;           // visual strength / CoC scale
    float focal_length;       // optional future physical-camera model
    float max_radius_px;      // clamp, typically 6-12 px at gallery resolution

    float near_blur_scale;    // foreground blur strength
    float far_blur_scale;     // background blur strength

    uint32_t blade_count;     // 0 = disk, 6 = hexagonal aperture, etc.
    float blade_rotation;
} DvzDofParams;
```

Suggested default for showcase use:

```c
DvzDofParams p = {
    .mode = DVZ_DOF_DEPTH,
    .quality = DVZ_DOF_QUALITY_HIGH,
    .focus_distance = 10.0f,
    .focus_range = 0.25f,
    .aperture = 0.035f,
    .max_radius_px = 8.0f,
    .near_blur_scale = 1.0f,
    .far_blur_scale = 0.75f,
    .blade_count = 0,
    .blade_rotation = 0.0f,
};
```

## 5. High-quality DoF pipeline

For one panel, the target high-quality pipeline is:

```text
1. Scene render pass
   outputs:
     color_hdr          RGBA16F preferred
     depth              D32 / D24S8 / backend-supported sampled depth

2. CoC pass
   inputs:
     depth
   output:
     coc_signed         R16F or RG16F
   purpose:
     compute signed circle of confusion

3. Prefilter/downsample pass
   inputs:
     color_hdr, coc_signed
   outputs:
     color_half         RGBA16F
     coc_half           R16F or RG16F
   purpose:
     reduce bandwidth and prepare stable blur input

4. Tile-max CoC pass
   inputs:
     coc_half
   output:
     coc_tile_max       R16F or RG16F
   purpose:
     accelerate variable-radius gather

5. Bokeh gather pass
   inputs:
     color_half, coc_half, coc_tile_max
   output:
     dof_half           RGBA16F
   purpose:
     high-quality disk/hexagonal aperture blur

6. Composite pass
   inputs:
     color_hdr, dof_half, coc_signed
   output:
     panel_color_final
   purpose:
     mix sharp and blurred images according to CoC

7. Late overlay pass
   axes, labels, colorbars, annotations, probe markers, GUI
   purpose:
     keep semantic overlays sharp and readable
```

The first implementation can simplify step 4 if needed, but the high-quality target should include it.

## 6. Circle of confusion model

Use **signed CoC**.

```text
coc < 0 : foreground blur, object in front of focus plane
coc > 0 : background blur, object behind focus plane
```

Signed CoC helps prevent foreground/background bleeding and gives future room for separate near/far blur paths.

A stable non-physical first model is enough:

```glsl
float z = linearize_depth(depth, near, far); // eye-space positive distance
float coc = (z - focus_distance) / max(z, 1e-6);
coc *= aperture_scale;

float dead = focus_range;
float a = abs(coc);
a = max(0.0, a - dead);
coc = sign(coc) * a;

coc = clamp(coc, -max_coc_px, +max_coc_px);
```

Later, a more physical camera model can be added without changing the general graph.

## 7. Render target and format requirements

Preferred formats:

```text
color_hdr      RGBA16F
color_half     RGBA16F
coc_signed     R16F or RG16F
coc_half       R16F or RG16F
tilemax        R16F or RG16F
dof_half       RGBA16F
depth          sampled depth format supported by runtime
```

Minimum useful capability set for high-quality DoF:

```text
supports_render_target_sampling = true
render_target_format_rgba16float = true
render_target_format_r16float = true, if added to capability snapshot
max_texture_dimension_2d >= panel target size
shader_format_glsl or shader_format_wgsl
```

Useful optional support:

```text
storage textures for compute tile-max path
linear filtering on half-float render targets
sampled depth textures
```

Fallback ladder:

```text
HIGH:
  RGBA16F + R16F/RG16F CoC + half-res bokeh + tilemax

MEDIUM:
  RGBA16F + half-res gather, no tilemax

LOW:
  RGBA8 + approximate CoC or screen-space tilt-shift

UNSUPPORTED:
  disable effect and emit deterministic diagnostic
```

## 8. Required FramePlan extensions

The current first-slice `FramePlan` helper API is enough for simple rendering but too narrow for high-quality post-processing. Add richer target and pass descriptors.

Suggested target descriptor:

```c
typedef struct DvzFramePlanTargetDesc
{
    const char* id;
    uint32_t kind;          // color, depth, transient, resolve, picking
    uint32_t format;
    uint32_t width;
    uint32_t height;
    uint32_t sample_count;
    uint32_t usage;         // render_attachment | sampled | storage | copy_src | copy_dst
    bool transient;
    bool persistent;
} DvzFramePlanTargetDesc;
```

Suggested API helpers:

```c
bool dvz_frame_plan_target(DvzFramePlan* plan, const DvzFramePlanTargetDesc* desc);
bool dvz_frame_plan_render_set_depth(DvzFramePlan* plan, const char* depth_target_id);
bool dvz_frame_plan_render_add_color(
    DvzFramePlan* plan,
    const char* color_target_id,
    uint32_t load_op,
    uint32_t store_op);
```

Current `dvz_frame_plan_render(plan, panel_id, render_target_id, picking)` can remain as a convenience for simple cases.

## 9. Required render pass roles

The existing `DvzFramePlanRenderPassRole` already contains roles for G-buffer, SSAO, EDL, transparency, WBOIT, depth peeling, and picking. Add DoF roles.

Recommended explicit roles:

```c
DVZ_FRAME_PLAN_RENDER_PASS_DOF_COC,
DVZ_FRAME_PLAN_RENDER_PASS_DOF_PREFILTER,
DVZ_FRAME_PLAN_RENDER_PASS_DOF_TILE_MAX,
DVZ_FRAME_PLAN_RENDER_PASS_DOF_BOKEH,
DVZ_FRAME_PLAN_RENDER_PASS_DOF_COMPOSITE,
```

If the enum should remain compact, use a generic postprocess role plus shader keys, but explicit roles are better for diagnostics, tests, and gallery reproducibility.

Suggested shader keys:

```text
post.dof.coc
post.dof.prefilter
post.dof.tilemax
post.dof.bokeh
post.dof.composite
post.fullscreen_triangle
```

## 10. Scene planning expansion

When a panel has DoF enabled, planning should expand it into a deterministic mini-graph.

Pseudo-code:

```c
if (panel->effects.dof.enabled)
{
    add_target("panel.color.hdr", RGBA16F, COLOR_ATTACHMENT | SAMPLED);
    add_target("panel.depth", D32, DEPTH_ATTACHMENT | SAMPLED);
    add_target("dof.coc", R16F, COLOR_ATTACHMENT | SAMPLED | TRANSIENT);
    add_target("dof.prefilter", RGBA16F, COLOR_ATTACHMENT | SAMPLED | TRANSIENT);
    add_target("dof.coc_half", R16F, COLOR_ATTACHMENT | SAMPLED | TRANSIENT);
    add_target("dof.tilemax", R16F, COLOR_ATTACHMENT_OR_STORAGE | SAMPLED | TRANSIENT);
    add_target("dof.bokeh", RGBA16F, COLOR_ATTACHMENT | SAMPLED | TRANSIENT);

    add_render_node(DVZ_FRAME_PLAN_RENDER_PASS_OPAQUE,
                    "panel.color.hdr", "panel.depth");

    add_render_node(DVZ_FRAME_PLAN_RENDER_PASS_DOF_COC,
                    "dof.coc");

    add_render_node(DVZ_FRAME_PLAN_RENDER_PASS_DOF_PREFILTER,
                    "dof.prefilter");

    add_compute_or_render_node(DVZ_FRAME_PLAN_RENDER_PASS_DOF_TILE_MAX,
                               "dof.tilemax");

    add_render_node(DVZ_FRAME_PLAN_RENDER_PASS_DOF_BOKEH,
                    "dof.bokeh");

    add_render_node(DVZ_FRAME_PLAN_RENDER_PASS_DOF_COMPOSITE,
                    "panel.color.final");

    add_late_overlay_nodes_after_postprocess();
}
```

The graph must record read/write dependencies for each node so the runtime can infer ordering and resource transitions.

## 11. Render pass vs compute pass

Recommended high-quality implementation:

| Stage | Recommended path |
|---|---|
| CoC | fullscreen render pass |
| Prefilter/downsample | fullscreen render pass |
| Tile max CoC | compute pass if available; fullscreen fallback acceptable |
| Bokeh gather | fullscreen render pass or compute |
| Composite | fullscreen render pass |

The first implementation can use fullscreen render passes for all stages to reduce complexity.

A compute tile-max pass is useful later because it maps naturally to tiled reductions and can accelerate high-radius bokeh blur.

## 12. Shader files to add

Suggested shader files:

```text
src/scene/shaders/fullscreen.vert
src/scene/shaders/dof_coc.frag
src/scene/shaders/dof_prefilter.frag
src/scene/shaders/dof_tilemax.comp
src/scene/shaders/dof_bokeh.frag
src/scene/shaders/dof_composite.frag
```

Or equivalent location following existing shader organization.

First native implementation may use GLSL. Longer term, WGSL should be supported for DRP2/WebGPU portability.

## 13. Resource transitions and barriers

The active DRP2 2.0 surface currently keeps the command set narrow. If explicit resource barriers are not active, the runtime must infer transitions from node ordering and read/write sets.

Critical transitions:

```text
color_hdr:
  color attachment write -> sampled read

depth:
  depth attachment write -> sampled read

coc:
  color attachment write -> sampled read

prefilter:
  color attachment write -> sampled read

tilemax:
  storage/color write -> sampled read

bokeh:
  color attachment write -> sampled read

final:
  color attachment write -> present/copy/readback
```

Short-term strategy:

```text
FramePlan read/write sets -> runtime transition inference -> backend-specific barriers
```

Medium-term strategy:

```text
Add enough target usage metadata to FramePlan and DRP2 so transitions remain deterministic.
```

Long-term strategy:

```text
Promote ResourceBarrier or an equivalent pass-dependency model into DRP2 if inference becomes fragile.
```

## 14. Interaction with MSAA, EDL, SSAO, WBOIT

Recommended ordering:

```text
MSAA scene render, if enabled
  -> resolve color/depth if needed
  -> G-buffer / normal / depth preparation, if needed
  -> SSAO, if enabled
  -> EDL resolve, if enabled
  -> DoF
  -> tone mapping / gamma
  -> annotations / overlays
  -> present or readback
```

### MSAA + DoF

MSAA is not a normal postprocess. It affects the primary render target.

Simple first policy:

```text
If DoF is enabled, render the effect path single-sample unless a robust depth resolve policy exists.
```

Alternative:

```text
Render MSAA color/depth -> resolve color -> resolve or reconstruct depth -> DoF.
```

Depth resolve must be defined carefully; naive averaging is often wrong at silhouettes.

### EDL + DoF

EDL should run before DoF.

```text
color + depth -> EDL resolve -> DoF -> final
```

DoF then blurs the depth-enhanced image consistently.

### SSAO + DoF

SSAO should run before DoF.

```text
depth/normals -> SSAO -> AO composite -> DoF
```

Otherwise AO remains artificially sharp over blurred geometry.

### Transparency / WBOIT + DoF

Composite transparent layers before DoF when the goal is a photographic scene effect.

```text
opaque -> transparent accumulation/resolve -> DoF -> overlays
```

For scientific transparency where exact layer reading matters, prefer disabling DoF.

## 15. Overlay and annotation policy

DoF should blur only the data-rendering layer.

Blurred:

```text
mesh
sphere
volume
point cloud
primitive 3D geometry
```

Not blurred:

```text
axes
tick labels
colorbars
legends
annotations
probe readouts
selection labels
GUI overlays
```

Implementation rule:

```text
1. render data scene to color/depth
2. apply post-processing effects
3. render scene-native overlays/annotations
4. render external UI overlay
5. present/readback
```

This prevents cinematic effects from damaging quantitative readability.

## 16. Diagnostics

DoF should never fail silently. Add scene-visible diagnostics during capability adaptation or planning.

Examples:

```text
DoF disabled: render-target sampling is unsupported by the runtime.
DoF downgraded: RGBA16F render targets are unavailable.
DoF downgraded: R16F CoC target unavailable; using approximate CoC format.
DoF disabled: sampled depth target unavailable.
DoF disabled: panel contains only 2D quantitative visuals and effect policy is showcase-only.
DoF+MSAA downgraded: depth resolve policy unavailable; rendering DoF path single-sample.
```

Diagnostics should remain scene-level and not expose Vulkan handles or backend-specific objects.

## 17. Testing plan

### Unit / spec tests

Add FramePlan JSON tests verifying that enabling DoF expands the plan deterministically:

```text
opaque render node
DoF CoC node
DoF prefilter node
DoF tilemax node
DoF bokeh node
DoF composite node
late overlay node after composite
```

Check target descriptors:

```text
color_hdr: RGBA16F, sampled, color attachment
panel_depth: sampled depth, depth attachment
coc: R16F/RG16F, sampled, transient
bokeh: RGBA16F, sampled, transient
final: present/export target
```

### Capability adaptation tests

Test each fallback path:

```text
full high-quality path accepted
RGBA16F unavailable -> downgraded or disabled
render-target sampling unavailable -> disabled
sampled depth unavailable -> disabled or tilt-shift fallback
MSAA+DoF without depth resolve -> deterministic downgrade
```

### DRP2 fixture tests

Add positive fixtures for the active command stream generated by a minimal DoF panel:

```text
CreateTexture transient targets
CreateTextureView target views
CreateSampler
CreateShaderModule fullscreen/dof shaders
CreateRenderPipeline postprocess pipelines
BeginRenderPass / Draw fullscreen triangle
QueueSubmit
```

Add negative fixtures for missing target sampling or unsupported format.

### Visual regression tests

Use deterministic scene, camera, and seed.

Suggested scene:

```text
sphere/molecular cluster with foreground, focused middle layer, and background layer
```

Assertions:

```text
center/focus object remains sharp
foreground/background are blurred
overlay text remains sharp
output is nonblank
output roughly stable under fixed seed
```

## 18. Implementation phases

### Phase 1: native prototype

Create an internal example, e.g.:

```text
examples/c/showcase_dof.c
```

Hard-code:

```text
RGBA16F color target
sampled depth target
R16F CoC
half-res bokeh target
fixed camera/focus distance
molecular/sphere visual scene
```

Goal: validate visual quality and backend sequencing before freezing API details.

### Phase 2: FramePlan target/pass extension

Implement:

```text
transient target descriptors
multi-attachment/depth render-node descriptors
sampled render target usage
DoF render pass roles
read/write dependency metadata
```

### Phase 3: scene technique expansion

Implement panel-scoped effect state and planner expansion:

```text
panel.effects.dof -> deterministic target/node graph
```

### Phase 4: DRP2 emission and runtime support

Extend scene-to-DRP2 emission:

```text
create transient textures/views/samplers
create postprocess shaders/pipelines
bind sampled inputs
emit fullscreen passes
reuse cached pipelines/resources where possible
```

Runtime responsibilities:

```text
allocate transient images
infer transitions from read/write sets
bind framebuffer/dynamic rendering targets
submit passes in order
```

### Phase 5: public API and diagnostics

Expose:

```c
dvz_panel_effects()
dvz_panel_set_dof()
```

Add default params, validation, capability adaptation, and diagnostics.

### Phase 6: documentation and gallery presets

Document policy:

```text
showcase preset: DoF allowed
visual baseline preset: DoF disabled
features preset: DoF disabled unless the feature is postprocessing
publication-adjacent examples: DoF disabled by default
```

Add a gallery example showing DoF on/off comparison.

## 19. Minimal first implementation target

The first useful target should be:

```text
Depth-based DoF
single panel
single-sample render targets
RGBA16F scene color
sampled depth
R16F signed CoC
half-resolution bokeh gather
fullscreen composite
late annotations unblurred
GLSL shaders in native runtime
```

This is enough for high-quality showcase screenshots and will validate the required postprocessing infrastructure.

## 20. Non-goals for first implementation

Do not attempt initially:

```text
full physical camera model
artist-editable custom render graph
perfect MSAA depth resolve
per-object focus tracking API
automatic focus picking
WebGPU parity if the native path is not ready
ray-traced DoF
motion blur
bloom/tone mapping unless needed for the example
```

## 21. Recommended defaults for Datoviz gallery

For gallery showcases:

```text
quality: HIGH
max_radius_px: 6-10 depending on capture resolution
foreground blur: mild
background blur: mild to moderate
focus target: selected/central object or fixed camera-space distance
overlays: always sharp
```

Avoid strong miniature-style tilt-shift unless the example explicitly demonstrates a stylized rendering effect.

## 22. Summary

High-quality DoF fits the v0.4-dev architecture if implemented as an optional panel-level technique expanded into FramePlan nodes and DRP2 commands.

The key missing infrastructure is not DoF-specific. It is the general post-processing foundation:

```text
richer logical targets
sampled render targets
transient resources
postprocess pass roles
fullscreen pass pipelines
runtime transition inference
late overlay ordering
capability-based fallback
```

Once this is in place, EDL, SSAO, tone mapping, bloom, and related optional effects can reuse the same model.
