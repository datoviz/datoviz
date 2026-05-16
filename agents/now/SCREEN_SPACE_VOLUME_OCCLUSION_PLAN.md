# Screen-Space Volume Occlusion for Embedded Visuals

> **Execution Status**
> - **Status:** `PLANNING NOTE`
> - **Updated on:** `2026-05-16`
> - **Purpose:** define a proper architecture-level plan for making ordinary scene visuals appear
>   embedded inside translucent volume renderings using a screen-space volume occlusion prepass.


## Goal

Add a reusable scene feature that lets selected visuals be partially hidden by dense volume material
in front of them.

The intended visual model is:

```text
front volume density attenuates embedded visual fragments
embedded visual remains visible where front volume is sparse
back volume stays contextual through normal volume compositing
```

This is not fully physically correct volume/object ray integration. It is a practical, interactive
screen-space approximation that fits the existing retained scene, frame-plan, DRP2, and runtime
architecture.


## Non-Goals

This feature should not initially attempt to:

1. implement full ray tracing or path tracing;
2. intersect arbitrary meshes inside the volume raymarcher;
3. solve exact order-independent compositing between all volume samples and all scene geometry;
4. support every visual family in the first implementation slice;
5. replace proper embedded slice compositing inside the volume shader.

The first correct architectural version should support one volume occluder per panel and one or two
visual families as occlusion targets.


## Concept

For each panel with volume occlusion enabled:

1. render a volume occlusion prepass into a transient screen-space texture;
2. the prepass raymarches the chosen volume and records the first meaningful front volume depth;
3. later embedded visuals sample that texture in their fragment shader;
4. if a visual fragment is behind the recorded volume depth, its alpha is attenuated.

Pass-level model:

```text
background / opaque pass
volume occlusion prepass -> rt.volume_occlusion_depth
transparent / WBOIT / blended passes
    embedded visuals sample rt.volume_occlusion_depth
resolve / present
```


## Occlusion Texture Semantics

Use a transient sampled color texture rather than a hardware depth attachment.

Recommended first format:

```text
VK_FORMAT_R32_SFLOAT
```

Stored value:

```text
0.0 = no meaningful front volume hit for this pixel
>0  = first-hit linear view depth
```

The volume prepass raymarches front-to-back. It accumulates alpha using the active volume transfer
controls and writes the first ray position where accumulated alpha crosses a threshold.

Pseudo-code:

```glsl
float accum = 0.0;
float hit_depth = 0.0;

for each ray sample from front to back:
    vec4 sample = texture(volume_tex, texcoord);
    float a = sample.a * opacity;
    accum += (1.0 - accum) * a;

    if (accum > alpha_threshold) {
        hit_depth = linear_view_depth(sample_world_pos);
        break;
    }

out_depth = hit_depth;
```


## Embedded Visual Fragment Semantics

Each supported embedded visual shader samples the occlusion texture using screen-space coordinates.

Pseudo-code:

```glsl
vec2 screen_uv = gl_FragCoord.xy / viewport_size;
float volume_depth = texture(volume_occlusion_depth, screen_uv).r;
float fragment_depth = linear_view_depth(fragment_world_pos);

if (volume_depth > 0.0 && fragment_depth > volume_depth) {
    float d = fragment_depth - volume_depth;
    float t = smoothstep(0.0, fade_distance, d);
    color.a *= mix(1.0, occluded_alpha, t);
}
```

Use a soft attenuation by default, not hard discard. Hard discard is useful as a debug mode but too
brittle visually.


## Proposed Public API Shape

Add a panel-level volume occluder and a visual-level opt-in flag.

Possible public descriptors:

```c
typedef struct DvzVolumeOcclusionDesc
{
    bool enabled;
    float alpha_threshold;
    float fade_distance;
    float occluded_alpha;
} DvzVolumeOcclusionDesc;
```

Possible API:

```c
DVZ_EXPORT int dvz_panel_set_volume_occluder(
    DvzPanel* panel, DvzVisual* volume, const DvzVolumeOcclusionDesc* desc);

DVZ_EXPORT int dvz_visual_set_volume_occluded(DvzVisual* visual, bool enabled);
```

Initial defaults:

```text
enabled = true
alpha_threshold = 0.08
fade_distance = 0.08
occluded_alpha = 0.20
```

For a narrower first landing, these can start as internal/private helpers and become public after the
first visual result is validated.


## Scene State

Add retained state to `DvzPanel`:

```text
volume_occluder_visual
volume_occlusion_desc
```

Add retained state to `DvzVisual`:

```text
volume_occluded
```

Validation rules:

1. the panel occluder must be a volume visual;
2. the occluder must belong to the same panel;
3. unsupported target visual families should return an error or no-op with a warning;
4. only one volume occluder per panel is supported initially.


## Frame Plan and Frame Graph

Add a new render pass role:

```c
DVZ_FRAME_PLAN_RENDER_PASS_VOLUME_OCCLUSION
```

Add a per-panel transient resource:

```text
<panel_id>.volume_occlusion_depth
```

Resource properties:

```text
kind = texture
format = R32_SFLOAT
extent = panel or figure
usage = color attachment | sampled
lifetime = per-frame
```

Frame graph dependencies:

```text
volume_occlusion pass writes <panel>.volume_occlusion_depth
embedded visual passes sample <panel>.volume_occlusion_depth
```

The prepass should be emitted only if:

1. the panel has an enabled volume occluder;
2. the panel has at least one visible supported `volume_occluded` visual;
3. capabilities support the required render-target format and sampled render targets.


## DRP2 Emission

DRP2 should not need semantic runtime changes. The feature should lower to existing commands:

1. create transient `R32_SFLOAT` texture;
2. create or reuse volume occlusion pipeline;
3. begin render pass targeting the occlusion texture;
4. render the volume box with the occlusion shader;
5. create bind groups that expose the texture to supported target visual pipelines;
6. render normal scene passes.

The runtime should see ordinary texture, bind-group, pipeline, render-pass, and draw commands.


## Shader Work

Add one volume prepass shader:

```text
volume_occlusion_depth.frag
```

It should reuse as much as possible from existing volume ray-box and texture sampling logic.

Add a shared shader helper for target visuals:

```glsl
float dvz_volume_occlusion_factor(
    vec2 screen_uv,
    float fragment_depth,
    sampler2D volume_depth,
    DvzVolumeOcclusionParams params);
```

Then each supported visual shader applies:

```glsl
color.a *= dvz_volume_occlusion_factor(...);
```

Required uniform parameters:

```text
viewport_size
alpha_threshold
fade_distance
occluded_alpha
depth convention / near-far or matrix values
```

Prefer linear view depth for first implementation because fade distances are easier to tune than with
nonlinear clip depth.


## Visual Family Rollout

Do not update every visual family in the first implementation.

Recommended order:

1. **Volume slice visual**
   - Highest value for the Allen example.
   - Closest to the existing volume shader family.

2. **Mesh / primitive visual**
   - Supports embedded cubes, surfaces, probes, and atlas meshes.
   - One material shader family covers a broad set of use cases.

3. **Point visual**
   - Useful for neuron/probe-site markers.

4. **Image / textured quad visual**
   - Useful for arbitrary planes inside volumes.

Unsupported visual families should fail clearly when `volume_occluded` is requested.


## Allen Example Integration

In `examples/c/allen_mouse_brain_slice_glfw.c`, enable the feature for the embedded slice first:

```c
dvz_panel_set_volume_occluder(
    panel,
    volume_3d,
    &(DvzVolumeOcclusionDesc){
        .enabled = true,
        .alpha_threshold = 0.08f,
        .fade_distance = 0.08f,
        .occluded_alpha = 0.20f,
    });

dvz_visual_set_volume_occluded(volume_slice, true);
```

Add GUI controls:

```text
Volume hides slice
Occlusion threshold
Occlusion fade distance
Hidden slice alpha
```

Keep atlas mesh occlusion disabled until the mesh/primitive shader family supports it.


## Tests

Add focused tests before relying on the live example.

Suggested coverage:

1. panel state accepts a volume visual as occluder;
2. non-volume occluders are rejected;
3. visual `volume_occluded` flag is retained;
4. frame plan emits a `VOLUME_OCCLUSION` pass only when both source and target exist;
5. frame graph contains the transient occlusion texture and sampled dependency;
6. DRP2 stream contains texture creation, render pass, and target bind group;
7. unsupported visual families return a clear error or warning;
8. Allen example one-frame smoke run succeeds.

Validation commands:

```bash
just build
./build/testing/dvztest_scene volume
./build/examples/c/allen_mouse_brain_slice_glfw 1 --downsample=2
git diff --check
```


## Commit Plan

Use a short feature series rather than one large commit:

1. **Scene state and API**
   - Add descriptor, panel occluder state, visual opt-in state, and state tests.

2. **Frame plan role and graph resource**
   - Add pass role, transient resource, and graph dependency tests.

3. **Volume occlusion prepass shader and pipeline**
   - Emit first-hit depth texture from the volume occluder.

4. **Slice shader sampling**
   - Add occlusion bind group and attenuation logic for volume slice visuals.

5. **Allen example integration**
   - Enable slice occlusion and add GUI controls.

6. **Mesh/primitive follow-up**
   - Add occlusion support for embedded atlas meshes or probe/cube meshes after the slice path is
     visually validated.


## Open Questions

1. Should occlusion depth be measured in linear view space or normalized device depth?
2. Should the occlusion texture be panel-sized or figure-sized with panel viewport/scissor mapping?
3. Should volume alpha threshold use raw sampled alpha, post-opacity alpha, or full accumulated alpha?
4. How should WBOIT and source-over blended panels share the occlusion texture and pass ordering?
5. Should unsupported visual families fail hard or silently render without occlusion?
6. Should this be public API in the first landing or private/internal until validated in the Allen
   example?
