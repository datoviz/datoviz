# Scene Code Split Roadmap

This note records the next maintainability split for `src/scene`. It is broader than the visual
family layout note: it covers frame planning, runtime emission, core scene ownership, query
execution, annotations, techniques, and tests.

Normative status: informative implementation roadmap. Public semantics remain in the specialized
scene specs; this file only defines safe source-ownership direction for refactors.


## Current Pressure Points

As of 2026-05-28, the highest-value split candidates are:

1. `src/scene/core/scene.c`: broad object creation, lifecycle, slots, buffers, visuals, fields,
   panels, and emitted-stream bookkeeping still share one translation unit.
2. `src/scene/runtime/render_emit.c`: runtime command emission still combines pass setup,
   descriptor/bind decisions, visual draw emission, draw-count resolution, and graph/pass routing.
3. `src/scene/plan/render_contract.c`: visual contract resolution and pass/resource contract
   diagnostics are still broad enough to hide policy changes.
4. `src/scene/plan/visual_lowering_uploads.c`: some derived payload builders remain in plan code.
   Continue moving only pure cache/data construction into family or subsystem helpers.
5. `src/scene/annotation/text.c`, `axis.c`, and `scale.c`: retained annotation objects,
   layout/reserve policy, generated visuals, and text/glyph lowering are still mixed.
6. `src/scene/domain/field.c` and `polygon.c`: public domain object state, sampled interpretation,
   generated visual glue, and upload dirtiness are still mixed.
7. `src/scene/query/execute.c`: request execution is split better than before, but visual-family
   target policy and runtime readback orchestration should stay visibly separate.
8. Scene tests have useful focused files, but several broad scene-graph/runtime tests still cover
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

Target `src/scene/plan/render_contract.c` after the FramePlan facade is stable.

Suggested ownership:

1. `render_contract.c`: top-level contract build/validate facade.
2. `render_contract_visual.c`: visual-family contract assembly from resolved descriptors.
3. `render_contract_resources.c`: resource-role and bind-layout compatibility checks.
4. `render_contract_diagnostics.c`: drift/error messages and debug summaries.

Guardrail: contract files should report mismatches between declared plan state and runtime-facing
requirements. They should not decide new visual semantics.


### 4. Split Runtime Render Emission

Target `src/scene/runtime/render_emit.c` only after plan contracts are easier to inspect.

Suggested ownership:

1. `render_emit.c`: top-level pass/visual emission facade.
2. `render_emit_pass.c`: render-pass begin/end, viewport/scissor, graph/pass target selection.
3. `render_emit_visual.c`: resolved visual draw command emission.
4. `render_emit_bindings.c`: descriptor-set and bind-group selection from resolved plan metadata.
5. `render_emit_counts.c`: draw-count, index-count, instance-count, and generated-cache count
   resolution.

Keep resource creation, uploads, graph targets, draw packet assembly, common bindings, and render
pass helpers in their current runtime files unless a move removes real duplication.

Validation: `just build`, `git diff --check`, `direnv exec . just test scene`, and at least one
offscreen/app smoke when command-buffer lifetime, render targets, graph resources, or descriptors
are touched.


### 5. Split Core Scene Ownership

Target `src/scene/core/scene.c` after frame-plan/runtime churn is under control.

Suggested ownership:

1. `scene.c`: public facade, scene lifecycle, and high-level object ownership.
2. `figure.c`: figure creation, destruction, emitted-stream bookkeeping, and frame callbacks.
3. `panel.c`: panel creation, layout attachment, panel-local controller pointers, and bounds state.
4. `visual.c`: visual slot allocation, visual lifecycle, visual lookup, and visual-level dirty
   marking.
5. `buffer.c`: scene buffer creation, mutation, view binding, and live-stream mutation guards.
6. `field.c` facade or bridge: public sampled-field object entry points that delegate to
   `src/scene/domain/field.c` for interpretation and upload state.

Guardrail: do not split `_scene.h` into many private headers in the same commit. First move code
behind the existing contract; then narrow headers once dependencies are visible.


### 6. Split Annotation And Domain Helpers

Do this after core object ownership is less tangled.

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
