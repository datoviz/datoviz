# Controller & Transform Architecture — v0.4

> **Status:** `IMPLEMENTED` — all action items complete as of 2026-05-07
> **Updated:** `2026-05-07`

---

## 1. Goal

Add interactive pan/zoom and arcball controllers to v0.4, fed by the existing `DvzInputRouter`
event system, with per-panel MVP data uploaded through a shared GPU uniform buffer.


## 2. Bind Group Layout

Two descriptor sets per visual pipeline:

| Set | Binding | Type | Content |
|-----|---------|------|---------|
| 0 | 0 | UNIFORM_BUFFER | MVP (`model`, `view`, `proj` mat4 + `time` float + `flags` uint) |
| 0 | 1 | UNIFORM_BUFFER | Viewport (pixel width, height, dpi) |
| 1 | 0+ | COMBINED_IMAGE_SAMPLER / STORAGE_BUFFER | Visual-specific params and samplers |

Set 0 is **panel-level**: same across all visuals in a panel, updated per controller frame.
Set 1 is **visual-level**: created once per visual, only re-uploaded when visual data changes.

The MVP struct is:
```c
typedef struct { mat4 model, view, proj; float time; uint32_t flags; } SceneMVP; // 192 bytes
```

`flags` bit 0 selects the transform type (see §4).


## 3. Shared UBO Pool

A single `VkBuffer` (~256 KB, `VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT`, HOST_VISIBLE | HOST_COHERENT)
lives in `DvzApp`. Sub-allocation is a bump pointer: each panel claims one slot of size
`sizeof(SceneMVP) * N_frames` at panel creation time.

- N bind group objects per panel (one per frame slot) are created at first emit.
- Per frame: one `WRITE_BUFFER` at `base_offset + frame_index * sizeof(SceneMVP)`.
- The bind group objects themselves are stable — no re-creation between frames.

This avoids per-frame `vkCreateBuffer` calls (which trigger Vulkan validation perf warnings).


## 4. Transform Pipeline

### GLSL layout (set 0, binding 0)
```glsl
layout(set=0, binding=0) uniform MVP { mat4 model, view, proj; float time; uint flags; } mvp;
layout(set=0, binding=1) uniform Viewport { float width, height, dpi; } viewport;
```

### Transform function chain
```glsl
vec4 scene_transform(vec4 pos) { return mvp.proj * mvp.view * mvp.model * pos; }
vec4 transform_margins(vec4 clip) { /* optional: adjust for panel margins */ return clip; }
vec4 to_vulkan(vec4 ndc) { ndc.y = -ndc.y; ndc.z = ndc.z * 0.5 + 0.5; return ndc; }
```

`scene_transform()` can be overridden per-shader for non-linear transforms (log scale, etc.).

### Transform type via specialization constants

```glsl
layout(constant_id = 0) const int transform_type = 0; // 0=MVP, 1=identity, 2=...
```

Vulkan: `VkSpecializationInfo` at pipeline creation. WebGPU: `override` constants (equivalent).
No per-vertex branch at runtime — baked at pipeline creation.


## 5. DRP2 Extensions Required

### a. Uniform buffer bind group layout
- Add `bool uniform_buffer` flag to `create_bind_group_layout` struct in `_stream.h`
- Runtime: create `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER` at VS+FS stage
- Add `dvz_drp2_stream_create_uniform_bind_group_layout()` to stream API

### b. Uniform buffer bind group with offset
- Add `uint64_t buffer0_offset` to `create_bind_group` struct
- Runtime: use `VkDescriptorBufferInfo.offset` with the provided sub-allocation offset

### c. Second bind group layout in CREATE_PIPELINE
- Add `bind_group_layout_id2` field to `create_render_pipeline` struct (optional, 0 = unused)
- Runtime: build `VkDescriptorSetLayout[2]` array for pipeline layout when id2 != 0

### d. PUSH_CONSTANTS command (deferred — not required for MVP/viewport UBO path)
- New DRP2 command `PUSH_CONSTANTS { pipeline_id, stage_flags, offset, size, data }`
- vklite: maps to `vkCmdPushConstants`
- WebGPU: ring-buffer UBO workaround until native push constants land


## 6. Shader Precompilation

Builtin visual shaders (point, primitive, image, …) should be compiled to SPIR-V at build time.
Runtime GLSL compilation (shaderc) is kept only for user-supplied custom shaders.

**Build-time pipeline:**
1. Write GLSL source files: `src/scene/glsl/common.glsl`, `point.vert`, `point.frag`, etc.
2. CMake rule: `glslc <src>.glsl -o <src>.spv` for each file.
3. Embed: `cmake/embed_resources.cmake` — `create_resources()` reads `.spv` as hex,
   generates `DVZ_RESOURCE_shader_xxx[]` arrays and `dvz_resource_shader(name, &size)`.
   (Same mechanism as v0.3 `embed_resources.cmake`, adapted for v0.4 paths.)
4. `converter.c` calls `dvz_resource_shader("point_vert", &size)` instead of inline GLSL macros.

The `#include "common.glsl"` pattern works via `glslc -I src/scene/glsl`.


## 7. Controller Porting Plan

### Panzoom
- Copy `src/scene/panzoom.c` + `include/datoviz/scene/panzoom.h` from v0.3
- Swap `DvzMouseEvent` → `DvzPointerEvent`; rename fields to match v0.4 input types
- Add `dvz_panzoom_connect(pz, DvzInputRouter*)` to subscribe to pointer events
- Output: `dvz_panzoom_mvp(pz, &mvp)` fills view+proj matrices (model stays identity)

### Arcball
- Copy `src/scene/arcball.c` + `include/datoviz/scene/arcball.h` from v0.3
- Same event adaption as panzoom
- State: accumulated `mat4 base` + in-flight `versor rotation`
- DRAG_STOP commits rotation into base, resets quaternion
- Output: `dvz_arcball_model(arc, &mat)` fills model matrix

### Panel API
```c
void dvz_panel_set_panzoom(DvzPanel* panel, int flags);   // creates panzoom, connects to router
void dvz_panel_set_arcball(DvzPanel* panel, int flags);   // creates arcball, connects to router
```

Frame emit reads controller state → uploads MVP UBO → emits DRP2 stream.


## 8. Ordered Action Items

1. **Docs commit** — save this file + update `V0_4_NEXT_STEPS.md` ✓
2. **DRP2: uniform bind group** — `_stream.h`, `stream.c`, `runtime.c` ✓
3. **DRP2: second bind group layout in pipeline** — `create_render_pipeline` + runtime ✓
4. **Shader infrastructure** — builtin GLSL compiled to SPIR-V at build time; converter uses embedded binaries ✓
5. **MVP UBO infrastructure** — per-panel buffer + bind group created in converter on first emit ✓
6. **Scene emit wiring** — `WRITE_BUFFER` + `SET_BIND_GROUP(set=0)` per frame in converter ✓
7. **Port panzoom** — `src/scene/panzoom.c` + `include/datoviz/scene/panzoom.h` ✓
8. **Port arcball** — `src/scene/arcball.c` + `include/datoviz/scene/arcball.h` ✓
9. **Panel controller API** — `dvz_panel_set_panzoom` / `dvz_panel_set_arcball` + `dvz_app_window_input` ✓
10. **`hello_point_glfw` extended** — live pan/zoom demo ✓
