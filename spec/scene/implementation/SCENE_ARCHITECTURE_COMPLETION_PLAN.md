# Scene Architecture Completion Plan

This note records the remaining work needed to finish the v0.4 scene refactor as a long-term
architecture, not merely as a source-file split. It is the source of truth for future agents working
on the final scene architecture cleanup.

Normative status: implementation architecture plan. Public behavior remains in the scene semantics
and API specs; this file defines ownership, module boundaries, migration order, and done criteria.


## Current Position

As of 2026-05-28, the old flat `src/scene/plan/` folder is gone. Its former ownership is split
across `frame_plan/`, `scene_emit/`, and `render_contract/`. Runtime render emission, core scene
ownership, render-contract files, frame-plan internals, several annotation/domain helpers, polygon
helpers, and the first visual descriptor/attribute slices have been split.

Important current state:

1. `scene_emit/` still owns retained scene to FramePlan orchestration.
2. `frame_plan/` owns backend-neutral pass, resource, dependency, readback, and debug structures.
3. `render_contract/` checks planned visual/resource/draw state against runtime requirements.
4. `runtime/` emits DRP2 from resolved FramePlan state. The normal render preparation path now
   consumes visual descriptors and capability flags without branching on concrete visual families;
   remaining runtime visual-family assumptions are concentrated in the explicit untyped
   compatibility/pass-emission fallback path.
5. `visuals/desc.c` lowers typed visual metadata into runtime descriptors. Its untyped descriptor
   fallback is now guarded by an explicit FramePlan compatibility flag.
6. `visuals/desc_untyped_compat.c` quarantines the current untyped fallback classifiers. These
   classifiers infer point, splat, primitive, image, and textured-mesh descriptors from resource
   roles/tags only for explicit compatibility fixtures.
7. `visuals/registry/` owns the first private `DvzVisualFamilyOps` table. It currently covers
   family identity, retained visual lowering, retained visual metadata fill, retained visual
   pass-capability resolution, bind descriptors, pipeline descriptors, shader descriptors, and draw
   descriptors, with tests enforcing active-family coverage. Retained lowering, bind descriptors,
   and normal pipeline descriptors are now implemented in the active family folders, while shared
   default pass-capability resolution lives in `visuals/pass_caps.c`.
8. `visuals/attrs.c` and `visuals/desc.c` are smaller after the first split, and image/labels/volume
   metadata fill has moved behind family hooks. Draw descriptor hooks now live in active family
   folders and call the shared default draw helper. Point-like, segment, path, sphere, splat,
   image, labels, and glyph shader bodies have moved into family folders. The final architecture
   still needs upload, query, bounds, and the remaining generic shader bodies to migrate into
   family-owned files.
9. `scene_emit/uploads.c`, `annotation/text.c`, `annotation/axis.c`, and `domain/field.c` remain the
   highest-value mixed-ownership files.

The current `desc_untyped_compat.c` path is an intermediate compatibility step, not the desired
final architecture. Normal v0.4 scene output emits explicit typed metadata and should not require
runtime code to infer a visual family from a list of resource ids.


## Immediate Next Execution Queue

Future agents continuing the current split should start here. Work in this order unless a regression
requires a narrower interruption. Make small commits after each coherent ownership move, and update
this section when a slice is completed.

1. Move normal shader descriptor bodies into family folders.
   - Current root: `src/scene/visuals/shader_desc.c`.
   - Status on 2026-05-28: started with point-like `point/shader.c`, `pixel/shader.c`,
     `marker/shader.c`, plus `segment/shader.c`, `path/shader.c`, `sphere/shader.c`,
     `splat/shader.c`, `image/shader.c`, `labels/shader.c`, and `glyph/shader.c`. The generic
     shader resolver remains as the fallback dispatcher for families that can lower to another
     descriptor kind.
   - Add family-owned shader files only where real code moves, likely:
     `point/shader.c`, `pixel/shader.c`, `marker/shader.c`, `splat/shader.c`,
     `sphere/shader.c`, `segment/shader.c`, `path/shader.c`, `vector/shader.c`,
     `primitive/shader.c`, `mesh/shader.c`, `image/shader.c`, `labels/shader.c`,
     `glyph/shader.c`, `text/shader.c`, and `volume/shader.c`.
   - Keep genuinely cross-family helpers centralized: cache-key suffix append, built-in shader
     source attachment, built-in identity metadata, pass-policy overrides, query shader overrides,
     GLSL variant creation, and depth-peel fragment selection.
   - Preserve the special/pass shader policy API until a later pass can decide whether those
     policies belong in family hooks or in technique policy.
   - Suggested commits: point-like shaders; stroke/mesh/primitive shaders; texture/semantic
     shaders; optional docs checkpoint.
   - Validate each commit with `git diff --check`, `just build`, and
     `direnv exec . just test scene-graph`; include `direnv exec . just test fields` for
     image/labels/volume shader slices.

2. Move draw descriptor bodies into family folders.
   - Current root: `src/scene/visuals/draw_desc.c`.
   - Status on 2026-05-28: completed for active families. Family `draw.c` files own the hooks and
     currently delegate to `_scene_visual_default_draw_desc()` until a family needs custom draw-count
     policy.
   - Add a shared default draw helper if most families still use `vertex_count`, `instance_count`,
     `index_buffer_id`, `index_format`, and `index_count` directly.
   - Register family draw hooks that call the default helper first, then move any generated or
     family-specific draw-count logic into the owning family as it appears.
   - Done when adding a visual with nonstandard draw counts does not require editing generic
     visual or runtime draw packet code.

3. Move upload/cache payload builders out of generic scene emission.
   - Primary root: `src/scene/scene_emit/uploads.c`.
   - Keep orchestration in `scene_emit/`: resource-key allocation, upload-node ordering,
     dependencies, dirty-range merge policy, and FramePlan node insertion.
   - Move pure family payload construction into `visuals/<family>/upload.c` or existing family
     upload/cache files: point/mesh/indexed typed data, stroke generated geometry, image generated
     quads and texture payloads, volume source/transfer/label textures, labels lookup payloads.
   - Commit by payload family so test failures are attributable.

4. Finish bounds and query family ownership.
   - Bounds is partly split; remove remaining generic visual branches that compute family semantics
     outside family folders or explicit shared visual subsystems such as `visuals/stroke/`.
   - Query has family files already; finish moving target capability, scratch geometry, native
     result decoding, and unsupported-policy decisions out of generic query code.
   - Keep the query executor, request queue, readback scheduling, and retained request processing
     generic.

5. Kill normal-path untyped descriptor compatibility.
   - Ensure query-generated render nodes and every normal retained visual render node emit typed
     metadata.
   - Add an architecture test/debug assertion that a normal render visual without typed metadata
     fails before runtime inference.
   - Delete `visuals/desc_untyped_compat.c` if no explicit fixture/import path still needs it; if
     it remains, keep it behind an explicit compatibility flag and document its callers.

6. Finish domain and annotation ownership.
   - Split `domain/field.c` into lifecycle/public setters, sampled interpretation, scale binding,
     and generated visual synchronization.
   - Split `annotation/text.c` into retained text state, layout, glyph/quad synchronization, and
     renderer payloads.
   - Split `annotation/axis.c` into retained axis state, tick generation, layout reserve, and
     generated visual ownership.
   - Keep rendering semantics in visual families, not in domain or annotation orchestration.

7. Split coarse scene CMake targets.
   - Implement the target layers described below: `datoviz_scene_frame_plan`,
     `datoviz_scene_core`, `datoviz_scene_visuals`, `datoviz_scene_runtime`, and
     `datoviz_scene_app`.
   - Do not create one target per visual family unless a concrete consumer or build problem proves
     the need.
   - Validate that planning-only/core/visual/runtime/app subsets can build without accidental
     app/window/runtime dependencies.

8. Shrink broad private headers and add architecture checks.
   - Audit `_scene.h`, `_visual_internal.h`, `_visual_pipeline.h`, and
     `_visual_pipeline_internal.h`.
   - Move declarations into owning-folder headers and keep only stable cross-subsystem contracts in
     broad headers.
   - Add tests/checks for typed metadata completeness, explicit compatibility use, registry
     coverage, absence of normal runtime visual switches, and CMake layer dependency boundaries.

9. Final cleanup and confidence pass.
   - Remove transitional v0.4-dev names such as remaining `legacy` terminology where it is only a
     migration artifact.
   - Move completed agent plans out of active queues and update roadmap records from pending work
     to final ownership.
   - Run `git diff --check`, `just build`, `direnv exec . just test scene`, and any relevant DRP2
     checks before declaring the architecture cleanup complete.


## Target Architecture

The final architecture should make generic scene code depend on visual-family operations, not on
concrete visual families.

Target layers:

1. `scene/core`: scene, figure, panel, layout, controllers, lifecycle, mutation notifications, and
   frame ownership.
2. `scene/domain`: reusable retained data objects such as buffers, sampled fields, scales, polygon
   data, text/font resources, and future data-domain helpers.
3. `scene/frame_plan`: backend-neutral frame graph and render/upload/readback node model.
4. `scene/render_contract`: validation between planned visual metadata and runtime requirements.
5. `scene/visuals/<family>`: visual-specific API, attributes, bounds, upload payloads, metadata,
   query implementation, shader/pipeline requirements, draw policy, and generated geometry.
6. `scene/visuals/registry`: visual-family operation table and registration glue.
7. `scene/scene_emit`: generic retained scene to FramePlan orchestration driven by visual-family
   operations.
8. `scene/runtime`: generic FramePlan to DRP2 emission from resolved descriptors and render
   contracts.
9. `scene/techniques`: graph-backed pass construction for MSAA, WBOIT, depth peel, SSAO, EDL, and
   related technique policy.
10. `scene/app`: presentation-only bridge over scene plus canvas/runtime.

Adding a visual family should require adding or editing only:

1. the family folder under `src/scene/visuals/<family>/`;
2. the family registration list;
3. family-local tests and examples.

Adding a visual family should not require editing generic scene emission, runtime emission, query
execution, render-contract resolution, pass-capability dispatch, descriptor inference, or shader
selection switch statements.


## Final Visual-Family Contract

Introduce a private `DvzVisualFamilyOps` contract before removing the remaining generic visual
switches. The exact C names may change, but the operation model should cover these responsibilities:

1. family identity: visual type, family name, supported controller/pass/query capabilities;
2. lifecycle defaults: constructor defaults, style/mode validation, family state cleanup;
3. attribute schema: supported attributes, storage names, item sizes, source policy, mutability
   rules, and dense data validation;
4. bounds: family-local visual-space bounds and generated bounds data;
5. upload payloads: derived geometry/cache/texture/lookup payload construction;
6. metadata emission: explicit `DvzFramePlanVisualMeta` population for normal render paths and
   family-owned query render paths;
7. pipeline contract: vertex buffers, index buffers, materials, textures, bind layouts, shader
   variants, depth/blend state, and pass eligibility;
8. draw contract: topology, draw counts, instance counts, index format, and optional generated draw
   expansion;
9. query operations: target capability, temporary query geometry, native result decoding, and
   explicit unsupported-policy decisions;
10. diagnostics: family-specific contract failure messages that do not leak into generic runtime
    code.

Keep orchestration outside the family ops. Family code may build payloads and declare requirements;
`scene_emit/` keeps resource-key allocation, upload-node ordering, pass scheduling, and dependency
graph construction. `runtime/` keeps DRP2 emission, render target realization, viewport/scissor,
clear commands, descriptor refresh, and graph-resource execution.


## Required End-To-End Sequence

Future agents should work through these phases in order unless a bug fix requires a narrower
interruption. Each phase should be split into small behavior-preserving commits.


### 1. Kill Untyped Descriptor Inference For Normal Scene Output

Goal: normal v0.4 scene rendering uses explicit typed metadata everywhere.

Steps:

1. Audit every path that creates a render visual and confirm it emits `DvzFramePlanVisualMeta`.
2. Make query-generated render nodes emit the same metadata model as normal retained scene visuals.
3. Add a test/debug mode that fails when a normal scene render visual reaches runtime without typed
   metadata.
4. Keep untyped descriptor inference only for explicit fixture/import compatibility if such a path
   is still needed.
5. Delete `visuals/desc_legacy.c` if no compatibility path remains; otherwise rename it to an
   explicit compatibility name such as `desc_untyped_compat.c`.

Done criteria:

1. `_scene_visual_desc_from_render()` does not infer normal scene visual families from resource-list
   shape.
2. `desc_legacy.c` is either gone or only reachable through an explicit compatibility switch.
3. Tests fail if scene emission omits typed metadata for an active visual.


### 2. Introduce The Visual-Family Registry

Goal: generic code calls visual-family operations instead of visual-family switch statements.

Status as of 2026-05-28: in progress. `src/scene/visuals/registry/` now contains the private
`DvzVisualFamilyOps` table, every active `DvzVisualType` is registered, and
`test_scene_visual_family_registry_coverage` enforces identity, lowering, pass-capability,
bind-descriptor, pipeline-descriptor, shader-descriptor, and draw-descriptor hooks. Retained visual
lowering is implemented in each active family folder. Image, labels, and volume metadata fill now
routes through family hooks. Bind descriptors and normal pipeline descriptors are implemented in the
active family folders. Retained visual pass-capability resolution uses a shared default hook in
`visuals/pass_caps.c`; runtime bind selection, runtime pipeline selection, runtime shader selection,
special/pass shader policy, pass pipeline policy, pass binding policy, and runtime draw-count
packetization now route through visual-owned descriptors and registry hooks. Continue by migrating
upload, query, bounds, and the current generic shader/draw bodies into family-owned files
incrementally.

Steps:

1. Add a narrow private registry header under `src/scene/visuals/registry/`.
2. Define the initial `DvzVisualFamilyOps` with only the operations needed by the first migration
   slice.
3. Register existing active families one by one.
4. Keep the initial registry minimal. Add hooks only when a generic switch is actually migrated.
5. Add registry coverage tests that assert every active `DvzVisualType` has registered operations.

Done criteria:

1. Registry coverage is enforced by tests.
2. Generic code can look up family operations without including family-private headers.
3. Runtime draw packetization no longer owns descriptor-kind fallback layout policy.
4. New visual-family code has a single obvious registration point.


### 3. Move Visual-Specific Logic Into Family Folders

Goal: each active visual owns its own semantics below `src/scene/visuals/<family>/`.

Status as of 2026-05-28: active family folders now own retained lowering for point, pixel, marker,
splat, sphere, segment, path, vector, primitive, mesh, image, glyph, labels, volume, and text.
They also own bind descriptors and normal pipeline descriptors. Image, labels, and volume own their
normal FramePlan metadata fill hooks. The next high-value family-folder moves are upload/cache
payload builders, bounds reducers, query policy/result decoding, and the shader/draw bodies still
shared in root visual helper files.

Recommended family layout:

```text
src/scene/visuals/<family>/
  api.c
  attrs.c
  bounds.c
  upload.c
  metadata.c
  pipeline.c
  query.c
  shaders.c
  internal.h
```

Only create files when there is real owned code. Do not create placeholders.

Move, family by family:

1. attribute schema and dense-data validation;
2. bounds reducers and generated bounds helpers;
3. pure derived upload/cache payload builders from `scene_emit/uploads.c`;
4. metadata construction;
5. pipeline, bind-layout, shader, pass-capability, and draw-count declarations;
6. query geometry/result decoding;
7. family-specific diagnostics.

Done criteria:

1. Generic visual files contain loops over registered operations, not concrete family branches.
2. Family-private headers are used instead of growing `_visual_internal.h`.
3. Adding a visual does not require editing existing family files.


### 4. Make Runtime Render Emission Descriptor-Driven

Goal: runtime consumes resolved descriptors and render contracts; it does not know visual semantics.

Status as of 2026-05-28: normal `runtime/render_emit_prepare.c` no longer branches on concrete
visual families for shader selection, pass shader policy, pipeline policy, bind policy, textured-mesh
layout selection, picking query overrides, or draw-count packetization. It still creates generic DRP2
objects, bind groups, pipeline layouts, and draw packets from descriptors. The remaining runtime
visual-family branching is mostly in `runtime/render_emit_passes.c`, where it supports the explicit
untyped compatibility fallback and should be retired or quarantined behind compatibility fixtures
after normal scene output is guaranteed to carry typed metadata.

Steps:

1. Move draw-count, instance-count, topology, and generated-draw policy into visual metadata or
   visual-family pipeline/draw contracts. Draw-count packetization now has a registry hook; finish
   moving per-family bodies out of generic descriptor switch files.
2. Move bind-layout selection and shader-variant requirements into family descriptors. Runtime now
   dispatches these through registry hooks; finish moving the bodies from root-level helpers into
   family folders.
3. Keep `runtime/render_emit_prepare.c` focused on assembling generic prepared draw packets.
4. Keep `runtime/render_emit_draws.c` focused on viewport/scissor and DRP2 draw commands.
5. Keep `runtime/render_emit_passes.c` focused on pass setup, graph/plain pass routing, targets,
   clear-only nodes, and compute-assisted pass routing.

Done criteria:

1. `src/scene/runtime/` does not branch on concrete visual family names for normal draw emission.
2. Runtime tests cover pass setup, binding selection, and draw-count behavior through descriptors.
3. Runtime remains reusable by alternative scene frontends that produce compatible FramePlans.


### 5. Finish Domain And Annotation Boundaries

Goal: domain objects own data and interpretation; annotations own semantic generated visuals; visual
families own rendering.

Steps:

1. Split `domain/field.c` into lifecycle/public setters, sampled interpretation, scale binding, and
   generated visual synchronization.
2. Keep image/volume/labels rendering behavior in visual-family folders after field interpretation
   has produced explicit metadata and resources.
3. Split `annotation/text.c` into retained text object state, layout, glyph/quad synchronization,
   and renderer-specific payloads.
4. Split `annotation/axis.c` into axis retained state, tick generation, layout reserve, and generated
   visual ownership.
5. Avoid moving annotation-generated visual orchestration into runtime or visual family internals.

Done criteria:

1. Domain objects can be reused without pulling in scene runtime.
2. Annotation code does not duplicate visual-family rendering semantics.
3. Generated visual synchronization is owned by the semantic object that creates those visuals.


### 6. Split Scene CMake Targets Into Coarse Reusable Layers

Goal: consumers such as VisPy2 can reuse the major scene layers without linking app/runtime pieces
they do not need. Source folders should be granular; CMake targets should stay coarse and stable
until a concrete consumer needs finer selection.

Target object/static-library layers:

```text
datoviz_scene_frame_plan
datoviz_scene_core
datoviz_scene_visuals
datoviz_scene_runtime
datoviz_scene_app
```

Layer contents:

1. `datoviz_scene_frame_plan`: FramePlan nodes, resources, graph passes, dependencies, readbacks,
   diagnostics, serialization, and DRP2 fixture/emission facades that do not need retained scene
   objects.
2. `datoviz_scene_core`: retained scene/domain/annotation/layout/controller object model without
   DRP2 runtime execution.
3. `datoviz_scene_visuals`: visual registry, built-in visual families, visual attributes, bounds,
   query implementations, metadata builders, and visual pipeline/draw contracts.
4. `datoviz_scene_runtime`: render-contract validation, techniques, graph-resource realization, and
   FramePlan to DRP2 runtime emission.
5. `datoviz_scene_app`: presentation/offscreen/GLFW bridge.

Dependency direction should be acyclic. A practical target is:

```text
common/math/geom
  -> frame_plan
  -> scene_core
  -> scene_visuals
  -> scene_runtime
  -> scene_app
```

Refine the exact direction while implementing, but keep these rules:

1. visual-family source modules may depend on common, math, geom, domain/core types, frame-plan
   metadata types, and registry contracts;
2. visual-family source modules must not depend on app or window;
3. scene emission may depend on retained scene/core/domain objects, frame-plan types, and visual
   registry contracts;
4. render-contract code may depend on frame-plan types and visual-family contracts, but it should
   not create new scene semantics;
5. generic runtime must not depend on app/window and should consume retained scene state only
   through FramePlan and resolved contracts where practical;
6. frame-plan must not depend on visual-family implementation files;
7. app depends on scene runtime, not the reverse.

Do not split visual CMake targets per family by default. The visual registry and source layout
should make that possible later, but the default target should remain `datoviz_scene_visuals`.
Consider grouped or per-family visual targets only after there is a concrete reuse, build-time, or
packaging reason.

Done criteria:

1. CMake can build meaningful subsets for planning-only, scene object model, full runtime, and app
   users.
2. VisPy2 can link the coarse layers it needs without pulling in app/window/runtime code
   accidentally.
3. The top-level `datoviz` target still assembles the full default stack.


### 7. Shrink Broad Private Headers

Goal: private headers reflect module boundaries and do not force broad recompilation.

Steps:

1. Audit `_scene.h`, `_visual_internal.h`, `_visual_pipeline.h`, and `_visual_pipeline_internal.h`.
2. Move declarations into narrow headers under the owning folders.
3. Keep only stable cross-subsystem contracts in broad headers.
4. Prefer family-private `internal.h` files and registry contracts for visual-family operations.

Done criteria:

1. A family implementation change does not require generic scene code to include family internals.
2. Broad headers stop accumulating new helper declarations during future feature work.


### 8. Add Architecture Tests And Static Checks

Goal: architecture regressions fail automatically.

Add tests or checks for:

1. every emitted normal render visual has typed metadata;
2. untyped descriptor compatibility is explicitly enabled when used;
3. every active visual type has registry operations;
4. generic runtime files do not reference concrete visual-family symbols or names;
5. generic scene-emission files call family operations instead of family-specific branches;
6. visual-family contract validation catches missing resources, wrong bind layouts, and draw-count
   mismatches;
7. selected CMake layer builds do not pull in app/runtime accidentally.

Done criteria:

1. Architecture constraints are checked in CI or by focused local commands.
2. `just build`, `git diff --check`, `direnv exec . just test scene`, and relevant DRP2 checks pass.


### 9. Remove Transitional v0.4-Dev Names And Specs

Goal: durable documentation describes the final architecture, not the migration.

Steps:

1. Remove or rename files whose only purpose was migration containment, including any remaining
   `legacy` descriptor naming.
2. Move completed agent plans out of active queues.
3. Update `SCENE_CODE_SPLIT_ROADMAP.md` so completed phases are recorded as final ownership, not
   as pending cleanup.
4. Keep this completion plan only until the architecture tests and CMake layers prove the final
   shape.

Done criteria:

1. Active specs describe the final architecture directly.
2. No active roadmap tells agents to redo completed structural work.


## Agent Execution Rules

When assigned this architecture cleanup, future agents should:

1. start with this file, then read `SCENE_CODE_SPLIT_ROADMAP.md` and
   `../visuals/IMPLEMENTATION_LAYOUT.md`;
2. inspect current code before assuming a roadmap item is still pending;
3. make one coherent ownership change per commit;
4. preserve existing comments and Doxygen blocks when moving code;
5. keep behavior-preserving moves separate from behavior changes;
6. add or update focused tests when an architecture invariant becomes enforceable;
7. run `git status --short` before staging and never stage `data`, generated binaries, vendored
   runtime libraries, or unrelated changes;
8. run `git diff --check`, `just build`, and the narrowest relevant tests for each commit;
9. run `direnv exec . just test scene` before closing a multi-commit scene architecture batch;
10. update this spec and the short agent pickup note when a phase becomes complete.


## Final Confidence Bar

The scene refactor can be considered architecturally complete when all of these are true:

1. adding a visual family does not require editing generic scene emission, runtime, query,
   render-contract, descriptor, pass-capability, or shader-selection code;
2. visual-specific logic is confined to `src/scene/visuals/<family>/` or to explicit shared visual
   subsystems such as `visuals/stroke/`;
3. normal scene output always carries typed visual metadata;
4. runtime render emission consumes explicit descriptors and contracts only;
5. untyped descriptor inference is deleted or isolated behind explicit compatibility;
6. domain, annotation, visual, scene-emission, frame-plan, render-contract, runtime, technique, and
   app responsibilities are separate and acyclic;
7. coarse scene CMake targets can be reused by external consumers such as VisPy2 without pulling in
   app/window/runtime layers accidentally;
8. broad private headers are reduced to stable contracts;
9. architecture tests enforce metadata completeness, registry coverage, no generic runtime visual
   switches, and layer dependency boundaries;
10. full scene validation passes after the final cleanup batch.
