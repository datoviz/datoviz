# Datoviz v0.4 Release Plan

> **Execution Status**
> - **Status:** `ACTIVE RELEASE ROADMAP`
> - **Updated on:** `2026-05-28`
> - **Purpose:** provide the step-by-step route from the current v0.4 branch to `v0.4.0`
> - **Audience:** maintainers and agents coordinating feature freeze, release candidates, and final
>   release work

This is the master release checklist for Datoviz v0.4. Future agents should start here when asked to
work toward release, then follow the linked implementation, specification, and validation records for
the current phase.

Use this document together with:

1. [START.md](START.md) for current branch context and active technical priorities.
2. [STATUS.md](STATUS.md) for remaining C implementation lanes and parallel-work
   coordination.
3. [DOCUMENTATION.md](DOCUMENTATION.md) for public documentation deliverables, API/docs inventory,
   RC documentation gates, and final documentation artifacts.
4. [../../spec/scene/examples/PLANNING.md](../../spec/scene/examples/PLANNING.md)
   for release example staging.
5. [../../spec/scene/validation/RENDER_CONFORMANCE.md](../../spec/scene/validation/RENDER_CONFORMANCE.md)
   for the planned render-conformance lane.


## Current Release Position

The active v0.4 codebase is close to feature freeze for the native scene path. Basic rendered text,
2D axes, generated ticks, axis labels, continuous colorbars, label annotations, and scale bars are
implemented enough that they should not be treated as primary feature-freeze blockers. Pinned
readouts currently have public/bookkeeping state and formatting; richer rendered readout UI can be
polish or v0.5 work unless a release example depends on it.

The gallery strategy still requires true retained textured mesh, with no baked-color workaround.
The narrow UV-mapped mesh texture slice has landed; remaining release work is deterministic
terrain/planet or fixture capture proof before the final v0.4 gallery can be considered honest.

Scale-bar rendering is present, including the retained update-performance refactor captured in
[../done/SCENE_SCALEBAR_UPDATE_PERF_REFACTOR.md](../done/SCENE_SCALEBAR_UPDATE_PERF_REFACTOR.md).
Panzoom/domain changes now avoid rebuilding text/glyph resources when the formatted label and text
style are unchanged.

The remaining feature-freeze blockers are:

1. **WebGPU/WASM experimental path:** a documented and tested browser/backend subset, not full
   native parity; browser proof exists, while WASM scene-emission disposition remains open.
2. **v0.3 visible parity audit:** visible capability parity or explicit deferral, not source/API
   compatibility.
3. **Public API/status cleanup:** supported, experimental, advanced/unstable, deferred, and
   external/GSP labels are clear.
4. **Release example proof:** a compact set of examples demonstrates the declared feature set,
   including retained textured mesh.

Completed RC1-critical lanes that must remain in validation:

1. **Raw `ctypes` API:** generated low-level Python bindings track the intended v0.4 C API surface;
   keep `just bindings` in release validation.
2. **Retained textured mesh:** UV attributes, mesh-bound sampled textures, texture-mode mesh
   shader/pipeline variant, sampler defaults, material integration, focused tests, and a live C
   proof example have landed; keep fixture/gallery capture proof in release validation.


## Scope Decisions

### Required Before Feature Freeze

1. Native scene/app path covers the declared v0.4 visual and interaction subset.
2. Retained textured mesh has a narrow but real implementation and deterministic example proof.
3. WebGPU/WASM has an experimental subset with explicit unsupported-feature diagnostics.
4. Raw `ctypes` generation and smoke tests work for the intended public C surface.
5. v0.3 visible capability gaps are fixed, explicitly deferred, or moved to GSP/VisPy2.
6. Core examples compile and exercise the release feature set.

### Not Required For v0.4 Feature Freeze

1. Exact v0.3 source or ABI compatibility.
2. High-level object-oriented Python plotting wrappers.
3. Publication-quality PDF/SVG/vector export from Datoviz.
4. Full WebGPU parity with the native Vulkan runtime.
5. Complex text shaping, TeX/math layout, rich labels, and label collision solving.
6. Rich rendered pinned readout UI beyond the state/formatting and examples needed for v0.4.
7. First-class vector/arrow visual, mesh face/region picking, path curve picking, volume DVR/MIP
   ray-hit picking, labels GPU probing, lasso selection, scene-level compute, custom shader APIs,
   CUDA interop, LOD/out-of-core policies, and full dashboard/application APIs.

### External Scope

1. GSP/VisPy2 owns the high-level OO Python and plotting APIs.
2. GSP/Matplotlib owns publication-oriented vector export.
3. Datoviz v0.4 owns the C engine, native scene/app path, low-level/generated Python binding
   surface, raster capture, and experimental WebGPU/WASM portability lane.


## Phase 1: Feature-Freeze Candidate

Goal: decide that v0.4 feature work is complete enough to move into API, docs, validation,
packaging, and release-candidate work.

Checklist:

1. Write or update the v0.4 feature status table.
2. Mark each feature as `supported`, `experimental`, `advanced/unstable`, `deferred`, or
   `external/GSP`.
3. Close or explicitly defer every item in [STATUS.md](STATUS.md) that is still
   listed as feature-freeze critical.
4. Keep retained textured mesh in RC validation and add or promote the deterministic terrain/planet
   or fixture capture proof.
5. Verify that text, axes, ticks, axis labels, colorbars, annotations, and scale bars are represented
   in examples/tests and are not stale planning-only claims.
6. Decide whether any rendered pinned readout work is required for release examples; otherwise mark
   richer readouts as polish or v0.5.
7. Keep the landed retained scale-bar update-performance churn tests and live example smokes
   represented in release validation.
8. Reconcile [STATUS.md](STATUS.md),
   [../../spec/scene/examples/PLANNING.md](../../spec/scene/examples/PLANNING.md),
   and the public feature table so completed first slices are not still presented as active
   feature-freeze blockers.
9. Decide or explicitly defer unresolved API-shape questions that affect RC1 feedback, especially
   unified panel query versus pick/probe polling and callback wording versus the retained polling
   model.

Suggested validation:

1. `git diff --check`
2. `just build`
3. `just test scene`
4. `just spec-check`

Exit criteria:

1. No unclassified release feature remains.
2. Every remaining feature gap has a recorded disposition.
3. The branch is ready for a v0.3 visible parity audit.


## Phase 2: v0.3 Visible Parity Audit

Goal: avoid surprising visible regressions compared with v0.3 while preserving freedom to break old
APIs.

Checklist:

1. Audit visible capabilities, not old symbol names:
   retained scene workflow, offscreen/GLFW app, screenshot/capture, frame callbacks, multi-panel
   figures, text, axes, colorbars, panzoom, arcball/fly/turntable, point, pixel, marker, primitive,
   segment/path, image, mesh including retained textured mesh, sphere, and volume.
2. Compare examples and v0.3-visible feature families against the v0.4 example staging table.
3. For each gap, choose one disposition:
   fix before RC1, document as deferred, or move to GSP/VisPy2.
4. Produce a short migration/status table from v0.3 to v0.4.

Exit criteria:

1. The visible regression table exists and is linked from release notes or release docs.
2. No v0.3-visible gap is left implicit.


## Phase 3: WebGPU/WASM Experimental Slice

Goal: ship an honest experimental browser path without claiming native feature parity.

Primary references:

1. [../soon/runtime/DRP2_WEBGPU_SUPPORT_PLAN.md](../soon/runtime/DRP2_WEBGPU_SUPPORT_PLAN.md)
2. [../soon/runtime/SCENE_WASM_WEBGPU_PORT_PLAN.md](../soon/runtime/SCENE_WASM_WEBGPU_PORT_PLAN.md)
3. [../../spec/scene/integration/WEBGPU_WASM.md](../../spec/scene/integration/WEBGPU_WASM.md)

Checklist:

1. Define the supported subset, initially point, primitive, image, and preferably basic mesh.
2. Ensure WGSL scene emission and the DRP2/WebGPU runner work for the subset.
3. Add explicit diagnostics for unsupported commands, visual families, shader variants, and runtime
   features.
4. Add one browser-visible demo, fixture dashboard, or runnable page.
5. Keep scene semantics shared; do not fork a WebGPU-only scene contract.

Current automated evidence:

1. `just webgpu-fixture-preflight` passes `39/39` strict WebGPU fixture checks.
2. `just webgpu-runner-smoke` passes the browserless WebGPU runner smoke over `37` positive DRP2
   fixtures, `2` WebGPU attachment streams, and `81` semantic negative fixtures.
3. Scene-emitted WGSL fixtures exist for point, primitive, and image visuals.
4. The committed runner subset covers the current point/primitive/image scene fixtures, texture
   sampling, multiple color attachments, depth attachments, copy commands, compute dispatch,
   readback, dynamic buffer updates, and destroy/lifetime validation.
5. The browser fixture dashboard passed `120/120` checks on 2026-05-28 after the
   repeated-runtime-frame smoke slice (`183812f27`).
6. The browser fixture dashboard passed fixture compatibility `120/120` and retained runtime
   stress `4/4` on 2026-05-29 after `292e82899`.
7. The remaining RC1 decision is whether WASM scene emission is required before feature freeze or
   remains an experimental follow-up.

Suggested validation:

1. `just webgpu-fixture-preflight`
2. portable native scene/DRP2 build check, when available
3. Emscripten compile, when available
4. browser or Node smoke for the supported subset

Exit criteria:

1. WebGPU/WASM is documented as experimental.
2. The subset is executable and tested enough for RC1.
3. Known gaps are explicit.


## Phase 4: Raw `ctypes` API

Goal: ship generated low-level Python bindings that load and reflect the v0.4 C API honestly.

Checklist:

1. Audit public headers intended for generated bindings.
2. Decide which headers, symbols, structs, enums, callbacks, and feature-gated APIs are included.
3. Update the generator as needed for v0.4 header shape.
4. Regenerate `datoviz/_ctypes.py`.
5. Add Python smoke tests for shared-library loading, create/destroy paths, and a tiny scene render
   or capture when runtime support is available.
6. Document that high-level OO Python plotting belongs to GSP/VisPy2.

Current RC1 evidence: `just bindings` now builds, regenerates the API JSON and raw ctypes module,
validates C ABI layout facts for generated structs, checks import/load/create-destroy, runs a tiny
offscreen point render smoke that skips cleanly when no GPU/runtime is available, and verifies both
editable and wheel installs. Examples live under `examples/python/raw/`.

Suggested validation:

1. `just ctypes`
2. `just ctypes-check`
3. `just ctypes-smoke`
4. `just ctypes-render-smoke`
5. `just ctypes-package-smoke`
6. `just bindings`

Exit criteria:

1. Raw bindings load on the supported development platform.
2. Generated symbols match the intended v0.4 C surface.
3. Python API scope is documented.


## Phase 5: API And Documentation Review

Goal: make the release surface coherent before users start testing release candidates.

Checklist:

1. Complete the RC1 API/docs inventory and public-surface classification described in
   [DOCUMENTATION.md](DOCUMENTATION.md).
2. Update the required README, build, release-note, feature-status, migration, and known-issues
   deliverables listed in [DOCUMENTATION.md](DOCUMENTATION.md).
3. Keep active v0.4 design material out of legacy `docs/` unless the public documentation migration
   has explicitly started.
4. Prepare a minimal RC1 user guide outside `docs/`, preferably at the repository root, so RC1
   users have a temporary v0.4 source of truth before the full documentation migration. Link it
   from `README.md` and the GitHub pre-release body.

Exit criteria:

1. Users can tell what is supported, experimental, advanced, or deferred.
2. Public headers and docs no longer imply obsolete v0.3 APIs are current.


## Phase 6: v0.4.0-rc1

Goal: publish an API and architecture candidate.

RC1 can still have visual polish gaps and incomplete gallery automation. It should not be a loose
development snapshot.

Required artifacts:

1. exact commit and tag,
2. feature status table,
3. v0.3 visible parity notes,
4. known issues,
5. API inventory or public-surface summary,
6. raw `ctypes` scope and smoke result,
7. WebGPU/WASM experimental scope and smoke result,
8. core example list and validation result,
9. explicit deferred-feature list aligned with
   [../../spec/scene/validation/DEFERRED_TRACKER.md](../../spec/scene/validation/DEFERRED_TRACKER.md),
10. release-staging reconciliation for examples that prove text, axes, colorbars, annotations,
    scale bars, pick/probe, sampled fields, retained textured mesh, sphere, volume, and dense point
    coverage,
11. temporary RC1 user guide at the repository root, for example `V0_4_RC1.md`, explaining that
    legacy `docs/` and the public website may still describe v0.3, and giving source-build,
    optional wheel/raw-`ctypes`, example-smoke, known-issues, feedback, and
    supported/deferred-scope instructions.

Minimum validation:

1. `git diff --check`
2. `just build`
3. `just test scene`
4. `just test drp2`
5. `just spec-check`
6. WebGPU fixture preflight
7. ctypes smoke
8. selected offscreen/GLFW example smokes, including a narrow scale-bar fixture

Exit criteria:

1. RC1 can be published with honest known issues.
2. Feedback requested from users is focused on API shape, architecture, installability, and obvious
   feature gaps.
3. Remaining text, axes, annotation, readout, picking, and visual-family polish is either tied to a
   required release example or clearly staged for RC2/v0.5; scale-bar update-performance has landed
   with focused churn tests and live example smoke.


## Phase 7: Render Conformance And Gallery

Goal: add enough automated visual confidence and public examples for later release candidates.

Primary references:

1. [../../spec/scene/validation/RENDER_CONFORMANCE.md](../../spec/scene/validation/RENDER_CONFORMANCE.md)
2. [../../spec/scene/validation/AUTOMATED_TESTING_STRATEGY.md](../../spec/scene/validation/AUTOMATED_TESTING_STRATEGY.md)
3. [../../spec/scene/examples/PLANNING.md](../../spec/scene/examples/PLANNING.md)

Checklist:

1. Add the minimal render-conformance harness or a narrow equivalent.
2. Start with a small stable fixture set:
   point, image plus colormap, mesh plus depth, textured mesh, multi-panel scissor, text/axis
   labels, colorbar, volume or sphere, and one technique such as EDL, SSAO, WBOIT, or MSAA.
3. Use exact image refs only where stable; otherwise use nonblank checks, expected sampled pixels,
   changed-region checks, and metamorphic assertions.
4. Add failure artifacts for actual/diff images when image comparison is active.
5. Keep the retained scale-bar update-performance churn tests or DRP2 trace smoke in the release
   validation set.
6. Build release gallery examples:
   scatter with axes, linked image probe/colorbar, protein, brain/volume, LiDAR or dense point
   cloud, textured terrain or planet surface, and one WebGPU/WASM subset page.
7. Add gallery/example metadata and asset-policy notes before broadening showcase examples.
8. Use the active view/canvas raster capture path for gallery screenshots; keep render-scale,
   panel-as-texture, and native vector/PDF/SVG export out of the v0.4 promise.

Exit criteria:

1. Minimal render-conformance checks run repeatably.
2. The gallery proves the declared v0.4 feature set.
3. Full backend image matrices remain optional until after RC1, but a small stable subset exists
   before final.


## Phase 8: v0.4.0-rc2

Goal: publish a documentation and gallery candidate.

Required artifacts:

1. the RC2 documentation and gallery artifacts listed in [DOCUMENTATION.md](DOCUMENTATION.md),
2. minimal render-conformance fixture result,
3. updated known issues from RC1 feedback.

Exit criteria:

1. A new user can understand what v0.4 is, install or build it, run examples, and see the intended
   Python-binding and WebGPU scope.


## Phase 9: Quality, Packaging, CI, And Legal Review

Goal: make release artifacts reproducible and reduce release-day surprises.

Checklist:

1. Run broad tests and focused graphics smokes.
2. Run static analysis where practical and triage actionable findings.
3. Check source build, wheels, shared-library loading, runtime dependencies, fonts, shaders, assets,
   and optional dependency fallbacks.
4. Validate supported platforms as far as available CI and hardware allow.
5. Audit vendored libraries, fonts, shaders, datasets, gallery assets, and license notices.
6. Add install-after-build tests for wheels and source distributions.
7. Capture the validation matrix for release notes.
8. Smoke-test component/package-consumer build paths where practical, including the modular target
   split and out-of-tree package-consumer cases tracked in [../later/SPLIT.md](../later/SPLIT.md).

Suggested validation:

1. `just build`
2. `just test`
3. focused GPU/offscreen/GLFW smokes
4. `just spec-check`
5. ctypes and package install smokes
6. docs and link checks, when documentation tooling is selected

Exit criteria:

1. Release artifacts build reproducibly from documented commands.
2. Wheels/source archives are smoke-tested after installation.
3. Asset and dependency licensing is known.


## Phase 10: v0.4.0-rc3

Goal: publish a packaging and quality candidate.

Required artifacts:

1. wheels and source artifacts,
2. package install test results,
3. CI/platform validation matrix,
4. static-analysis or quality-audit summary,
5. render-conformance minimum result,
6. docs link/example validation result,
7. release notes and migration notes close to final.

Exit criteria:

1. No known release blocker remains.
2. Only blocker fixes are allowed after RC3 unless a new feature fixes a declared v0.4 release gap.


## Phase 11: v0.4.0 Final

Goal: publish the final v0.4.0 release.

Checklist:

1. Apply only release-blocking fixes after RC3.
2. Re-run the final validation matrix.
3. Tag the final release.
4. Publish the final documentation artifacts listed in [DOCUMENTATION.md](DOCUMENTATION.md).
5. Publish website/gallery/release announcement assets.

Exit criteria:

1. `v0.4.0` is tagged and published with reproducible artifacts.
2. Known limitations are documented rather than implicit.
3. The next active work queue is reset for v0.4 patch work and v0.5 planning.


## After v0.4.0: Refactor Queue

After the final release, move non-blocking cleanup to
[../later/POST_V0_4_REFACTOR_ROADMAP.md](../later/POST_V0_4_REFACTOR_ROADMAP.md). The first
post-release refactor batch should focus on:

1. lower-risk `src/scene/scene.c` structural cleanup that was not needed for v0.4;
2. shared DRP2 diagnostics for app trace normalization and future render-conformance snapshots;
3. out-of-tree package-consumer smoke tests before tightening component-owned header installs;
4. conservative shared-fixture test migrations while keeping failure-path, resize, pick/probe,
   GLFW, video, and device-lost tests isolated until audited.

Do not let these post-release refactors delay `v0.4.0` unless one becomes necessary to fix a
release-blocking bug.


## Recommended Agent Workflow

1. Start each release task by reading this file and the current phase's linked references.
2. Keep changes scoped to one phase or one checklist item.
3. Update this checklist, [START.md](START.md), [STATUS.md](STATUS.md), or
   [DOCUMENTATION.md](DOCUMENTATION.md)
   when a release-blocking item changes state.
4. Move completed focused plans from `agents/now/` or `agents/soon/` to `agents/done/` when they
   are no longer active.
5. End every code-changing slice with `git diff --check` and the narrowest relevant build/test
   command.
6. For documentation-only checklist updates, run `git diff --check` and inspect `git status --short`.
