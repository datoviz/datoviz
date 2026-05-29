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

For the next major visual-boundary phase, including checks that keep generic code from adding new
visual-family switches or family-private includes, use
[`SCENE_VISUAL_BOUNDARY_GUARDRAILS.md`](SCENE_VISUAL_BOUNDARY_GUARDRAILS.md).


## Current Pressure Points

As of 2026-05-29 after the scene-plan folder removal, upload/panel helper passes,
helper-declaration boundary pass, query scratch-helper sharing, query render-metadata guard,
shared item-id decode, shared item-target eligibility, query native-target policy cleanup,
sample-target native policy cleanup, FramePlan/render-contract metadata enforcement, typed
fallback label resolution for point/pixel/marker/splat/primitive/image/labels/textured mesh,
vector/stroke query-family decode ownership, field dirty propagation cleanup, and scalar field
sampling split through `14b1ab6f7`, plus axis text/tick helper splits, sampled-field binding
release ownership, and image/labels/volume unsupported-policy hooks on 2026-05-29, the
highest-value split candidates are:

1. `src/scene/query/` and family `visuals/*/query.c` files: query has a registry, split
   executor/policy/readback files, shared scratch helpers, standard item-id decoding, shared
   item-target eligibility for the simple item families, shared native-target fallback policy, and
   family-owned vector/segment/path item-result decoding. Image/labels/volume unsupported native
   target fallback now lives behind family hooks. Family scratch geometry and non-item result
   decoding outside the standard item-id path should still be checked family by family. Keep
   queueing, request freshness, executor lifecycle, common item decoding, common item-target
   eligibility, native-target fallback policy, and readback scheduling generic. Keep render
   metadata completeness in `frame_plan/` and render-contract validation, with query execution only
   consuming that invariant.
2. `src/scene/annotation/text.c` and `axis.c`: retained annotation objects, layout/reserve policy,
   generated visuals, and text/glyph lowering are still mixed. Axis text realization now lives in
   `axis_text.c`, and axis tick/domain planning lives in `axis_ticks.c`; remaining axis work is
   layout reserve and generated primitive-visual ownership. Scale, colorbar, legend, colormap,
   scale-bar, and text-font ownership now have first-pass owner files.
3. `src/scene/domain/field.c`: public domain object state and generated visual glue are still
   mixed. Field dirty propagation and bound-visual texture dirtiness now live in
   `domain/field_dirty.c`, scalar sampled-field value interpretation now lives in
   `domain/field_sample.c`, and sampled-field destroy binding release now lives in
   `domain/field_binding.c`.
4. `src/scene/scene_emit/uploads.c` and `scene_emit/derived_upload.c`: the upload path is mostly
   phase orchestration now. Continue moving only pure cache/data construction into family or
   subsystem helpers when a concrete mixed helper remains; do not re-extract dense/index/material
   upload emission or panel helper code that already moved.
5. `src/scene/visuals/`: descriptor and attribute helper ownership has a first split. Draw hooks
   have moved into active family folders, and active-family shader descriptor bodies have moved into
   family folders. Wide visual-family helper files and shared headers should only be split further
   when a stable owner boundary is clear. Volume retained-state bounds now live in the volume
   family, and mesh position/instance-transform bounds now live in the mesh family.
6. Standalone scene layers: `frame_plan/` and `render_contract/` are closest to reusable
   `src/scene`-local layers. `query/`, `text/`, `domain/`, and `visuals/registry/` can become more
   standalone after their remaining dependencies on broad retained-scene types and family-local
   hooks are narrowed.
7. Scene tests have useful focused files, but several broad scene-graph/runtime tests still cover
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


## Standalone Extraction Candidates

These are candidates for more independent locations or coarse reusable layers within `src/scene/`.
They should move only when includes and ownership prove the boundary, not as a directory shuffle.

1. `frame_plan/`: already the cleanest backend-neutral layer. It can become a standalone
   `datoviz_scene_frame_plan` object target once fixture/debug helpers and include dependencies are
   checked.
2. `render_contract/`: already mostly descriptor/FramePlan validation. It can stand apart from
   retained scene semantics if it keeps reporting mismatches rather than deciding visual behavior.
3. `query/`: the generic queue, result, readback, policy, registry, and executor files are a real
   subsystem. Scratch helpers and standard item-id decoding are already generic. The remaining work
   is to move family scratch geometry, non-item result decoding, and unsupported-policy decisions
   into family `query.c` files or narrow registry hooks.
4. `text/`: atlas/block/raster helpers are candidates for a reusable text service under
   `src/scene/text/`, while retained annotation text and generated visual synchronization should
   stay in `annotation/`.
5. `domain/`: buffers, sampled-field interpretation, scalar sampling, field texture payload
   construction, and polygon storage are candidates for a reusable retained-data layer once
   generated visual synchronization is separated from object state.
6. `visuals/registry/`: family operation registration and default hooks can become the stable
   contract between generic scene code and family folders. Root visual helper files should shrink
   toward registry dispatch plus shared defaults.
7. `scene_emit/`, `runtime/`, and `techniques/`: keep these as orchestration/runtime layers. They
   can be separate CMake layers, but they should not become scene-independent semantic owners.


## Recommended Sequence

### 1. Finish Query-Family Ownership

Status: generic query scratch helpers, standard item-id decode, standard item-target eligibility,
native-only target fallback policy, sample-target native policy, vector/segment/path family decode
ownership, image/labels/volume unsupported-policy hooks, and FramePlan-owned render-metadata
completeness checks completed on 2026-05-29. The next pickup is family-by-family ownership of
remaining scratch geometry and non-item result decoding outside the standard item-id path.

Current ownership:

1. `query/policy.c`: target capability, query-profile selection, candidate visual selection,
   family-op eligibility, shared item-target eligibility, native-target fallback policy, and
   framebuffer coordinate policy.
2. `query/execute.c`: retained executor schema reset, static-upload bookkeeping, native family
   execution, FramePlan render-metadata completeness consumption, and readback orchestration.
3. `query/scratch.c`: scratch destruction plus shared temporary allocation, dense-attribute,
   target-extent, and request-centered render-state helpers.
4. `query/result.c`: result freshness, queue push/poll, and standard r32uint item-id decode.
5. `query/queue.c`, `query/readback.c`, and `query/registry.c`: request queue, readback routing,
   and family-op lookup.
6. `visuals/<family>/query.c`: family query hooks for active queryable families.

Next moves:

1. Move remaining scratch-geometry builders into the owning family query file or a narrow shared
   visual subsystem when the geometry is family-specific.
2. Move non-item native result decoding and unsupported-target decisions into the owning family
   query file where practical. Keep standard r32uint item-id decoding in `query/result.c`; vector,
   segment, and path now select their own family ids before calling that helper. Keep the standard
   item/object eligibility policy in `query/policy.c`.
3. Keep query queueing, request freshness, readback scheduling, and executor lifecycle generic.
4. Add focused tests that distinguish orchestration behavior from family payload/result behavior.

Validation: `just build`, `git diff --check`, and focused query tests. The latest broad
`direnv exec . just test scene/query` run passed `40/40`; `direnv exec . just test fields` passed
`47/47`; focused WGSL typed-fallback and FramePlan static-render filters passed. Earlier
`direnv exec . just test scene/frame-plan` and `direnv exec . just test scene-graph` runs passed
`55/55` and `158/158`. The volume/labels readback failures that blocked the earlier split are no
longer recorded as current failures.


### 2. Finish Low-Risk Derived Payload Extraction

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


### 3. Split FramePlan Internals

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


### 4. Split Render Contract Resolution

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


### 5. Move Scene Emission Out Of The Old Plan Folder

Status: completed on 2026-05-28. The old `src/scene/plan/` folder has been removed; its ownership
is now split across `frame_plan/`, `render_contract/`, and `scene_emit/`.

Current scene-emission ownership:

1. `scene_emit/core.c`: retained scene -> FramePlan facade.
2. `scene_emit/panel.c`: panel render lowering and pass scheduling; drawable guards/counting and
   framebuffer-scaled viewport setup now have local helpers.
3. `scene_emit/uploads.c`: panel-visible visual upload phase orchestration.
4. `scene_emit/derived_upload.c`: derived-geometry upload orchestration for family-owned payloads.
5. `scene_emit/upload_support.c`: shared upload metadata/resource-role helpers plus dense-attribute,
   index-buffer, and material-trigger upload emission.
6. `scene_emit/metadata.c`: typed visual metadata emission.
7. `scene_emit/visual_lowering.c` and `scene_emit/visual_lowering.h`: visual-family lowering
   decisions shared by scene emission, render contracts, visuals, and query paths.
8. Image query FramePlan generation is no longer in `scene_emit/`; it is owned by
   `visuals/image/query.c` and related image query helpers.
9. `scene_emit/scene_emit.h` and `scene_emit/internal.h`: narrow scene-emission declarations.

Guardrail: `scene_emit/` owns retained scene lowering into FramePlan nodes. It should not grow
runtime execution or backend resource realization.


### 6. Split Runtime Render Emission

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


### 7. Split Core Scene Ownership

Status: source split completed on 2026-05-28 and helper-declaration boundary pass completed on
2026-05-29. Keep the current ownership unless later work exposes new coupling.

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
10. Narrow helper declarations now live in owner-private headers: text helpers in `text/` and
    `visuals/text/`, annotation preparation hooks in `annotation/`, polygon lifecycle helpers in
    `domain/`, frame tracing and figure emission helpers in `core/`, query request helpers in
    `query/`, format/panel-layout helpers in `core/`, shared visual lifecycle/binding helpers in
    `visuals/`, and scene notification helpers in `core/`.

Guardrail: `_scene.h` is no longer the bucket for narrow helper declarations. It remains broad
because it still carries shared retained scene object and type definitions. Future shrink work
should target type ownership and acyclic includes, not another round of prototype shuffling.


### 8. Split Annotation And Domain Helpers

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


### 9. Split Visual Descriptor And Attribute Helpers

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
   compatibility/pass-policy dispatch. Bounds dispatch now routes through the same registry, with
   family reducers for segment, vector, image, mesh, volume, sphere, and glyph and a shared default
   dense-position hook for the remaining active families. The generic upload path now reads
   position-topology and material-parameter upload policy from registry flags instead of local enum
   lists, and shared dense/index/material upload emission is factored into upload support helpers.

Guardrail: do not split the remaining visual pipeline helpers by enum case alone. The long-term
direction is a registry-driven visual-family contract, not more generic switch files. Treat
`visuals/desc_untyped_compat.c` as temporary containment for untyped descriptor inference: normal
scene output now uses explicit typed metadata, and the compatibility path should be deleted when the
remaining low-level fixture/import use cases are migrated. The next visual split should move upload,
query, and bounds logic into family-owned files instead of adding more root-level visual switches.
See
[`SCENE_ARCHITECTURE_COMPLETION_PLAN.md`](SCENE_ARCHITECTURE_COMPLETION_PLAN.md).


### 10. Tighten Query And Interaction Boundaries

Status: query policy split started on 2026-05-28; request-helper declarations, shared scratch
helpers, and render-metadata completeness checks live in `query/` as of 2026-05-29.
`query/policy.c` owns target capability, query-profile selection, drawable candidate selection,
family-op eligibility lookup, and framebuffer coordinate policy. `query/execute.c` keeps retained
executor schema reset, static-upload bookkeeping, native family execution, metadata completeness
checks, and readback orchestration. Current focused validation has `scene/query` passing `40/40`.

The query folder already has queue, executor, readback, registry, result, and execute files. The
next split should be policy-driven:

1. keep runtime readback orchestration in query execution files;
2. keep visual-family query geometry and non-item result decoding in visual-family or query
   registry helpers, while standard item-id decode remains generic;
3. keep CPU fallback or unsupported-policy decisions explicit and documented;
4. do not combine request-path cleanup with new picking semantics unless the feature requires it.


### 11. Rebalance Tests With Each Split

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
