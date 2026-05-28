# Scene Code Split Roadmap

This note records the next maintainability split for `src/scene`. It is broader than the visual
family layout note: it covers frame planning, runtime emission, core scene ownership, query
execution, annotations, techniques, and tests.

Normative status: informative implementation roadmap. Public semantics remain in the specialized
scene specs; this file only defines safe source-ownership direction for refactors.

For the final architecture target, including the visual-family registry, removal of normal
untyped/legacy descriptor inference, coarse reusable CMake layers, and done criteria for saying the
scene architecture is robust, read
[`SCENE_ARCHITECTURE_COMPLETION_PLAN.md`](SCENE_ARCHITECTURE_COMPLETION_PLAN.md) first. This file
tracks source-split progress; the completion plan defines the end state.


## Current Pressure Points

As of 2026-05-28 after the scene-plan folder removal, the highest-value split
candidates are:

1. `src/scene/scene_emit/uploads.c` and `scene_emit/derived_upload.c`: some derived payload
   orchestration remains in scene-emission code. Continue moving only pure cache/data construction
   into family or subsystem helpers; image generated-quad cache/payload decisions now live in the
   image family, and stroke-quad/path-stroke cache/payload decisions now live in the stroke
   subsystem. Image/glyph RGBA texture upload payload decisions also live in the image family, and
   volume source/transfer/label-lookup payload decisions live in the volume family.
2. `src/scene/annotation/text.c` and `axis.c`: retained annotation objects, layout/reserve policy,
   generated visuals, and text/glyph lowering are still mixed. Scale, colorbar, legend, colormap,
   scale-bar, and text-font ownership now have first-pass owner files.
3. `src/scene/domain/field.c`: public domain object state, sampled interpretation, generated
   visual glue, and upload dirtiness are still mixed.
4. `src/scene/visuals/`: descriptor and attribute helper ownership has a first split. Draw hooks
   have moved into active family folders, and active-family shader descriptor bodies have moved into
   family folders. Wide visual-family helper files and shared headers should only be split further
   when a stable owner boundary is clear.
5. Scene tests have useful focused files, but several broad scene-graph/runtime tests still cover
   multiple ownership boundaries at once.


## Split Principles

1. Preserve the scene -> FramePlan -> DRP2 -> runtime boundary. Source files may move, but
   dependencies should not point backward from runtime execution into scene semantics.
2. Do not move orchestration into family helpers. Family and subsystem helpers may build geometry,
   cache payloads, upload bytes, or validation-local decisions; `scene_emit/` keeps resource keys,
   upload nodes, dependency ordering, dirty-state policy, and pass scheduling.
3. Runtime files should emit already-resolved FramePlan state. They should not infer visual-family
   semantics that the visual or scene-emission layer could resolve earlier.
4. Keep graph-technique files as graph builders and technique policy owners; keep generic
   graph-resource realization in runtime files.
5. Split by stable ownership, not by enum case. A smaller file is not a win if it duplicates policy
   or forces cross-file mutation of the same object invariants.
6. Use private subsystem headers once declarations are local to a folder. Avoid growing broad
   headers such as `_scene.h`, `frame_plan/frame_plan.h`, or `_visual_internal.h` for narrow helpers.
7. Each commit should be behavior-preserving unless the commit message and tests make a targeted
   behavior change explicit.


## Recommended Sequence

### 1. Finish Low-Risk Derived Payload Extraction

Continue from `spec/scene/visuals/IMPLEMENTATION_LAYOUT.md`.

1. Move remaining pure builders from `scene_emit/uploads.c` into family or subsystem
   files only when the helper owns a coherent payload:
   1. mesh generated/index/upload payload helpers;
   2. remaining labels/image generated query or upload payload helpers;
   3. glyph/text derived buffer helpers;
   4. any remaining stroke cache math shared by render and query paths beyond render-upload payload
      decisions.
2. Keep FramePlan resource-key allocation, upload-node creation, and upload ordering in
   `src/scene/scene_emit/`.
3. Prefer private headers under the owning subsystem, such as `mesh/internal.h`, `text/internal.h`,
   or a narrow `scene_emit/` helper header, instead of adding family details to
   `_visual_internal.h`.

Validation: `just build`, `git diff --check`, and `direnv exec . just test scene`.


### 2. Split FramePlan Internals

Status: completed on 2026-05-28. Keep the current ownership unless a later behavior change exposes
new coupling.

Current ownership:

1. `frame_plan/core.c`: public/internal frame-plan lifecycle and top-level facade.
2. `frame_plan/nodes.c`: node storage growth, append/last helpers, and node accessors.
3. `frame_plan/capabilities.c`: capability snapshot defaults and copy helper.
4. `frame_plan/diagnostics.c`: diagnostic report helpers.
5. `frame_plan/resources.c`: upload node resources and upload metadata.
6. `frame_plan/graph/resources.c`: graph resource descriptors.
7. `frame_plan/passes.c`: compute/render/clear node appenders and render visual metadata.
8. `frame_plan/graph/passes.c`: graph pass descriptors, access builders, and attachments.
9. `frame_plan/dependencies.c`: dependency graph count/get facade.
10. `frame_plan/graph/helpers.c` and `frame_plan/graph/internal.h`: shared graph helper routines.
11. `frame_plan/graph/validation.c`: graph validation diagnostics.
12. `frame_plan/readback.c`: readback/copy metadata and request-facing bookkeeping.
13. `frame_plan/ascii.c`: terminal graph/debug text output.
14. `frame_plan/json.c`: FramePlan JSON/debug serialization.
15. `frame_plan/fixture.c`: deterministic fixture-mode FramePlan -> DRP2 emission.
16. `frame_plan/emit.c`: shared FramePlan -> DRP2 emission facade.

Do not change FramePlan semantics while moving code.

Validation: focused `direnv exec . just test scene/frame-plan` if available through the runner
filter, then full `direnv exec . just test scene`.


### 3. Split Render Contract Resolution

Status: completed on 2026-05-28. Keep the current ownership unless later contract work exposes
new coupling.

Current ownership:

1. `render_contract/core.c`: top-level contract build/validate facade.
2. `render_contract/visual.c`: visual-family contract assembly from resolved descriptors.
3. `render_contract/resources.c`: resource-role and bind-layout compatibility checks.
4. `render_contract/draw.c`: draw/resource compatibility checks against retained visual metadata.
5. `render_contract/drp2.c`: DRP2 command-stream contract checks.
6. `render_contract/diagnostics.c`: drift/error messages and debug summaries.

Guardrail: contract files should report mismatches between declared plan state and runtime-facing
requirements. They should not decide new visual semantics.


### 4. Move Scene Emission Out Of The Old Plan Folder

Status: completed on 2026-05-28. The old `src/scene/plan/` folder has been removed; its ownership
is now split across `frame_plan/`, `render_contract/`, and `scene_emit/`.

Current scene-emission ownership:

1. `scene_emit/core.c`: retained scene -> FramePlan facade.
2. `scene_emit/panel.c`: panel render lowering and pass scheduling.
3. `scene_emit/uploads.c`: generic retained visual upload loop and upload-node orchestration.
4. `scene_emit/derived_upload.c`: derived-geometry upload orchestration for family-owned payloads.
5. `scene_emit/upload_support.c`: shared upload metadata/resource-role helpers.
6. `scene_emit/metadata.c`: typed visual metadata emission.
7. `scene_emit/visual_lowering.c` and `scene_emit/visual_lowering.h`: visual-family lowering
   decisions shared by scene emission, render contracts, visuals, and query paths.
7. `scene_emit/image_query.c`: image query FramePlan generation.
8. `scene_emit/scene_emit.h` and `scene_emit/internal.h`: narrow scene-emission declarations.

Guardrail: `scene_emit/` owns retained scene lowering into FramePlan nodes. It should not grow
runtime execution or backend resource realization.


### 5. Split Runtime Render Emission

Status: completed on 2026-05-28. Keep the current ownership unless later runtime work exposes new
coupling.

Current ownership:

1. `render_emit.c`: shared runtime-emitter labels, object-key helpers, and depth-peeling shader
   key helpers.
2. `render_emit_passes.c`: render-pass setup, graph/plain pass routing, target lookup, clear-only,
   texture, and compute-assisted render emission.
3. `render_emit_prepare.c`: per-visual draw preparation, pipeline selection, bind selection, and
   draw-count resolution.
4. `render_emit_draws.c`: viewport/scissor setup and prepared draw command emission.
5. `render_emit_bindings.c`: descriptor-set and bind-group selection helpers split from visual
   preparation.

Keep resource creation, uploads, graph targets, draw packet assembly, common bindings, and render
pass helpers in their current runtime files unless a move removes real duplication.

Validation: `just build`, `git diff --check`, `direnv exec . just test scene`, and at least one
offscreen/app smoke when command-buffer lifetime, render targets, graph resources, or descriptors
are touched.


### 6. Split Core Scene Ownership

Status: first batch completed on 2026-05-28. Keep the current ownership unless later work exposes
new coupling.

Current ownership:

1. `scene.c`: scene lifecycle plus figure and panel facade functions.
2. `scene_notify.c`: request-frame callbacks and visual/buffer mutation notifications.
3. `format_state.c`: shared format-state copying.
4. `frame_trace.c`: figure ids and FramePlan tracing.
5. `panel_geometry.c`: panel pixel/plot geometry and MVP helpers.
6. `panel_layout.c`: reserve/padding/layout policy and screen-space invalidation.
7. `grid.c`: grid tracks, margins, spans, and panel layout resolution.
8. `controllers.c`: controller lifecycle, links, panel input dispatch, fly stepping, and panel
   camera/controller APIs.
9. `figure_emit.c`: figure emission, pending-render checks, emitted-stream ownership, and live-stream
   mutation guards.

Guardrail: `_scene.h` remains broad after this batch. Prefer local private headers for later narrow
subsystem declarations instead of growing it further.


### 7. Split Annotation And Domain Helpers

Status: first annotation/domain batches completed on 2026-05-28.

Completed first slices:

1. `annotation/colormap.c`: colormap sampling, built-in stop tables, colormap object lifecycle, and
   colormap-driven scale dirtiness.
2. `annotation/scalebar.c`: scale-bar annotation constructor, realization, segment/text visual
   synchronization, and 2D/3D units-per-pixel resolution.
3. `annotation/legend.c`: categorical legend lifecycle, panel reserve policy, generated marker/text
   visuals, layout updates, and preparation.
4. `annotation/colorbar.c`: continuous colorbar lifecycle, panel reserve policy, ramp/tick/text
   generated visuals, layout updates, and preparation.
5. `annotation/text_font.c`: text font resource lifecycle and font-atlas lookup helpers shared by
   retained text and batched text preparation.
6. `annotation/scale.c`: retained scale lifecycle and categorical-entry state after colorbar/legend
   extraction.
7. `annotation/scale_internal.h`: narrow annotation-private scale/colorbar/legend dirty hooks used
   by split annotation files.
8. `domain/buffer.c`: scene buffer lifecycle, buffer payload mutation, visual buffer binding, copied
   visual index data, buffer reset/index helpers, and visual buffer release.

Continue after these slices.

Annotation candidates:

1. text object state and public setters;
2. text layout/reserve helpers;
3. generated glyph/quad visual synchronization;
4. axis tick generation versus axis generated-visual ownership;
5. remaining colorbar/legend polish only when a later behavior change exposes tighter boundaries.

Domain candidates:

1. sampled-field object lifecycle and public setters;
2. sampled interpretation and label/scale binding;
3. generated visual synchronization for image, volume, labels, colorbar, and legends;
4. any remaining polygon/polygon-set scene visual glue that a later behavior change exposes.


### 8. Split Visual Descriptor And Attribute Helpers

Status: first visual descriptor/attribute batch completed on 2026-05-28.

Current ownership:

1. `visuals/attrs.c`: public attribute source, mutability, buffer, and scale-binding facade.
2. `visuals/attr_schema.c`: visual attribute names, supported attributes, source policy, and
   attribute lookup/creation helpers.
3. `visuals/attr_data.c`: dense/string/range attribute data mutation, validation, and versioning.
4. `visuals/visual_bindings.c`: retained visual binding slot assignment, lookup, and clearing.
5. `visuals/visual_lookup.c`: scene-global visual index lookup and visual type names.
6. `visuals/desc.c`: render-node visual descriptor assembly from typed metadata and explicit
   untyped compatibility ids.
7. `visuals/desc_resources.c`: render visual resource-key/id resolution and persistent vertex-buffer
   lookup.
8. `visuals/desc_untyped_compat.c`: explicit compatibility resource-list classifiers for point,
   splat, primitive, image, and textured mesh descriptors.
9. `visuals/registry/`: first `DvzVisualFamilyOps` table, active-family coverage test, retained
   visual lowering, normal metadata fill for image/labels/volume, retained visual pass-capability
   resolution, and runtime bind/pipeline/shader/draw descriptor dispatch. Retained lowering,
   bind-descriptor bodies, and normal pipeline-descriptor bodies now live in active family folders.
   The shared default pass-capability hook lives in `visuals/pass_caps.c`, leaving
   `visuals/registry/` as registration glue. Draw hooks now live in active family folders and call
   the shared default draw helper until family-specific draw policy is needed. Point, pixel, marker,
   segment, path, sphere, splat, image, labels, glyph, primitive, mesh, volume, vector, and text
   shader hooks are family-owned; the generic shader resolver keeps shared helpers and
   compatibility/pass-policy dispatch.

Guardrail: do not split the remaining visual pipeline helpers by enum case alone. The long-term
direction is a registry-driven visual-family contract, not more generic switch files. Treat
`visuals/desc_untyped_compat.c` as temporary containment for untyped descriptor inference: normal
scene output now uses explicit typed metadata, and the compatibility path should be deleted when the
remaining low-level fixture/import use cases are migrated. The next visual split should move upload,
query, and bounds logic into family-owned files instead of adding more root-level visual switches.
See
[`SCENE_ARCHITECTURE_COMPLETION_PLAN.md`](SCENE_ARCHITECTURE_COMPLETION_PLAN.md).


### 9. Tighten Query And Interaction Boundaries

Status: query policy split started on 2026-05-28. `query/policy.c` owns target capability,
query-profile selection, drawable candidate selection, family-op eligibility lookup, and
framebuffer coordinate policy. `query/execute.c` now keeps retained executor schema reset,
static-upload bookkeeping, native family execution, and readback orchestration.

The query folder already has queue, executor, readback, registry, result, and execute files. The
next split should be policy-driven:

1. keep runtime readback orchestration in query execution files;
2. keep visual-family query geometry and result decoding in visual-family or query registry
   helpers;
3. keep CPU fallback or unsupported-policy decisions explicit and documented;
4. do not combine request-path cleanup with new picking semantics unless the feature requires it.


### 10. Rebalance Tests With Each Split

Do not perform a mechanical test-file split first. Move or add tests when an implementation split
creates a new stable boundary:

1. frame-plan resource/pass/dependency tests near FramePlan internals;
2. runtime emission tests for pass setup, binding selection, and draw counts;
3. visual-family payload tests for family-local cache builders;
4. query tests for request orchestration versus visual-family query payload decoding.


## Commit And Validation Rules

For each coherent split:

1. inspect `git status --short` before editing and before committing;
2. preserve comments and Doxygen blocks when moving code;
3. use `apply_patch` for edits;
4. run `just build`;
5. run `git diff --check`;
6. run `direnv exec . just test scene`;
7. add `just spec-check` when DRP2 schemas, fixtures, or emitted portable command shape changes;
8. run an offscreen or bounded GLFW smoke when runtime command buffers, render targets, descriptor
   refresh, graph resources, swapchains, or synchronization are touched;
9. stage only intended source/spec/agent files and leave `data`, `libs/vulkan/`, generated
   binaries, and unrelated WebGPU/example changes unstaged unless explicitly approved.


## Done Criteria For This Roadmap

This roadmap can move to a completed record only when:

1. visual-family helpers own their family-local API, query, bounds, metadata, pipeline/draw
   contract, and derived payload builders;
2. frame-plan lifecycle, resources, passes, dependencies, and readback bookkeeping have separate
   owner files;
3. the old `src/scene/plan/` bucket remains gone, with `frame_plan/`, `scene_emit/`, and
   `render_contract/` keeping separate ownership;
4. runtime render emission separates pass setup, visual draw emission, bindings, and draw counts,
   and does not infer normal visual-family semantics from resource tags;
5. core scene object ownership is split enough that new public object families do not edit a
   monolithic `scene.c`;
6. annotation/domain helpers have clear generated-visual and public-object boundaries;
7. coarse scene CMake layers can be reused without pulling in app/window/runtime code when a
   consumer only needs planning, scene object-model, or visual-family pieces;
8. tests name the boundary they protect rather than only the broad scene graph;
9. the final architecture confidence bar in
   [`SCENE_ARCHITECTURE_COMPLETION_PLAN.md`](SCENE_ARCHITECTURE_COMPLETION_PLAN.md) is satisfied.
