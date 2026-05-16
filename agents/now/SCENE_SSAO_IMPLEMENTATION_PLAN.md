# Scene SSAO Implementation Plan

> **Execution Status**
> - **Status:** `PLANNING NOTE`
> - **Updated on:** `2026-05-16`
> - **Purpose:** keep the SSAO integration path aligned with the current scene FramePlan graph,
>   runtime graph-resource emission, and the landed DRP2/vklite descriptor refresh path.


## Context

SSAO should be integrated through the active scene -> FramePlan graph -> DRP2 -> vklite runtime
path. Do not add a parallel renderer, presentation layer, or ad-hoc Vulkan path for scene SSAO.

The low-level mechanics already have a narrow vklite prototype:

- `src/vklite/tests/test_techniques.c:test_technique_ssao`
- `src/vklite/tests/shaders/ssao.frag`
- `src/vklite/tests/shaders/ssao_depth.frag`
- `src/vklite/tests/shaders/fullscreen.vert`

That prototype proves the Vulkan mechanics: create a sampled depth image, render into it, transition
it to shader-read layout, then run a fullscreen pass. It is not a production scene implementation:
the shader has hardcoded resolution, uses depth only, and does not reconstruct view-space position
or use normals.

The closest scene-level precedents are now WBOIT and retained depth peeling. They both split one
panel into multiple render nodes, declare the intermediate resources and pass dependencies in the
FramePlan graph, then let `frame_plan_runtime.c` resolve graph resource ids to DRP2 texture ids.
Use these files as the model:

- `src/scene/_frame_plan.h`
- `src/scene/frame_plan.c`
- `src/scene/scene_emit.c`
- `src/scene/frame_plan_runtime.c`
- `src/scene/shader_registry.c`
- `src/scene/tests/frame_plan.c`
- `src/scene/tests/scene_graph.c`


## Current FramePlan Graph Contract

The FramePlan is no longer just an ordered list of render roles. It now carries a compact frame
graph:

- `DvzFrameGraphResource`: logical resources with kind, format, extent kind, usage flags, and
  lifetime.
- `DvzFrameGraphPass`: render/compute/copy/readback/clear pass descriptors with explicit reads,
  writes, color attachments, depth attachments, viewport/scissor metadata, and a `work_label`.
- `DvzFrameGraphDependency`: derived producer/consumer dependencies exposed by
  `dvz_frame_plan_graph_dependency_get()`.
- Graph resource usage flags are translated into DRP2 texture usage, and declared reads add
  sampled/storage/copy usage in `_graph_declared_texture_usage_to_drp2()`.

Render nodes still carry `DvzFramePlanRenderPassRole`. Treat those roles as the bridge from retained
scene visuals to executable DRP2 work, but treat the graph as the authoritative resource/pass
description. For SSAO, add roles only for runtime dispatch and tests; also add graph resources,
graph passes, and dependencies.

Important current limits:

- `DVZ_FRAME_PLAN_MAX_GRAPH_COLOR_ATTACHMENTS` is `4`, so a base color + normal + linear-depth
  gbuffer fits without changing graph capacity.
- Existing WBOIT/depth-peel graph resources use `DVZ_FRAME_GRAPH_EXTENT_FIGURE` and panel
  viewport/scissor rectangles for per-panel work. Follow that first unless panel-sized graph
  allocation is deliberately implemented and tested.
- Existing graph-backed intermediates use `DVZ_FRAME_GRAPH_RESOURCE_LIFETIME_PER_FRAME`.
- Graph pass ordering is used by `frame_plan_runtime.c` when `dvz_frame_plan_graph_pass_count() > 0`.


## Target Pipeline

The minimum useful scene SSAO pipeline is:

```text
GBUFFER PASS
  outputs:
    base color texture
    normal texture
    linear depth texture
  depth:
    graph-declared D32 depth attachment for depth testing

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
    final panel color target (`rt`)
```

Start without blur. The first slice should be gbuffer -> SSAO -> composite.


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


## FramePlan Graph Changes

Extend `DvzFramePlanRenderPassRole` with SSAO-specific roles for runtime dispatch:

```c
DVZ_FRAME_PLAN_RENDER_PASS_SSAO_GBUFFER,
DVZ_FRAME_PLAN_RENDER_PASS_SSAO,
DVZ_FRAME_PLAN_RENDER_PASS_SSAO_BLUR,
DVZ_FRAME_PLAN_RENDER_PASS_SSAO_COMPOSITE,
```

Then add a graph builder in `scene_emit.c`, parallel to `_scene_emit_wboit_frame_graph()` and
`_scene_emit_depth_peel_frame_graph()`, for example `_scene_emit_ssao_frame_graph()`.

Suggested graph resource ids for panel `figure_0_p0`:

- `figure_0_p0.ssao.color`
- `figure_0_p0.ssao.normal`
- `figure_0_p0.ssao.linear_depth`
- `figure_0_p0.ssao.depth`
- `figure_0_p0.ssao.ao`
- `figure_0_p0.ssao.blur` later, only when blur is enabled
- borrowed `rt`

Suggested resource declarations:

- color: `TEXTURE`, `VK_FORMAT_R8G8B8A8_UNORM`, `COLOR_ATTACHMENT | SAMPLED`, `PER_FRAME`
- normal: `TEXTURE`, `VK_FORMAT_R16G16B16A16_SFLOAT`, `COLOR_ATTACHMENT | SAMPLED`, `PER_FRAME`
- linear depth: `TEXTURE`, `VK_FORMAT_R32_SFLOAT`, `COLOR_ATTACHMENT | SAMPLED`, `PER_FRAME`
- graph depth: `TEXTURE`, `VK_FORMAT_D32_SFLOAT`, `DEPTH_ATTACHMENT`, `PER_FRAME`
- AO: `TEXTURE`, `VK_FORMAT_R8_UNORM` or `VK_FORMAT_R16_SFLOAT`,
  `COLOR_ATTACHMENT | SAMPLED`, `PER_FRAME`
- `rt`: `EXTERNAL_TARGET`, `COLOR_ATTACHMENT | COPY_SRC`, `BORROWED`

Suggested pass shape:

1. `ssao_gbuffer`: render pass with three color attachments and the graph depth attachment. It
   draws only SSAO-applicable opaque visuals.
2. `ssao`: render pass with `reads = normal, linear_depth` and one AO color attachment. It draws a
   fullscreen triangle.
3. `ssao_blur`: deferred first; when added it reads AO and writes blur.
4. `ssao_composite`: render pass with `reads = color, ao_or_blur` and `rt` as a loaded color
   attachment. It draws a fullscreen triangle into the final panel region.

For the first implementation, support mesh and primitive visuals with normals. Points, images, text,
volume, and fixed-overlay visuals can remain on the ordinary final-target path until the composition
policy is deliberately broadened.


## Runtime Emission Changes

Add an SSAO target bundle in `frame_plan_runtime.c`, equivalent in spirit to `SceneWboitTargets`
and `SceneDepthPeelTargets`. It should contain:

- runtime texture ids for color, normal, linear depth, graph depth, AO, and optional blur;
- `SceneGraphRuntimeTargets graph` for graph resource id -> runtime texture id lookup;
- sampler id;
- bind-group layout ids for SSAO and composite;
- bind-group ids for SSAO and composite;
- fullscreen pipeline ids.

Resolve graph resources with `_graph_resolve_texture_2d()` instead of hardcoding private texture
creation. Add every resolved graph texture to `SceneGraphRuntimeTargets`, then use
`_graph_sampled_read_texture_id()` / `_graph_color_attachment_texture_id()` to bind the graph reads
and attachments.

The first runtime path can mirror the existing WBOIT/depth-peel special cases:

- include SSAO roles in the multi-pass detection helper currently named `_plan_has_wboit_roles()`;
- prepare render batches for `SSAO_GBUFFER`;
- prepare SSAO fullscreen resources before graph-order execution;
- branch on `SSAO_GBUFFER`, `SSAO`, `SSAO_COMPOSITE`, and later `SSAO_BLUR` in graph pass order.

Consider renaming `_plan_has_wboit_roles()` and `_emitter_emit_scene_wboit_renders()` once SSAO
lands, because they will become the generic scene multi-pass runtime path.


## Descriptor Refresh Coordination

`agents/now/DRP2_DESCRIPTOR_REFRESH_PLAN.md` has now landed its first texture-recreation slice in
the DRP2/vklite runtime. SSAO should rely on the runtime invariant from that plan:

> A live bind group in the runtime must always describe the current backend handles of every
> resource id it references.

Current runtime shape:

- `_vklite_build_bind_group_descriptors()` centralizes descriptor wrapper creation from saved
  bind-group entries.
- `_vklite_refresh_dependent_bind_groups()` walks live bind groups that reference a recreated
  resource id, rebuilds their descriptor wrappers, swaps them onto the bind-group object, and retires
  the old wrapper.
- `_vklite_create_texture()` calls that refresh helper after replacing an existing texture id.
- Retired wrappers go through the existing borrowed-command-buffer deferred-destroy path when a
  borrowed frame command buffer is active.

Do not create an SSAO-specific descriptor freshness mechanism. Cache SSAO bind groups by semantic
resource ids and binding shape. WBOIT and depth peeling still have local bind-group fingerprints that
include texture ids, sampler ids, and target extent as tactical guardrails; SSAO should not add a
third copy of that pattern unless a new runtime gap is demonstrated.

SSAO resize tests should include the stable-id texture recreation path and should pass without
re-emitting SSAO bind groups solely because the target extent changed. If buffer or sampler
recreation becomes part of the SSAO path, verify whether the generic refresh has been generalized
beyond texture recreation before relying on stable ids for those resource kinds.


## Shader Work

Add built-in scene shaders in the shader registry:

- gbuffer mesh/primitive vertex and fragment variants,
- fullscreen SSAO fragment,
- optional fullscreen blur fragment,
- fullscreen composite fragment.

The gbuffer pass should use the graph-declared `VK_FORMAT_D32_SFLOAT` depth attachment for depth
testing and additionally write sampled linear depth into a color attachment. Do not start by
sampling an implicit transient depth attachment from
`dvz_drp2_stream_begin_render_pass_set_depth()`; graph-declared sampled attachments are now the
right contract for multi-pass scene techniques.

The SSAO shader should take viewport size, radius, bias, intensity, sample count, and
projection/reconstruction data through a uniform buffer.

Runtime-generated data needed by the SSAO pass:

- `ssao_kernel`: float32 sample vectors, default 16 samples,
- `ssao_noise_texture`: small random tangent-space rotation texture, default 4x4.


## DRP2 / Runtime Work

Most required DRP2 primitives already exist:

- multi-color render attachments,
- graph-declared depth attachments,
- sampled textures,
- samplers,
- bind-group layouts and bind groups,
- fullscreen triangle draws,
- render-target-to-sampled transitions driven by declared access.

Remaining DRP2 gaps to close or deliberately defer before the first SSAO slice:

1. **Sampler configuration.** DRP2 `CreateSampler` currently creates a fixed linear,
   clamp-to-edge sampler. SSAO noise usually wants nearest filtering and may want repeat wrapping.
   Strategy: either extend DRP2 with a narrow sampler descriptor command/API before the SSAO noise
   texture lands, or choose a first shader path that does not rely on repeat/nearest noise sampling.
   If the latter is chosen, record it as a deliberate first-slice simplification.
2. **Single-channel render-target format serialization.** Runtime pipeline creation accepts
   backend VkFormat color targets, but the DRP2 JSON/spec output should explicitly name every SSAO
   render-target format used by tests. Add `VK_FORMAT_R32_SFLOAT` for linear depth and the chosen
   AO format (`VK_FORMAT_R8_UNORM` or `VK_FORMAT_R16_SFLOAT`) to pipeline color-target
   serialization before adding SSAO fixtures that inspect JSON.
3. **Focused format/runtime coverage.** Add a DRP2 fixture/test that creates a three-output gbuffer
   pass with `RGBA8`, `RGBA16F`, and `R32F` color targets, plus an AO fullscreen pass with the
   chosen AO format. Validate the semantic stream and execute it through the vklite semantic runtime
   at minimum.
4. **Descriptor refresh regression coverage for SSAO-shaped bindings.** Descriptor refresh now
   handles recreated buffers, textures, and samplers in the vklite runtime. Add one SSAO-shaped
   resize test that recreates a stable graph texture id after its bind group exists and verifies the
   subsequent fullscreen pass does not require re-emitting the bind group solely for descriptor
   freshness.
5. **Capability reporting.** Extend scene capability validation once final formats are chosen so
   unsupported SSAO paths fail with explicit diagnostics rather than relying on Vulkan creation
   failure.

The color-attachment count itself is not a blocker: `DVZ_DRP2_MAX_COLOR_ATTACHMENTS` and
`DVZ_FRAME_PLAN_MAX_GRAPH_COLOR_ATTACHMENTS` are both `4`, so the three-output gbuffer fits the
current command and graph limits. Named graph depth attachments, sampled texture bindings, fullscreen
draws, and render-target-to-sampled transitions are already present.

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

Diagnostics should be explicit, like the current WBOIT and depth-peeling messages.


## Tests

Add tests in increasing cost order:

1. `frame_plan` graph-shape test: an SSAO panel graph declares color, normal, linear depth, graph
   depth, AO, and the expected pass dependencies.
2. `scene` command-shape test: enabling panel SSAO emits gbuffer, SSAO, and composite render roles
   plus matching graph passes.
3. `scene` DRP2 emission test: stream contains graph-created gbuffer/AO textures, SSAO/composite
   bind groups, fullscreen draws, declared depth, and validates.
4. Semantic runtime resize test: repeated emits with the same runtime scope and a different target
   extent recreate SSAO textures and still execute through the vklite semantic runtime without
   SSAO-local descriptor fingerprints.
5. Runtime GPU smoke: offscreen mesh with SSAO enabled executes through vklite without validation
   errors and produces nonblank pixels.
6. Toggle test: rendering with SSAO enabled changes the captured image compared with disabled SSAO.

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
2. SSAO render roles plus `ssao_gbuffer`, `ssao`, and `ssao_composite` graph passes.
3. Mesh/primitive-with-normals gbuffer path.
4. Graph-declared color, normal, linear-depth, depth, and AO textures.
5. Fullscreen SSAO pass and composite pass in GLSL.
6. Scene graph-shape test, DRP2 command-shape test, resize semantic-runtime test, and one GPU smoke
   test.

Defer blur, WebGPU/WGSL parity, point/image/volume participation, generated normals, GUI controls,
panel-sized graph allocation, and a fully generic public framegraph until the first retained scene
SSAO path is executing reliably.
