# Scene Converter Refactor Plan

> **Execution Status**
> - **Status:** `MECHANICAL SPLIT LANDED; EMITTER STATE HARDENING STARTED`
> - **Updated on:** `2026-05-13`
> - **Scope:** track the remaining scene -> DRP2 emission cleanups after the first
>   behavior-preserving `src/scene/converter.c` split.


## Current State

The initial converter split has landed. `src/scene/converter.c` is gone, and the current emission
code is distributed across:

1. `frame_plan_emit.c` - public/shared emission helpers and validation.
2. `frame_plan_fixture.c` - deterministic fixture-mode FramePlan -> DRP2 emission.
3. `frame_plan_runtime.c` - runtime-mode scene emission and per-frame orchestration.
4. `frame_plan_runtime_state.c` - persistent emitter state and resource/object id allocation.
5. `frame_plan_runtime_upload.c` - runtime buffer, texture, and compute-buffer upload helpers.
6. `shader_registry.c` - built-in shader selection and SPIR-V/GLSL lookup.
7. `visual_pipeline.c` - first visual-family detection helpers.
8. `render_pass.c` - color-target, readback-buffer, transient id, copy/submit helpers.

Follow-up commit `3d4bd920` added the first `DvzSceneVisualDesc` extraction so
`visual_pipeline.c` now owns per-visual resource resolution, family classification, vertex/index
counts, image draw-buffer narrowing, and index-format selection for the multi-visual scene render
path. Follow-up commit `bc65010f` added `DvzSceneVisualShaderDesc`, moving visual shader keys,
shader source selection, SPIR-V keys, and pipeline cache-key selection into `visual_pipeline.c`.
Follow-up commit `45ae792a` added `DvzSceneVisualPipelineDesc`, moving vertex layout and depth-state
selection into `visual_pipeline.c`. Follow-up commit `0f29f4f1` added
`DvzSceneVisualBindDesc`, moving bind-role selection into `visual_pipeline.c`.
`frame_plan_runtime.c` still owns concrete DRP2 bind-group object creation and draw submission for
that path. Follow-up commit `586d3fa0` started the emitter state hardening pass: persistent runtime
resource/object tables now grow dynamically, fixture converter state is cleaned up explicitly, and
the runtime compute-buffer path reacquires resource entries after possible table growth.

The rest of this document is now a follow-up tracker. Sections describing already-created files are
historical context unless they call out remaining work explicitly.


## Goal

The old `src/scene/converter.c` mixed several concerns:

1. fixture-only FramePlan -> DRP2 command stream generation,
2. persistent runtime emitter state and object-id allocation,
3. resource upload tracking,
4. visual-family detection and draw descriptor construction,
5. shader source/SPIR-V selection,
6. pipeline and bind-group creation,
7. render-pass/readback/submit orchestration.

The first refactor made those responsibilities more explicit while preserving the current scene ->
FramePlan -> DRP2 -> vklite/canvas behavior.

The remaining cleanup is more targeted:

1. centralize current resource-key formatting/parsing before replacing stringly metadata,
2. continue hardening emitter/resource failure paths and diagnostics as they are exposed,
3. replace stringly visual/resource metadata with typed FramePlan metadata in a later
   behavior-changing pass.

This is a structural cleanup plan, not an API compatibility constraint. The v0.4 branch can still
change internal APIs aggressively when that improves correctness and maintainability.


## Naming Direction

Avoid mechanically splitting into many `converter_*` files. Prefer filenames that name ownership
boundaries:

1. `frame_plan_emit.c` - public emission entry points and high-level orchestration.
2. `frame_plan_fixture.c` - deterministic fixture-mode emission path.
3. `frame_plan_runtime.c` - runtime-mode emission path and persistent emitter lifecycle.
4. `shader_registry.c` - built-in shader metadata, GLSL source lookup, SPIR-V key lookup.
5. `visual_pipeline.c` - visual-family classification, vertex layouts, pipeline descriptors,
   draw descriptors.
6. `render_pass.c` - color target, readback buffer, encoder/pass/submit helpers.

Private headers should follow the same ownership:

1. `_frame_plan_emit.h` - shared private emission declarations.
2. `_shader_registry.h` - built-in shader registry API.
3. `_visual_pipeline.h` - visual draw/pipeline descriptor API.
4. `_render_pass.h` - render target/readback/pass helper API.

Keep `converter.c` only as a temporary facade if needed during migration. Once the public entry
points move cleanly, either delete it or turn it into a very small compatibility wrapper.


## Shader Ownership

Runtime visual shaders should live as shader files, not C string macros.

Use this source layout:

```text
src/scene/glsl/
  common.glsl
  point.vert
  point.frag
  point_pick.vert
  point_pick.frag
  primitive.vert
  primitive.frag
  primitive_lit.vert
  primitive_lit.frag
  image.vert
  image.frag
```

The existing root CMake shader pipeline already compiles `src/scene/glsl/*.vert` and `*.frag` to
SPIR-V and embeds them in generated `_shaders.c`. Extend that path instead of adding another shader
loading mechanism.

Keep tiny fixture shaders inline only if they remain true fixture scaffolding. Anything used by the
retained runtime scene path should be a file-backed shader with a registry entry.


## Step-By-Step Plan

Steps 2 through 5 and 7 through 8 are implemented in the current tree. Keep them here as historical
context for reviewers, but do not restart them unless a regression requires it.

### Step 1 - Freeze Behavior With Focused Tests

Before moving code, record the current behavior with narrow tests:

1. Run `just build`.
2. Run `just test scene`.
3. If graphics/runtime changes are already pending, also run the smallest relevant live/offscreen
   smoke used by the current branch.
4. Run `git diff --check`.

Do not begin file splitting from a failing baseline unless the failure is understood and unrelated.


### Step 2 - Add Shared Private Emit Header

Create `_frame_plan_emit.h` for private structs and helper declarations that will be shared during
the split.

Move only declarations first:

1. `ResourceId`
2. `ConverterState`
3. `SceneRenderStateCache`
4. `DvzFramePlanEmitter` definition
5. shared ID constants that remain genuinely cross-file

Keep helper function bodies in `converter.c` until the compiler proves the declaration boundary is
right.

Validation:

1. `just build`
2. `git diff --check`


### Step 3 - Extract Emitter State

Create `frame_plan_runtime.c` or a small internal state file only if needed. Move state and ID
helpers that do not emit DRP2 commands:

1. `_state_init`
2. `_resource_id`
3. `_resource_lookup_id`
4. `_resource_find`
5. `_resource_entry`
6. `_resource_ensure_byte_size`
7. `_resource_data_tag`
8. `_resource_byte_size`
9. `_resource_usage`
10. `_resource_item_stride`
11. `_resource_topology`
12. `_obj_id`
13. `_obj_buffer_id`
14. `_emitter_next_transient_id`
15. `_emitter_mvp_slot`
16. `dvz_frame_plan_emitter`
17. `dvz_frame_plan_emitter_destroy`
18. `dvz_frame_plan_emitter_object_id`

While moving, keep behavior identical. Do not switch fixed arrays to dynamic storage in this step.

Validation:

1. `just build`
2. `just test scene`
3. `git diff --check`


### Step 4 - Extract Shader Registry

Create `shader_registry.c` and `_shader_registry.h`.

Move shader selection out of the emitter code:

1. move runtime GLSL source text into `src/scene/glsl/*.vert` and `*.frag`,
2. keep or add embedded fallback strings only inside `shader_registry.c` if runtime GLSL fallback
   still requires source strings,
3. expose internal lookup helpers by semantic shader key, not raw macro name,
4. map each shader variant to:
   - stage,
   - GLSL source text,
   - optional SPIR-V resource key,
   - DRP2 source format support.

Suggested internal enum shape:

```c
typedef enum
{
    DVZ_SCENE_BUILTIN_SHADER_FIXTURE,
    DVZ_SCENE_BUILTIN_SHADER_TEXTURE,
    DVZ_SCENE_BUILTIN_SHADER_COMPUTE_COPY,
    DVZ_SCENE_BUILTIN_SHADER_POINT,
    DVZ_SCENE_BUILTIN_SHADER_POINT_PICK,
    DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE,
    DVZ_SCENE_BUILTIN_SHADER_PRIMITIVE_LIT,
    DVZ_SCENE_BUILTIN_SHADER_IMAGE,
} DvzSceneBuiltinShader;
```

The converter should ask for shader metadata; it should not contain shader source text.

Validation:

1. `just build`
2. `just test scene`
3. run at least one GLSL runtime/offscreen scene path if available,
4. `git diff --check`


### Step 5 - Extract Render-Pass Helpers

Create `render_pass.c` and `_render_pass.h`.

Move repeated command sequences into small helpers:

1. resolve or create the color target,
2. resolve or create the readback buffer,
3. allocate transient encoder/render-pass/command-buffer/submission IDs,
4. begin clear/region render pass,
5. optionally attach depth state,
6. copy texture to readback buffer,
7. finish encoder and queue submit or submit-readback.

Keep helpers at the command-stream level. They should not know visual-family details.

Validation:

1. `just build`
2. `just test scene`
3. run focused readback/pick/probe tests if they are filterable,
4. `git diff --check`


### Step 6 - Complete Visual Pipeline Lowering

Status: **partially implemented**. `visual_pipeline.c` owns visual-family detection and the first
`DvzSceneVisualDesc` / `DvzSceneVisualShaderDesc` resolvers for the multi-visual scene render path.
`visual_pipeline.c` also owns first-slice `DvzSceneVisualPipelineDesc` vertex-layout/depth-state
selection and `DvzSceneVisualBindDesc` bind-role selection. `frame_plan_runtime.c` still creates
concrete DRP2 bind-group objects.

Continue using `visual_pipeline.c` and `_visual_pipeline.h`.

Move logic that turns frame-plan visual/resource metadata into draw/pipeline descriptors:

1. `_parse_visual_id`
2. `_is_point_visual`
3. `_is_primitive_visual`
4. `_is_image_visual`
5. `_scene_render_needs_depth`
6. per-visual buffer resolution,
7. vertex/index count calculation,
8. shader variant selection,
9. vertex layout selection,
10. pipeline cache-key construction,
11. bind-group layout requirements.

The output should be a compact internal descriptor that runtime emission can consume:

```c
typedef struct DvzSceneDrawDesc DvzSceneDrawDesc;
typedef struct DvzScenePipelineDesc DvzScenePipelineDesc;
```

Do this in two passes:

1. first extract existing code with minimal reshaping,
2. then replace stringly visual detection with explicit typed frame-plan metadata in a later
   behavior-changing pass.

Validation:

1. `just build`
2. `just test scene`
3. include tests covering point, primitive, mesh/depth, indexed primitive, image, pick/probe,
4. `git diff --check`


### Step 7 - Split Fixture Emission

Create `frame_plan_fixture.c`.

Move fixture-only helpers and entry points:

1. `_zero_base64`
2. `_zero_base64_alloc`
3. `_emit_upload`
4. `_emit_texture_upload`
5. `_emit_compute_buffers`
6. `_emit_readback_buffer`
7. `_emit_compute_assisted_render`
8. `_emit_texture_render`
9. `_emit_render`
10. `_emit_clear_only`
11. `_emit_readback`
12. `dvz_frame_plan_emit_drp2`
13. `dvz_frame_plan_emit_drp2_ex`

If a helper is also needed by runtime, move it to a shared utility file instead of duplicating it.

Validation:

1. `just build`
2. run fixture-oriented scene tests,
3. run `just spec-check` if the DRP2 fixture output may have changed,
4. `git diff --check`


### Step 8 - Split Runtime Emission

Create `frame_plan_runtime.c`.

Move runtime-only helpers and entry point:

1. `_emitter_emit_upload`
2. `_emitter_emit_texture_upload`
3. `_emitter_emit_compute_buffers`
4. `_emitter_resolve_render_vertex_buffers`
5. `_emitter_emit_render_multi_in_pass`
6. `_emitter_emit_render_multi`
7. `_emitter_emit_scene_figure_renders`
8. `_emitter_emit_render`
9. `_emitter_emit_plain_renders`
10. `_emitter_emit_clear_only`
11. `_emitter_emit_texture_render`
12. `_emitter_emit_compute_assisted_render`
13. `dvz_frame_plan_emitter_emit_drp2`

At the end of this step, `converter.c` should either be gone or contain only very small wrappers.

Validation:

1. `just build`
2. `just test scene`
3. focused runtime tests for repeated partial updates, multi-panel figures, depth, readback,
   point pick, and image probe,
4. `git diff --check`


### Step 9 - Improve Emitter Data Structures After The Split

Status: **partially implemented**. Commit `586d3fa0` replaced the fixture-era fixed resource table
with growable state for runtime resources and persistent object ids, added cleanup for temporary
fixture converter state, guarded texture row-pitch multiplication, and added a regression test that
exceeds the old `DRP2_MAX_FIXTURE_RESOURCES` object-id ceiling.

Continue this pass by replacing fixture-era assumptions where they still hurt runtime behavior.

Candidate changes:

1. split fixture resource state from persistent runtime resource/object state,
2. report capacity/growth failures with specific diagnostics,
3. guard remaining texture byte-size arithmetic,
4. guard `uint64_t` -> `uint32_t` vertex/index count downcasts,
5. add explicit resource kind to entries instead of inferring buffer/texture role from tags.

Validation:

1. `just build`
2. `just test scene`
3. add regression tests for capacity growth and downcast/overflow failure paths,
4. `git diff --check`


### Step 10 - Replace Stringly Visual Metadata

Status: **deferred until Step 6 and Step 9 are stable**.

This is the first intentionally behavior-shaping cleanup.

Move visual identity out of resource-name conventions and `data_tag` string inference:

1. add typed visual family metadata to FramePlan render nodes,
2. add typed attribute descriptors for position/color/size/normal/texcoords/texture/index/shading,
3. make topology and controller mode explicit per draw,
4. keep resource names as debug labels and stable DRP2 cache keys, not semantic truth,
5. remove substring routing such as checking whether a visual id contains `"image"` or `"texture"`.

Validation:

1. `just build`
2. `just test scene`
3. `just spec-check` if serialized frame-plan or DRP2 fixtures change,
4. focused runtime scene smoke tests.


## Suggested End State

The active scene emission files should read approximately like this:

```text
src/scene/
  frame_plan.c
  frame_plan_emit.c
  frame_plan_fixture.c
  frame_plan_runtime.c
  render_pass.c
  shader_registry.c
  visual_pipeline.c
  glsl/
    common.glsl
    point.vert
    point.frag
    point_pick.vert
    point_pick.frag
    primitive.vert
    primitive.frag
    primitive_lit.vert
    primitive_lit.frag
    image.vert
    image.frag
```

`frame_plan_emit.c` should be thin. Most detailed decisions should live in the file that owns the
concept:

1. shader registry owns shader source and SPIR-V keys,
2. visual pipeline owns attribute layouts and pipeline descriptors,
3. render pass owns target/readback/submit command sequences,
4. runtime emitter owns persistent object IDs and per-frame orchestration,
5. fixture emitter owns deterministic test-fixture streams.


## Non-Goals For The First Split

Do not combine these behavior changes with the initial file split:

1. changing public scene APIs,
2. adding new visual families,
3. introducing WebGPU semantics,
4. changing DRP2 command semantics,
5. replacing the native vklite runtime path,
6. broad render graph redesign.

Make the split mechanically reviewable first, then improve architecture in smaller follow-up slices.


## Final Validation Checklist

Before considering the refactor complete:

1. `git diff --check`
2. `just build`
3. `just test scene`
4. `just test drp2`
5. `just spec-check`
6. focused runtime smoke for:
   - point rendering,
   - primitive rendering,
   - mesh/depth rendering,
   - indexed primitive shading update,
   - image rendering and partial texture update,
   - multi-panel figure,
   - point pick,
   - image probe.

If any graphics smoke cannot be run in the current environment, record the exact skipped command and
reason in the final handoff.
