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
| `1` | Volume | `0`: volume texture, `1`: sampler, `2`: volume/state uniform, `3`: optional depth texture |
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
