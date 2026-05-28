# Scene Code Split Roadmap

This note records the next maintainability split for `src/scene`. It is broader than the visual
family layout note: it covers frame planning, runtime emission, core scene ownership, query
execution, annotations, techniques, and tests.

Normative status: informative implementation roadmap. Public semantics remain in the specialized
scene specs; this file only defines safe source-ownership direction for refactors.


## Current Pressure Points

As of 2026-05-28 after the first source-split batch, the highest-value split candidates are:

1. `src/scene/plan/visual_lowering_uploads.c`: some derived payload builders remain in plan code.
   Continue moving only pure cache/data construction into family or subsystem helpers.
2. `src/scene/annotation/text.c`, `axis.c`, and `scale.c`: retained annotation objects,
   layout/reserve policy, generated visuals, and text/glyph lowering are still mixed.
3. `src/scene/domain/field.c` and `polygon.c`: public domain object state, sampled interpretation,
   generated visual glue, and upload dirtiness are still mixed.
4. `src/scene/visuals/desc.c` and `attrs.c`: descriptor resolution, retained metadata, and binding
   updates are better than before but still carry several ownership concerns.
5. Scene tests have useful focused files, but several broad scene-graph/runtime tests still cover
   multiple ownership boundaries at once.


## Split Principles

1. Preserve the scene -> FramePlan -> DRP2 -> runtime boundary. Source files may move, but
   dependencies should not point backward from runtime execution into scene semantics.
2. Do not move orchestration into family helpers. Family and subsystem helpers may build geometry,
   cache payloads, upload bytes, or validation-local decisions; plan code keeps resource keys,
   upload nodes, dependency ordering, dirty-state policy, and pass scheduling.
3. Runtime files should emit already-resolved plan state. They should not infer visual-family
   semantics that the visual or plan layer could resolve earlier.
4. Keep graph-technique files as graph builders and technique policy owners; keep generic
   graph-resource realization in runtime files.
5. Split by stable ownership, not by enum case. A smaller file is not a win if it duplicates policy
   or forces cross-file mutation of the same object invariants.
6. Use private subsystem headers once declarations are local to a folder. Avoid growing broad
   headers such as `_scene.h`, `_frame_plan.h`, or `_visual_internal.h` for narrow helpers.
7. Each commit should be behavior-preserving unless the commit message and tests make a targeted
   behavior change explicit.


## Recommended Sequence

### 1. Finish Low-Risk Derived Payload Extraction

Continue from `spec/scene/visuals/IMPLEMENTATION_LAYOUT.md`.

1. Move remaining pure builders from `plan/visual_lowering_uploads.c` into family or subsystem
   files only when the helper owns a coherent payload:
   1. mesh generated/index/upload payload helpers;
   2. labels/image generated query or upload payload helpers;
   3. glyph/text derived buffer helpers;
   4. any remaining stroke cache math shared by render and query paths.
2. Keep FramePlan resource-key allocation, upload-node creation, and upload ordering in
   `src/scene/plan/`.
3. Prefer private headers under the owning subsystem, such as `mesh/internal.h`, `text/internal.h`,
   or a narrow plan helper header, instead of adding family details to `_visual_internal.h`.

Validation: `just build`, `git diff --check`, and `direnv exec . just test scene`.


### 2. Split FramePlan Internals

Status: completed on 2026-05-28. Keep the current ownership unless a later behavior change exposes
new coupling.

Current ownership:

1. `frame_plan.c`: public/internal frame-plan lifecycle and top-level facade.
2. `frame_plan_nodes.c`: node storage growth, append/last helpers, and node accessors.
3. `frame_plan_capabilities.c`: capability snapshot defaults and copy helper.
4. `frame_plan_diagnostics.c`: diagnostic report helpers.
5. `frame_plan_resources.c`: upload node resources and upload metadata.
6. `frame_plan_graph_resources.c`: graph resource descriptors.
7. `frame_plan_passes.c`: compute/render/clear node appenders and render visual metadata.
8. `frame_plan_graph_passes.c`: graph pass descriptors, access builders, and attachments.
9. `frame_plan_dependencies.c`: dependency graph count/get facade.
10. `frame_plan_graph_helpers.c` and `frame_plan_graph_internal.h`: shared graph helper routines.
11. `frame_plan_graph_validation.c`: graph validation diagnostics.
12. `frame_plan_readback.c`: readback/copy metadata and request-facing bookkeeping.

Keep existing `frame_plan_ascii.c`, `frame_plan_fixture.c`, `frame_plan_emit.c`,
`visual_metadata.c`, and `image_query_plan.c` separate. Do not change FramePlan semantics while
moving code.

Validation: focused `direnv exec . just test scene/frame-plan` if available through the runner
filter, then full `direnv exec . just test scene`.


### 3. Split Render Contract Resolution

Status: completed on 2026-05-28. Keep the current ownership unless later contract work exposes
new coupling.

Current ownership:

1. `render_contract.c`: top-level contract build/validate facade.
2. `render_contract_visual.c`: visual-family contract assembly from resolved descriptors.
3. `render_contract_resources.c`: resource-role and bind-layout compatibility checks.
4. `render_contract_diagnostics.c`: drift/error messages and debug summaries.

Guardrail: contract files should report mismatches between declared plan state and runtime-facing
requirements. They should not decide new visual semantics.


### 4. Split Runtime Render Emission

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


### 5. Split Core Scene Ownership

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


### 6. Split Annotation And Domain Helpers

Status: started on 2026-05-28.

Completed first slices:

1. `annotation/colormap.c`: colormap sampling, built-in stop tables, colormap object lifecycle, and
   colormap-driven scale dirtiness.
2. `annotation/scale_internal.h`: narrow annotation-private scale dirty hook used by colormaps.
3. `domain/buffer.c`: scene buffer lifecycle, buffer payload mutation, visual buffer binding, copied
   visual index data, buffer reset/index helpers, and visual buffer release.

Continue after these slices.

Annotation candidates:

1. text object state and public setters;
2. text layout/reserve helpers;
3. generated glyph/quad visual synchronization;
4. axis tick generation versus axis generated-visual ownership;
5. scale/colorbar/legend layout versus generated visual construction.

Domain candidates:

1. sampled-field object lifecycle and public setters;
2. sampled interpretation and label/scale binding;
3. generated visual synchronization for image, volume, labels, colorbar, and legends;
4. polygon/polygon-set geometry construction versus scene visual glue.


### 7. Tighten Query And Interaction Boundaries

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


### 8. Rebalance Tests With Each Split

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

1. visual-family helpers own their family-local API, query, bounds, and derived payload builders;
2. frame-plan lifecycle, resources, passes, dependencies, and readback bookkeeping have separate
   owner files;
3. runtime render emission separates pass setup, visual draw emission, bindings, and draw counts;
4. core scene object ownership is split enough that new public object families do not edit a
   monolithic `scene.c`;
5. annotation/domain helpers have clear generated-visual and public-object boundaries;
6. tests name the boundary they protect rather than only the broad scene graph.
