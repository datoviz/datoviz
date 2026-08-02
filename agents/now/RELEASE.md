# Datoviz v0.4 Release Plan

Status: active roadmap from closed RC2 through RC3 and RC4 to final `v0.4.0`. Updated: 2026-08-01.

Use [STATUS.md](STATUS.md) for current blockers, [DOCUMENTATION.md](DOCUMENTATION.md) for documentation gates, [DISTRIBUTION_RELEASE_CHECKLIST.md](DISTRIBUTION_RELEASE_CHECKLIST.md) for packaging proof, and [../../spec/release/](../../spec/release/) for durable release policy.

## Scope Boundary

Datoviz v0.4 owns the C engine, native scene/app runtime, generated low-level Python binding with NumPy adaptation, raster capture, an experimental WebGPU/WASM subset, a narrow experimental compute-to-render path, reproducible packaging, and the release-pinned Modern GPU Graphics in Vulkan course.

GSP/VisPy2 owns high-level object-oriented plotting and publication-oriented vector export. v0.4 does not require v0.3 compatibility, full native/WebGPU parity, a general compute framework, broad CUDA/CuPy interop, complex text shaping, dashboards, or long-horizon backend refactors.

## Completed Milestones

RC1 established the public v0.4 candidate surface and exposed a packaged macOS native-window loader defect. RC2 is published and closed at tag `v0.4.0rc2`, release commit `8a3bd7509`; it fixes that defect and has complete immutable hosted, package-index, artifact, documentation, and physical MacBook M3 evidence with physical Linux and Windows recorded as unavailable.

Do not replay RC1 or RC2 execution instructions. Audit their tagged releases, assets, reports, and workflow evidence.

## 1. Post-RC2 Branch Cutover

Preserve old v0.3 `main` as `v0.3-maintenance`, rename `v0.4-dev` to `main`, make the renamed v0.4 line the GitHub default, and reconcile live branch-specific automation and guidance. Do not merge incompatible histories, rewrite commits, move tags, or force-update RC refs. Follow [BRANCH_CUTOVER.md](BRANCH_CUTOVER.md).

## 2. RC3 Documentation, Packaging, Quality, And Course Foundation

Completed RC3 implementation includes gallery-media policy and tooling, generated C/Python documentation inventories, dataset attribution and provenance, known limitations, visual-system pilot, canonical Linux screenshots, tutorial-facing API, unified shader toolchain, rewritten course chapters 1-3, local installed-consumer proof, Qt bridge implementation, local Apple Silicon split-package proof, and the local R1-R8 render-product/GTAO/public-AO implementation. Render R9, exact-head validation, integration, and affected QA remain release-quality gates rather than new feature scope.

Remaining RC3 deliverables:

1. Execute and verify the branch cutover.
2. Obtain maintainer review of the visual pilot, rewritten course voice and previews, and exact gallery publication candidates; decide focused PR #132 successors.
3. Publish compatible Vulkan-enabled Qt and PyQt packages, then build and validate exact split `libdatoviz`, `datoviz`, and `datoviz-qtbridge` artifacts on supported hosted platforms.
4. Prove the rewritten course and runtime shaderc against the first official package newer than RC2 on supported hosted platforms; retain honest live-resize and physical-machine exclusions.
5. Validate the final source bundle, six-wheel matrix, installed Python/CMake consumers, Windows vcpkg overlay, conda layouts, optional-provider diagnostics, third-party notices, and checksum/signing policy.
6. Run or explicitly disposition static analysis, memory/UB checks where practical, Vulkan validation, long-running loops, docs, gallery, example, WebGPU, query/readback, compute, recovery, and package gates.
7. Freeze RC3 notes, known issues, validation evidence, artifacts, and feedback request only after the exact release scope is fixed.

Hosted Linux and Windows exact-artifact validation is mandatory for RC3. Physical Linux and Windows should be restored when suitable machines are available; unavailable hardware remains an exclusion. Final requires the missing physical proof or an explicit maintainer-approved exception.

Optional RC3 candidates must not delay these gates: GSP Texture2D mesh integration, scene-owned multi-light support, hosted documentation preview, wind globe, prompt widget, Pyodide playground, hero composition, or broad visual polish.

## 3. RC4 Course And Installed Developer Experience

Required deliverables:

1. Complete rewritten course chapters 4-15: triangle, external shaders and reload, vertex/index buffers, push constants, matrices, depth/culling, mouse control, texture upload/sampling, lighting, and a generated real mesh.
2. Generate and validate a distinct preview for every chapter from canonical programs without committed binary prerequisites or `data` submodule changes.
3. Freeze the tutorial-facing API profile and document vklite as advanced/unstable with exact release compatibility.
4. Build and run every chapter from exact installed source archives and wheels through `find_package(datoviz CONFIG REQUIRED)`, packaged runtime shaderc, deterministic offscreen proof, and bounded live resize, input, depth, repeated-frame, and shutdown smoke.
5. Pass Vulkan validation, public-header and binding checks, source synchronization, links, navigation, captures, license review, known-issue review, and supported hosted-platform validation, or record explicit limitations.
6. Collect and disposition feedback on setup, progression, diagnostics, GPU/driver behavior, resize, input, concepts, ownership, and cleanup.

The required course uses generated Datoviz geometry and a procedural asymmetric texture. Suzanne and committed binary assets are optional polish only after the course is complete.

Planned non-blocking RC4 engineering lane: implement and physically validate the experimental Windows/NVIDIA counterpart of the existing Linux CUDA/Vulkan external-memory path. Follow [../../docs/tasks/2026-08-02-windows-cuda-vulkan-interop/STATUS.md](../../docs/tasks/2026-08-02-windows-cuda-vulkan-interop/STATUS.md) and [NEXT_STEPS.md](../../docs/tasks/2026-08-02-windows-cuda-vulkan-interop/NEXT_STEPS.md). This does not become an RC4 release gate unless the maintainer explicitly promotes it.

## 4. Final `v0.4.0`

Exit criteria:

1. Reproducible source and binary artifacts pass final packaging, installation, consumer, rendering, documentation, license, and known-issue gates.
2. RC4 course and installed-consumer feedback is resolved or recorded; final course media and compatibility wording are regenerated from exact release code.
3. Physical validation is complete or explicit maintainer-approved exceptions are recorded in evidence and notes.
4. Final screenshots, clips, README/website assets, release notes, and announcement text use current canonical examples and course output.
5. GitHub-Zenodo archiving is enabled and the exact version DOI, concept DOI, and release date appear consistently in `CITATION.cff`, citation documentation, final notes, and announcements.
6. The JOSS draft is submitted or explicitly deferred; acceptance remains separate from the software release.
7. `v0.4.0` is tagged and published with checksums, validation evidence, documentation, and direct feedback channels.
8. The active queue resets for v0.4 patch maintenance and v0.5 planning.

## Deferred Beyond v0.4

- Coordinated repository-history cleanup or object-database shrinking.
- Explicit linear `f16`/`f32` scientific image export/readback beyond the sRGB RGBA8 contract.
- Lower-risk scene structural cleanup, broader registry/backend modularity, long-horizon WebGPU parity, and advanced visuals not required by a release blocker.
- Additional package-manager channels beyond the validated wheel, selected conda, and vcpkg work when they would delay release.

## Publication Discipline

Prepare artifacts, reports, notes, and commands locally. Pushes, tags, releases, package uploads, documentation deployment, branch-setting changes, comments, pull requests, and other external publication require the approvals defined in `AGENTS.md`. Never stage or commit the `data` submodule, generated runtime libraries, binary payloads, or unrelated user changes without exact approval.
