# Scene SSAO Quality Plan

> **Execution Status**
> - **Status:** `PLANNING NOTE`
> - **Updated on:** `2026-05-16`
> - **Purpose:** define the next quality upgrades for the landed scene SSAO slice without
>   replacing the active FramePlan graph and DRP2/vklite runtime path.


## Context

The current SSAO implementation is intentionally minimal. It validates the graph shape and runtime
mechanics:

1. eligible visuals write normal/depth information into a G-buffer pass;
2. a fullscreen SSAO pass samples those textures;
3. a fullscreen composite pass darkens the final target;
4. sphere impostors now feed analytic normals and a linear view-distance value.

That is a useful first slice, but it is not yet a high-quality SSAO algorithm. The current shader
still uses a fixed 2D screen-space kernel and simple depth/normal comparisons. It should be treated
as a runtime foundation, not the final visual target.


## Quality Target

The target should be stable, smooth, depth-aware SSAO suitable for dense scientific scenes:

1. sphere clouds and mesh cavities should show contact/cavity darkening;
2. occlusion strength should not collapse merely because the camera zooms or the scene moves deeper
   in clip space;
3. blur should smooth sample noise without bleeding across depth or normal discontinuities;
4. controls should have predictable semantics: radius in view/world units, bias in the same units,
   strength as a perceptual intensity.


## G-Buffer Upgrade

Move from normal plus view distance to explicit view-space reconstruction.

Preferred first robust shape:

1. sampled hardware depth remains available for depth tests and existing graph compatibility;
2. G-buffer normal texture stores encoded normal plus optional material/object metadata;
3. SSAO shader reconstructs view-space position from depth using inverse projection and viewport
   coordinates;
4. if reconstruction proves awkward with the current Vulkan clip mapping, add a dedicated
   `R16G16B16A16_SFLOAT` or `R32G32B32A32_SFLOAT` view-position target only as an intermediate
   debugging/quality step.

Long-term, reconstructing position is preferable to storing position:

1. lower bandwidth;
2. fewer graph color attachments;
3. same depth source as the actual rendered surface;
4. easier to keep resize and post-process paths consistent.


## Shader Algorithm

Replace the fixed 2D neighbor test with a normal-oriented hemisphere kernel:

1. reconstruct the current pixel view-space position `P`;
2. read/decode the current normal `N`;
3. build a tangent frame around `N`;
4. rotate the kernel per pixel using a small noise texture or deterministic hash;
5. for each hemisphere sample, compute a view-space sample position `P + radius * sample`;
6. project that sample position back to screen coordinates;
7. compare the sampled scene depth/position against the projected sample depth;
8. accumulate occlusion with range falloff and bias.

Kernel policy:

1. keep the public sample count in a small set such as `8`, `16`, `32`, `64`;
2. keep a fixed built-in kernel in shader constants or a small uniform buffer for the first upgrade;
3. later move kernels/noise into scene-owned resources if runtime resizing or quality presets need
   more control.


## Noise And Blur

Raw SSAO should be noisy if the sample count is not excessive. Add smoothing as a separate pass:

```text
GBUFFER -> SSAO_RAW -> SSAO_BILATERAL_BLUR -> COMPOSITE
```

Bilateral blur inputs:

1. raw AO texture;
2. depth texture;
3. normal texture;
4. blur params: radius, depth threshold, normal threshold.

Blur rules:

1. do not blur across large depth discontinuities;
2. do not blur across strong normal discontinuities;
3. keep the first blur separable only if it remains visibly correct;
4. otherwise use a compact 2D kernel with explicit depth/normal weights.

Use a small per-pixel rotation/noise source before increasing sample counts too far. A deterministic
hash based on pixel coordinates is acceptable for the first upgraded shader. A tiny tiled noise
texture is better if the hash creates stable patterning.


## Parameter Semantics

Current UI terms can remain, but their meaning should become stricter:

1. `radius`: view-space sampling radius, not a screen-pixel multiplier;
2. `bias`: minimum view-space separation before a sample can occlude;
3. `strength`: final occlusion contrast/intensity;
4. `sample_count`: number of hemisphere samples;
5. `blur`: optional, enabled by default for interactive examples once available.

Bias should be in the same view-space units as radius. The default should be small enough to keep
contact shadows but high enough to avoid self-occlusion acne on smooth spheres.


## FramePlan And Runtime Changes

The existing graph-backed SSAO path should be extended rather than replaced.

Expected changes:

1. add an optional SSAO blur render role and graph pass;
2. add a blur output graph texture when blur is enabled;
3. keep SSAO sampled bind groups based on graph resource ids, relying on DRP2 descriptor refresh
   for texture recreation;
4. add inverse projection or reconstruction parameters to the SSAO params uniform;
5. pass viewport/scissor information to the SSAO shader consistently with per-panel rendering.

Do not create a private Vulkan SSAO path. All work should lower through FramePlan and DRP2.


## Example Changes

`hello_sphere_ssao_glfw` should become the visual tuning surface:

1. keep toggles for SSAO and auto-rotation;
2. keep sphere size and material controls;
3. expose radius, bias, strength, sample count, and blur;
4. optionally expose a quality preset selector: `fast`, `balanced`, `high`.

The example should include enough overlapping spheres at different depths to reveal whether
occlusion remains stable while zooming.


## Implementation Order

Recommended commits:

1. Add inverse projection / viewport reconstruction parameters to the SSAO uniform upload path.
2. Rework the SSAO shader to reconstruct view-space position from depth.
3. Replace the fixed 2D kernel with a normal-oriented hemisphere kernel.
4. Add deterministic per-pixel kernel rotation.
5. Add the bilateral blur graph pass and shader.
6. Update the example UI defaults and labels.
7. Add offscreen smoke coverage comparing SSAO enabled, disabled, and blur enabled.


## Validation

Focused validation:

```text
cmake --build build --target dvztest_scene hello_sphere_ssao_glfw -j 8
./build/testing/dvztest_scene test_scene_ssao
./build/testing/dvztest_scene test_scene_sphere_ssao_glsl_executes
./build/examples/c/hello_sphere_ssao_glfw 2
git diff --check
```

Before public API changes:

```text
just test scene
just spec-check
```

