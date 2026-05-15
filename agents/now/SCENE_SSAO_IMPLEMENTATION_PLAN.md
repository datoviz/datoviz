# Scene SSAO Implementation Plan

> **Execution Status**
> - **Status:** `PLANNING NOTE`
> - **Updated on:** `2026-05-15`
> - **Purpose:** make the SSAO integration path easy to find before the protein and
>   tractography examples start depending on it.


## Context

SSAO should be integrated through the active scene -> FramePlan -> DRP2 -> vklite runtime path. Do
not add a parallel renderer, presentation layer, or ad-hoc Vulkan path for scene SSAO.

The low-level mechanics already have a narrow vklite prototype:

- `src/vklite/tests/test_techniques.c:test_technique_ssao`
- `src/vklite/tests/shaders/ssao.frag`
- `src/vklite/tests/shaders/ssao_depth.frag`
- `src/vklite/tests/shaders/fullscreen.vert`

That prototype proves the Vulkan mechanics: create a sampled depth image, render into it, transition
it to shader-read layout, then run a fullscreen pass. It is not a production scene implementation:
the shader has hardcoded resolution, uses depth only, and does not reconstruct view-space position
or use normals.

The closest scene-level precedent is WBOIT. It already splits one panel into multiple FramePlan
render roles, creates per-panel intermediate textures, samples them in a fullscreen resolve pass,
and emits the path through DRP2. Use the WBOIT code shape in:

- `src/scene/scene_emit.c`
- `src/scene/frame_plan_runtime.c`
- `src/scene/shader_registry.c`
- `src/scene/tests/scene_graph.c`


## Target Pipeline

The minimum useful scene SSAO pipeline is:

```text
GEOMETRY / GBUFFER PASS
  outputs:
    base color texture
    normal texture
    linear depth texture

SSAO PASS
  inputs:
    normal texture
    linear depth texture
    noise texture
    kernel samples / params
  output:
    ssao texture

OPTIONAL BLUR PASS
  input:
    ssao texture
  output:
    blurred ssao texture

COMPOSITE PASS
  inputs:
    base color texture
    ssao or blurred ssao texture
  output:
    final panel color target
```

Start without blur. The first slice should be geometry -> SSAO -> composite.


## Public Scene API

Add panel-level SSAO state rather than visual-level state. SSAO depends on the composed panel
depth/normal buffer, so per-visual toggles are the wrong first abstraction.

Suggested first public shape:

```c
typedef struct DvzSsaoDesc
{
    bool enabled;
    float radius;
    float bias;
    float intensity;
    uint32_t sample_count;
    bool blur;
} DvzSsaoDesc;

DVZ_EXPORT DvzSsaoDesc dvz_ssao_desc(void);
DVZ_EXPORT int dvz_panel_set_ssao(DvzPanel* panel, const DvzSsaoDesc* desc);
```

Keep the API typed. Do not add a generic public framegraph or binding API for this first slice.


## FramePlan Changes

Extend `DvzFramePlanRenderPassRole` with SSAO-specific roles. A narrow role-based extension is
consistent with the current WBOIT approach and avoids introducing a generic framegraph too early.

Suggested roles:

```c
DVZ_FRAME_PLAN_RENDER_PASS_GBUFFER,
DVZ_FRAME_PLAN_RENDER_PASS_SSAO,
DVZ_FRAME_PLAN_RENDER_PASS_SSAO_BLUR,
DVZ_FRAME_PLAN_RENDER_PASS_SSAO_COMPOSITE,
```

When a panel has SSAO enabled, `_scene_emit_panel_render()` should emit:

1. a gbuffer render node for SSAO-applicable visuals,
2. an SSAO fullscreen render node,
3. optionally an SSAO blur render node,
4. a composite render node targeting `rt`.

For the first implementation, support mesh and primitive visuals with normals. Points, images, text,
and fixed-overlay visuals can remain on the ordinary final-target path until the composition policy
is deliberately broadened.


## Intermediate Resources

Create cached per-panel resources in the runtime emitter, following the WBOIT target-cache pattern:

- `_ssao_color_<panel_id>`: base color, likely `VK_FORMAT_R8G8B8A8_UNORM`
- `_ssao_normal_<panel_id>`: normal, likely `VK_FORMAT_R16G16B16A16_SFLOAT`
- `_ssao_depth_<panel_id>`: linear depth, likely `VK_FORMAT_R32_SFLOAT`
- `_ssao_ao_<panel_id>`: AO factor, likely `VK_FORMAT_R8_UNORM` or `VK_FORMAT_R16_SFLOAT`
- `_ssao_blur_<panel_id>`: optional blur target

Do not start by sampling the transient Vulkan depth attachment created by
`dvz_drp2_stream_begin_render_pass_set_depth()`. DRP2 currently creates that depth attachment
inside the vklite render-pass path for depth testing, and it is not represented as a sampled scene
resource. A color-encoded linear-depth render target is a simpler and more portable first slice.


## Shader Work

Add builtin scene shaders in the shader registry:

- gbuffer mesh/primitive vertex and fragment variants,
- fullscreen SSAO fragment,
- optional fullscreen blur fragment,
- fullscreen composite fragment.

The gbuffer pass should still request normal depth testing through the existing transient depth
attachment path. It should additionally write a sampled linear-depth color target. The SSAO shader
should take viewport size, radius, bias, intensity, sample count, and projection/reconstruction data
through a uniform buffer.

Runtime-generated data needed by the SSAO pass:

- `ssao_kernel`: float32 sample vectors, default 16 samples,
- `ssao_noise_texture`: small random tangent-space rotation texture, default 4x4.


## DRP2 / Runtime Work

Most required DRP2 primitives already exist:

- multi-color render attachments,
- sampled textures,
- samplers,
- bind-group layouts and bind groups,
- fullscreen triangle draws,
- render-target-to-sampled transitions for texture-binding render targets.

Likely DRP2 gaps to check while implementing:

- support for the chosen single-channel formats in serialization/validation/runtime,
- enough color attachments for gbuffer output,
- stable resource recreation on figure resize,
- bind-group layout shape for multiple sampled textures plus uniform buffer plus sampler.

If a gap appears in DRP2, extend the existing command model narrowly rather than bypassing it with
vklite-only code.


## Capability Checks

Extend `DvzCapabilitySnapshot` and `_validate_capabilities()` for SSAO:

- `max_color_attachments >= 3` for base color, normal, and linear depth gbuffer outputs,
- render-target sampling is supported,
- chosen normal/depth/AO render-target formats are supported,
- enough sampled texture bindings and bind groups exist,
- color blending is not required for the basic SSAO composite unless the composite is blended over
  pre-existing content.

Diagnostics should be explicit, like the current WBOIT messages.


## Tests

Add tests in increasing cost order:

1. `scene` command-shape test: enabling panel SSAO emits gbuffer, SSAO, and composite roles.
2. `scene` DRP2 emission test: stream contains gbuffer textures, SSAO bind group, fullscreen draws,
   and validates.
3. Runtime GPU smoke: offscreen mesh with SSAO enabled executes through vklite without validation
   errors and produces nonblank pixels.
4. Toggle test: rendering with SSAO enabled changes the captured image compared with disabled SSAO.
5. Resize test: per-panel SSAO intermediate targets are recreated at the new target extent.

Use the narrowest available validation loop while working:

```sh
just build
just test scene
just test drp2
git diff --check
```

For Vulkan-path changes, also run a focused GPU/offscreen smoke when the environment supports it.


## Recommended First Slice

Implement only this first:

1. `DvzSsaoDesc` plus `dvz_panel_set_ssao()`.
2. FramePlan roles for gbuffer, SSAO, and composite.
3. Mesh/primitive-with-normals gbuffer path.
4. Per-panel color, normal, linear-depth, and AO textures.
5. Fullscreen SSAO pass and composite pass in GLSL.
6. Scene command-shape test and one GPU smoke test.

Defer blur, WebGPU/WGSL parity, point/image participation, generated normals, GUI controls, and a
fully generic framegraph until the first retained scene SSAO path is executing reliably.
