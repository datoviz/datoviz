# Visual/Shader Refactor Implementor Notes

Status: implementation-facing notes for the active `src/scene` visual -> shader -> DRP2 path.
Keep this file in sync when adding visual families, shader variants, or bind-group layouts.

## Current flow

1. Retained visuals write typed metadata and resources into a `DvzFramePlan`.
2. `src/scene/visual_pipeline.c` resolves a `DvzSceneVisualDesc` from metadata/resources.
3. The same file resolves shader identity, vertex layout, pass capabilities, and bind requirements.
4. `src/scene/frame_plan_runtime.c` emits DRP2 resources, shader modules, bind-group layouts,
   pipelines, bind groups, and draw commands.
5. Runtime execution stays below DRP2/vklite/canvas; scene must not own swapchain or command-buffer
   lifetimes.

## Shader ABI

### Vertex attributes

Locations and bindings must match `_scene_visual_pipeline_desc()` and shader source exactly.

| Family | Location/binding ABI |
| --- | --- |
| Point, pixel, sphere | `0`: `vec3 position`, `1`: `rgba8 color`, `2`: `float size` |
| Point/pixel picking | `0`: `vec3 position`, `2`: `float size`; item id is derived from vertex index |
| Marker | `0`: `vec3 position`, `1`: `rgba8 color`, `2`: `float size`, `3`: `float angle`, `4`: `uint32 shape` |
| Primitive/mesh/path-strip | `0`: `vec3 position`, `1`: `rgba8 color`, optional `2`: `vec3 normal` |
| Segment/path-stroke | `0`: `vec3 start`, `1`: `vec3 end`, `2`: `rgba8 color`, `3`: `float width` |
| Image | `0`: `vec3 position`, `1`: `vec2 texcoord` |
| Volume | `0`: `vec3 position`, `1`: `vec3 texcoord` |

Use the same `VkFormat`, stride, binding, and location in shader inputs and DRP2 pipeline metadata.
Picking variants may intentionally drop color/material bindings; keep that special case explicit.

### Bind groups

Set ordering is part of the scene shader ABI and is mirrored by `src/scene/_scene_shader_abi.h`:

| Set | Layout | Bindings |
| --- | --- | --- |
| `0` | Scene common | `0`: `DvzMVP` uniform, `1`: `DvzSceneViewportUniform` uniform |
| `1` | Material | `0`: `DvzSceneMaterialParams` uniform |
| `1` | Image | `0`: sampled texture, `1`: sampler |
| `1` | Volume | `0`: volume texture, `1`: sampler, `2`: volume/state uniform, `3`: optional depth texture, `4`: scalar transfer texture |
| `2` | Scene occlusion | `0`: depth texture, `1`: sampler, `2`: occlusion uniform |

Only one set-1 layout is used by a visual pipeline. Scene occlusion moves to set `2` when another set
`1` layout is already present. Fixed-controller visuals still use set `0`, but with identity MVP data.

### Shader identities and cache keys

Each built-in shader variant needs:

- stable `builtin_family` such as `scene.point`, `scene.primitive`, or `scene.volume`;
- stable `builtin_variant` such as `default`, `pick`, `lit`, `depth_cue`, `wboit`, or `mip`;
- deterministic vertex/fragment/pipeline cache keys that include shader format and topology when needed;
- WGSL and GLSL source pointers, plus SPIR-V resource keys when precompiled SPIR-V exists.

Do not reuse a cache key for a different ABI, bind layout, topology, blending, depth state, or shader
format.


## Item-Range Lowering Notes

Retained visual item ranges should lower through existing draw command offset/count fields whenever
possible. They should not require a new shader variant or DRP2 command for the point first slice.

Implementation checks:

1. native point-list range should update draw `first_vertex` and `vertex_count`;
2. instanced billboard range should update draw `first_instance` and `instance_count`;
3. picking variants must preserve global logical item ids;
4. if a shader currently derives item id from local vertex or instance index, ranged draws may need
   a small base-item parameter or adjusted draw semantics;
5. do not add a visual modifier, scalar predicate, or custom shader path as part of the item-range
   slice.

Any ABI-affecting base-item parameter must be reflected in shader and pipeline cache keys. If point
rendering can preserve item identity using existing draw builtins, prefer that path for the RC
slice.


## WGSL parity lanes

Current committed WGSL coverage is intentionally narrower than GLSL runtime coverage. Treat this
table as the implementation queue for portable visual work:

| Lane | Current status | Next implementation step |
| --- | --- | --- |
| Pixel/point | WGSL instanced-quads path is active | Keep parity when changing style, cue, or picking ABI |
| Primitive/image | WGSL default/lit/image paths are active | Keep texture/sampler and material ABI aligned with GLSL |
| Marker | GLSL runtime path is active; WGSL source/lowering is deferred | Add instanced-quad WGSL lowering, material style binding, and fixture tests |
| Segment/path stroke | GLSL analytic stroke path is active; WGSL is deferred | Port stroke vertex math and cap/style material binding to WGSL |
| Sphere | GLSL impostor path is active; WGSL is deferred | Decide WebGPU impostor lowering and depth behavior before adding source |
| Volume | GLSL slice/MIP/composite paths are active; WGSL is deferred | Port transfer-texture and clipping-plane ABI once GLSL semantics settle |
| Advanced passes | WBOIT, depth peel, SSAO, EDL are GLSL/native only | Add capability-gated WGSL variants only when backend support exists |

When adding a WGSL lane, do not silently fall back to GLSL or browser-side shader replacement. Add
committed WGSL source, registry coverage, DRP2 emission tests, and a runtime or fixture smoke when
the backend can execute it.

Ownership boundary:

1. scene owns built-in shader semantics and committed GLSL/WGSL source variants;
2. scene-to-DRP2 emission selects one backend-ready shader format for the target stream;
3. DRP2 transports selected shader modules;
4. WebGPU accepts WGSL and rejects unsupported shader formats;
5. browsers must not translate GLSL to WGSL or replace scene shaders by hash.

Remaining cleanup should extract material, image, volume, and sampled-pass bind helpers from runtime
emission only when persistent object keys remain stable. Repeated table-shaped switch logic should
move to small internal descriptors instead of growing open-coded family switches. Unsupported WGSL
combinations should fail with diagnostics before a missing shader source pointer fails late.

## Add a visual family

1. Add/extend the retained visual state and public API only if the family is in the active v0.4 slice.
2. Emit typed FramePlan metadata/resource roles for all required attributes; avoid relying on legacy tags
   for new code.
3. Add a `DvzSceneVisualDescKind` and teach visual metadata lowering to produce it.
4. Add pass-capability rules: depth write/test, transparency path, post-process eligibility, material and
   texture usage, and fixed-controller behavior.
5. Add shader resolution and pipeline resolution in `visual_pipeline.c`.
6. Add bind resolution and DRP2 bind-group creation/binding in `frame_plan_runtime.c` if existing layouts
   are not sufficient.
7. Add or update focused fixtures/tests before broad examples.

## Add a shader variant

1. Define the feature condition in `_scene_shader_features_resolve()` or the relevant pass path.
2. Add a `DvzSceneBuiltinShader` entry only when a distinct source module is required.
3. Register WGSL/GLSL source in `shader_registry.c`; add SPIR-V keys if precompiled assets exist.
4. Return distinct shader and pipeline keys from `_scene_visual_shader_desc()`.
5. Confirm `_scene_visual_pipeline_desc()` describes the exact vertex, bind, depth, raster, and blend ABI.
6. Keep variant names semantic (`pick`, `lit`, `style_depth_cue`) rather than backend-specific.

## Descriptor checklist

Before a new family/variant is considered wired, verify:

- FramePlan resource roles, byte sizes, item strides, topology, index format/count, and ownership are set.
- Vertex attribute count, locations, bindings, formats, offsets, and strides match shader source.
- Required bind-group layouts are created once with stable keys and correct visibility.
- Bind groups point at live resources, correct offsets/sizes, and are not created for missing optional data.
- Pipeline key changes when any ABI-relevant state changes.
- Depth, raster, cull/front-face, alpha-to-coverage, WBOIT/depth-peel/picking modes are explicit.
- Scene/common/material/image/volume/occlusion set numbers match shader declarations.

## Validation checklist

Run the narrowest useful loop, then broaden only as needed:

- `git diff --check` for any documentation/code patch.
- `just build` after code or shader changes.
- `just test scene` for scene visual, FramePlan, and emitter changes.
- `just spec-check` after DRP2 fixture/schema changes.
- Vulkan validation-layer smoke tests for `vk`, `vklite`, `canvas`, `drp2`, `scene`, or shader ABI changes.
- Use `DVZ_DRP2_TRACE=1` or `DVZ_DRP2_TRACE=full` when investigating unexpected app-frame stream churn.

For docs-only updates, do not touch source code; record changed documentation files in the final summary.
