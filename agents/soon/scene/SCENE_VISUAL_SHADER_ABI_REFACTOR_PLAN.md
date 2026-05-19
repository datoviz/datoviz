# Scene Visual Shader ABI Refactor Plan

> **Execution Status**
> - **Status:** `FIRST ABI PASS LANDED; WGSL PARITY CONTINUES`
> - **Created on:** `2026-05-16`
> - **Updated on:** `2026-05-17`
> - **Scope:** built-in scene visual shaders, common visual bindings, visual pipeline descriptors,
>   and GLSL/WGSL alignment for the scene -> FramePlan -> DRP2 path.


## Goal

Make built-in scene visuals easier to extend across Vulkan and WebGPU by turning the current
implicit shader ABI into a small, documented, test-covered contract.

The immediate motivation is duplicated shader code: many built-in GLSL and WGSL vertex shaders
redeclare the same MVP, viewport, transform, and sometimes material helper logic. The C side already
has a strong start with `scene_common_bindings.c`, `visual_pipeline.c`, and the shader registry; the
next step is to make those helpers the single source of truth for both shader languages and the DRP2
stream shape.

This plan is intentionally behavior-preserving at first. It should not fork scene semantics for
WebGPU, should not introduce a public generic binding API, and should not create a parallel renderer
contract.


## Progress Update

The first behavior-preserving ABI cleanup pass has landed:

1. `_scene_shader_abi.h` records the common, visual, image, volume, and occlusion set/binding
   constants used by runtime emission;
2. `src/scene/glsl/common.glsl` and `src/scene/wgsl/common.wgsl` share the common MVP/viewport
   transform ABI across the active point, pixel, primitive, image, sphere, marker, segment, and
   volume vertex paths;
3. image shaders now use split texture/sampler bindings in both GLSL and WGSL;
4. `visual_pipeline.c` centralizes visual vertex attribute descriptor writes and consolidated
   depth-state decisions;
5. `frame_plan_runtime.c` centralizes runtime pipeline bind-layout ordering for common/material,
   image, volume, and occlusion layouts;
6. `just shader-abi-check` validates the documented shader ABI and should stay green whenever
   shader source, bind layouts, vertex attributes, or cache keys move.

The remaining work is no longer the initial ABI extraction. It is WGSL parity and incremental
descriptor-driven cleanup for visual families and techniques that still have native GLSL-only
runtime coverage.


## Current State

### What Is Already Good

1. `src/scene/scene_common_bindings.c` centralizes the common bind-group layout for retained visual
   rendering:
   - set 0, binding 0: `DvzMVP`
   - set 0, binding 1: `DvzSceneViewportUniform`
2. `src/scene/visual_pipeline.c` owns most per-visual decisions:
   - visual family classification,
   - shader key/source selection,
   - pipeline cache keys,
   - vertex-buffer layouts,
   - depth/blend/raster state,
   - bind-role selection.
3. `src/scene/shader_registry.c` hides generated GLSL/WGSL resource lookup from emitters.
4. The root CMake pipeline already embeds GLSL and WGSL text resources.
5. `cmake/embed_text_resources.cmake` already expands local `#include "..."` statements while
   embedding text resources, which gives us a practical shared-include mechanism for both GLSL and
   WGSL.
6. `tools/generate_missing_scene_wgsl.py` already exists as a conservative desktop-side maintenance
   tool for missing WGSL siblings.
7. The existing `SCENE_WGSL_SHADER_VARIANTS_PLAN.md` has the right ownership rule: scene owns shader
   variants, DRP2 transports backend-ready shader code, and WebGPU accepts WGSL rather than
   translating GLSL in the browser.


### Remaining Problems

1. `src/scene/frame_plan_runtime.c` still creates some material, image, volume, and technique bind
   groups inline, so visual-specific binding details remain partly spread across runtime emission.
2. `visual_pipeline.c` is a good centralization point, but parts of it are now table-shaped switch
   logic. That makes every new visual family or shader variant more likely to duplicate cache-key,
   shader-key, layout, and bind-role code.
3. WGSL coverage is intentionally incomplete for marker, segment/path stroke, sphere, volume, and
   advanced passes. That is acceptable, but the unsupported surface should be explicit and
   capability-gated.


## Non-Goals

1. Do not add a public generic binding API yet. Public visual setters should remain typed.
2. Do not move shader semantics into `examples/webgpu/` or browser-side replay code.
3. Do not require WebGPU to run every scene technique in the first refactor slice.
4. Do not force all point rendering onto instanced quads for Vulkan. Native point-list rendering
   remains valuable for high-throughput Vulkan paths.
5. Do not restructure `src/scene/glsl/` and `src/scene/wgsl/` into a new directory layout unless the
   shader build pipeline is being changed for another concrete reason.
6. Do not change scene public APIs as part of the first cleanup slices.


## Target Architecture

### Shader ABI Ownership

The scene module should own one internal shader ABI document in code:

1. common visual set:
   - set 0, binding 0: `DvzMVP`
   - set 0, binding 1: `DvzSceneViewportUniform`
2. material set:
   - set 1, binding 0: `DvzSceneMaterialParams`
3. image set:
   - set 1, binding 0: sampled texture
   - set 1, binding 1: sampler
4. volume set:
   - set 1, binding 0: volume texture
   - set 1, binding 1: sampler
   - set 1, binding 2: volume params
   - set 1, binding 3: sampled scene depth or dummy depth

At first this can be a private header, for example:

```text
src/scene/_scene_shader_abi.h
```

That header should expose C constants used by the emitter and comments matching the GLSL/WGSL
common files. The goal is not to generate shader source from C macros yet; the goal is to keep the
binding contract named, reviewed, and shared across C helpers.


### Common Shader Includes

Use embedded text-resource includes as the shared-source mechanism:

```text
src/scene/glsl/common.glsl
src/scene/wgsl/common.wgsl
src/scene/glsl/scene_material.glsl
src/scene/wgsl/scene_material.wgsl
```

Built-in vertex shaders that use the common visual set should include the common file instead of
redeclaring `MVP`, `Viewport`, and `transform()`.

Recommended shared WGSL helpers:

```wgsl
struct MVP {
    model: mat4x4f,
    view: mat4x4f,
    proj: mat4x4f,
    time: f32,
    flags: u32,
}

struct Viewport {
    rect: vec4f,
}

@group(0) @binding(0) var<uniform> mvp: MVP;
@group(0) @binding(1) var<uniform> viewport: Viewport;

fn transform(position: vec3f) -> vec4f {
    return mvp.proj * mvp.view * mvp.model * vec4f(position, 1.0);
}
```

If a Vulkan-style Y/depth conversion is needed in WGSL later, make that a named helper rather than
hand-coding it in visual shaders. Do not silently make GLSL and WGSL transforms diverge.


### Visual Pipeline Descriptor Ownership

`visual_pipeline.c` should remain the place where retained visual semantics become concrete draw
descriptors. The target is:

```text
retained visual + panel attachment + backend/capabilities
  -> visual descriptor
  -> shader variant descriptor
  -> vertex layout descriptor
  -> bind-role descriptor
  -> pass-state descriptor
```

`frame_plan_runtime.c` should consume those descriptors and emit DRP2 commands. It should not need
to know every visual-family rule.


### GLSL/WGSL Ownership

Keep the existing parallel source model:

```text
src/scene/glsl/<name>.<stage>
src/scene/wgsl/<name>.<stage>.wgsl
```

GLSL may remain the canonical native/Vulkan source where SPIR-V precompilation is available. WGSL
should be committed, reviewable, and emitted by scene for WebGPU streams. The browser path should
not translate GLSL or substitute scene shaders by hash.


## Refactor Slices

### Slice 1 - Common Shader Prelude

Purpose: remove direct MVP/viewport/transform duplication from built-in visual shaders.

Work:

1. Add `src/scene/wgsl/common.wgsl`.
2. Update simple GLSL vertex shaders to use `#include "common.glsl"`:
   - `point.vert`
   - `pixel.vert`
   - `point_cue.vert`
   - `pixel_cue.vert`
   - `primitive.vert`
   - `primitive_lit.vert`
   - `image.vert`
   - `volume_slice.vert`
   - sphere vertex shaders where the helper fits cleanly.
3. Update equivalent WGSL vertex shaders to use `#include "common.wgsl"`.
4. Keep shader behavior byte-for-byte equivalent where possible; when generated SPIR-V changes,
   verify the visual output path rather than assuming it is harmless.
5. Add or update focused tests that inspect emitted shader code for expanded common bindings if
   practical. If direct code inspection is brittle, rely on existing scene emit and fixture tests.

Validation:

1. `just build`
2. `just test test_scene_visual_common_binding_layout_order`
3. `just test test_scene_point_emit_glsl_native_points`
4. `just test test_scene_point_emit_wgsl_instanced_quads`
5. `just test test_scene_primitive_triangle_list_emit_wgsl`
6. `just test test_scene_image_emit_wgsl`
7. `python3 tools/generate_missing_scene_wgsl.py --check` as an informational coverage check.
8. `git diff --check`


### Slice 2 - Texture/Sampler ABI Alignment

Purpose: make image and sampled-pass shader bindings portable by default.

Work:

1. Convert GLSL image shaders from combined `sampler2D` to split `texture2D` + `sampler` bindings:
   - set 1, binding 0: texture
   - set 1, binding 1: sampler
2. Confirm that `dvz_drp2_stream_create_texture_sampler_bind_group_layout()` and
   `dvz_drp2_stream_create_texture_sampler_bind_group()` already emit the matching split binding
   shape for both native and WebGPU-oriented streams.
3. Audit sampled pass shaders that already use split texture/sampler style and document exceptions.
4. Avoid changing volume texture bindings until a focused volume WGSL slice is active, because volume
   still has additional depth and params bindings.

Validation:

1. `just build`
2. `just test test_scene_image_glsl_executes`
3. `just test test_scene_image_emit_wgsl`
4. `just test drp2` if DRP2 texture/sampler helpers change.
5. WebGPU fixture preflight for scene image fixtures when available.
6. `git diff --check`


### Slice 3 - Private Shader ABI Header

Purpose: remove magic binding numbers and layout assumptions from emitter code.

Work:

1. Add `src/scene/_scene_shader_abi.h`.
2. Define private constants for common, material, image, and volume set/binding indices.
3. Use these constants in:
   - `scene_common_bindings.c`
   - image bind-group helper code
   - material bind-group helper code
   - volume bind-group helper code
   - future sampled-pass helper code where applicable.
4. Keep shader files as readable source. Do not generate shader text from C macros in this slice.
5. Add comments in `common.glsl`, `common.wgsl`, `scene_material.glsl`, and `scene_material.wgsl`
   pointing to the private ABI header.

Validation:

1. `just build`
2. focused scene emit tests for common binding order and image/material visual paths.
3. `git diff --check`


### Slice 4 - Extract Visual Binding Helpers

Purpose: shrink `frame_plan_runtime.c` and make binding ownership explicit.

Work:

1. Create a private helper pair:

```text
src/scene/_scene_visual_bindings.h
src/scene/scene_visual_bindings.c
```

2. Move behavior-preserving helpers from `frame_plan_runtime.c`:
   - material bind-group layout and bind-group resolution,
   - image sampler/layout/bind-group resolution,
   - volume layout, dummy-depth texture, params uniform, sampler, and bind-group resolution.
3. Keep `scene_common_bindings.c` separate; it already has focused ownership.
4. Keep technique-specific sampled bindings in `frame_plan_runtime.c` for the first extraction unless
   moving them is purely mechanical and low risk.
5. Reuse `DvzSceneVisualBindDesc` as the input descriptor rather than re-reading visual family state.
6. Preserve persistent object keys exactly unless the slice explicitly adds tests for key changes.

Validation:

1. `just build`
2. `just test scene`
3. Narrow visual tests:
   - point
   - primitive
   - image
   - volume if active in the current test suite
4. `git diff --check`


### Slice 5 - Descriptor Table Cleanup In `visual_pipeline.c`

Purpose: reduce switch duplication after the ABI is stable.

Work:

1. Introduce small internal static descriptors for repeated vertex layouts:
   - position/color/size point-like layout,
   - position/color primitive layout,
   - position/color/normal primitive layout,
   - position/uv image layout,
   - position/uvw volume layout.
2. Keep dynamic decisions in code:
   - point native vs instanced-quad lowering,
   - picking color-as-id layout,
   - lit vs unlit shader variant,
   - depth-cue variant,
   - WBOIT/depth-peel/G-buffer pass role.
3. Replace repeated assignments to `strides`, `bindings`, `locations`, `formats`, and `offsets`
   with a helper that copies a descriptor into `DvzSceneVisualPipelineDesc`.
4. Add tests or assertions for descriptor capacity so future layouts cannot overflow the fixed
   arrays in `DvzSceneVisualPipelineDesc`.

Validation:

1. `just build`
2. `just test test_scene_visual_common_binding_layout_order`
3. `just test test_scene_point_emit_glsl_native_points`
4. `just test test_scene_point_emit_wgsl_instanced_quads`
5. `just test test_scene_primitive_triangle_list_emit_wgsl`
6. `just test scene`
7. `git diff --check`


### Slice 6 - WGSL Coverage And Capability Gates

Purpose: make unsupported WebGPU visual/technique paths explicit.

Work:

1. Keep `python3 tools/generate_missing_scene_wgsl.py --check` as the coverage report.
2. Add a curated expected-missing list for advanced techniques if the raw output is too noisy:
   - depth peeling,
   - WBOIT,
   - EDL,
   - SSAO,
   - G-buffer,
   - volume,
   - sphere until ported.
3. Make scene emission reject unsupported WGSL visual/technique combinations with diagnostics
   instead of failing later because a shader source pointer is `NULL`.
4. Add one focused test per unsupported active path that requests WGSL and verifies the diagnostic.
5. Keep generated or hand-written WGSL variants committed when a path is promoted.

Validation:

1. `python3 tools/generate_missing_scene_wgsl.py --check`
2. `just test scene`
3. `just webgpu-fixture-preflight` if available in the active workflow.
4. Browser fixture dashboard only when touching `examples/webgpu/`.
5. `git diff --check`


## Detailed Design Notes

### Common Transform Semantics

The GLSL common helper currently converts cglm/OpenGL-style NDC to Vulkan NDC:

```glsl
tr.y = -tr.y;
tr.z = 0.5 * (tr.z + tr.w);
```

WGSL shaders currently tend to use the direct MVP chain without that conversion. Before unifying
names across GLSL and WGSL, confirm the intended clip-space contract:

1. Native Vulkan GLSL path:
   - preserve current rendered output.
   - keep explicit Vulkan conversion if cglm matrices remain OpenGL-style.
2. WebGPU WGSL path:
   - WebGPU clip-space conventions differ from OpenGL and Vulkan details.
   - preserve current WebGPU fixture output first.
   - if conversion is needed, name it explicitly, for example `transform_vulkan_clip()` and
     `transform_webgpu_clip()`, rather than hiding divergent semantics behind the same function.

Do not make this a broad camera/controller rewrite. The first cleanup should only centralize the
contract that already exists.


### Point-Like Lowering

The current backend-aware decision is correct:

1. GLSL/Vulkan point-like visuals can use native point lists.
2. WGSL/WebGPU point-like visuals should use instanced quads because WebGPU does not expose
   programmable point size like Vulkan GLSL `gl_PointSize`.

Keep the public `dvz_point()` semantic stable. The concrete lowering belongs in
`visual_pipeline.c`, keyed by backend shader format and eventual backend capabilities. Future
pipeline keys should distinguish:

```text
scene.point/native_points/glsl
scene.point/instanced_quads/wgsl
```

The public scene visual should not expose this unless a real advanced override is needed.


### Material And Lighting Helpers

`scene_material.glsl` and `scene_material.wgsl` should remain the shared material/depth-cue helper
files. The C-side material uniform shape is `DvzSceneMaterialParams`; shader structs must stay in
lockstep with that type.

Immediate cleanup:

1. Ensure lit WGSL shaders include `scene_material.wgsl` instead of redeclaring a shorter material
   struct.
2. Keep depth-cue helpers in the shared material include.
3. Confirm camera-position handling in WGSL lit shaders; current simple WGSL lit vertex code uses a
   fixed camera position while GLSL derives it from `inverse(mvp.view)`. This should be corrected
   when lit WGSL paths move beyond fixture/portability coverage.


### Image And Sampled Resources

Use separate texture and sampler bindings as the scene convention. This aligns with WebGPU and also
works in Vulkan GLSL:

```glsl
layout(set = 1, binding = 0) uniform texture2D tex;
layout(set = 1, binding = 1) uniform sampler samp;

vec4 color = texture(sampler2D(tex, samp), uv);
```

Avoid adding new combined `sampler2D` bindings in scene-owned shaders intended to be portable.


### Runtime Emitter Boundaries

`frame_plan_runtime.c` should eventually read like pass orchestration:

1. prepare render targets,
2. prepare visual draw descriptors,
3. resolve bind groups through helpers,
4. create or reuse pipelines,
5. record render pass commands,
6. submit/copy/readback.

It should not own every bind-group layout literal. Extracting visual binding helpers before deeper
pipeline-table cleanup will make later reviews smaller and safer.


## Testing Strategy

Use focused validation per slice rather than always running the full graphics stack.

Minimum checks for documentation or mechanical shader include work:

```text
just build
just test scene
git diff --check
```

Additional checks for shader and runtime emitter changes:

```text
just test test_scene_point_emit_glsl_native_points
just test test_scene_point_emit_wgsl_instanced_quads
just test test_scene_primitive_triangle_list_emit_wgsl
just test test_scene_image_glsl_executes
just test test_scene_image_emit_wgsl
```

Additional checks for DRP2 binding or fixture contract changes:

```text
just test drp2
python3 tools/generate_missing_scene_wgsl.py --check
just webgpu-fixture-preflight
```

For changes that touch Vulkan resources, command buffers, render targets, synchronization, or
readback behavior, run the appropriate Vulkan validation smoke test or focused live example from an
unsandboxed graphics-capable shell.


## Acceptance Criteria

The refactor is complete for the first milestone when:

1. All active built-in visual vertex shaders use shared common GLSL/WGSL includes for MVP,
   viewport, and transform helpers where applicable.
2. The common set, material set, image set, and volume set binding numbers are named in one private
   C ABI header and reflected in shader comments.
3. Image GLSL and WGSL use the same split texture/sampler binding contract.
4. `frame_plan_runtime.c` no longer owns visual material/image/volume bind-group construction
   directly.
5. `visual_pipeline.c` uses small shared layout descriptors for repeated vertex layouts.
6. WGSL unsupported paths fail with clear scene-emission diagnostics instead of silent missing shader
   source failures.
7. Focused scene tests and DRP2 fixture/preflight checks pass for point, primitive, image, and any
   promoted WGSL visual paths.


## Suggested Commit Sequence

1. `scene shader abi: add common WGSL prelude`
2. `scene shaders: use common visual includes`
3. `scene shaders: split image GLSL texture sampler bindings`
4. `scene: name private shader ABI bindings`
5. `scene: extract visual bind-group helpers`
6. `scene: table repeated visual vertex layouts`
7. `scene wgsl: add explicit unsupported diagnostics`

Keep each commit behavior-preserving unless the commit title says otherwise. For any behavior change,
include the narrow test that fails before the change.
