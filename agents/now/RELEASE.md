# Datoviz v0.4 Release Plan

Status: active roadmap from closed RC2 through RC3 and RC4 to final `v0.4.0`. Updated: 2026-07-24.

Use [STATUS.md](STATUS.md) for current state, [DOCUMENTATION.md](DOCUMENTATION.md) for public documentation gates, [DISTRIBUTION_RELEASE_CHECKLIST.md](DISTRIBUTION_RELEASE_CHECKLIST.md) for packaging proof, and [../../spec/release/](../../spec/release/) for durable readiness, evidence, physical-validation, communication, and gallery-outreach policy.

## Scope Boundary

Datoviz v0.4 owns the C engine, native scene/app runtime, generated low-level Python binding with NumPy adaptation, raster capture, an experimental WebGPU/WASM subset, and a narrow experimental compute-to-render path. GSP/VisPy2 owns high-level object-oriented plotting and publication-oriented vector export.

Required release work is correctness and lifetime hardening, honest supported/experimental/deferred status, representative native and browser proof, complete generated-reference and binding guidance, reproducible packaging, attribution and license review, user-facing examples and release communication, and the release-pinned modern GPU graphics tutorial specified in [VKLITE_GRAPHICS_TUTORIAL.md](VKLITE_GRAPHICS_TUTORIAL.md).

Not required for v0.4 are v0.3 source or ABI compatibility, a new high-level plotting API, publication-quality vector export, complete WebGPU/native parity, complete advanced visual parity, a general compute framework, broad CUDA/CuPy interop, complex text shaping, dashboards, or long-horizon backend refactors.

## Completed Milestones

RC1 established the public v0.4 candidate surface and exposed a packaged macOS native-window Vulkan-loader defect. Its tag, artifacts, checksums, reports, documentation, and evidence are immutable historical records.

RC2 is published and closed at tag `v0.4.0rc2` and release commit `8a3bd7509`. It fixes the macOS packaged-loader handoff, preserves the RC1 API and feature scope, publishes six verified wheels plus release assets, passes hosted exact-artifact and package-index gates, and has attended MacBook M3 Quickstart and interaction proof. Physical Linux and Windows machines were unavailable for the narrow replacement campaign and were recorded as unavailable rather than passed.

Do not replay completed RC1 or RC2 execution instructions from old commits. Use the tagged releases, GitHub release assets, and recorded workflow runs when auditing historical evidence.

## 1. Post-RC2 Branch Cutover

Preserve the old v0.3 `main` tip as `v0.3-maintenance`, rename `v0.4-dev` to `main`, make the renamed v0.4 line the GitHub default, and reconcile branch-specific automation and guidance. Follow [BRANCH_CUTOVER.md](BRANCH_CUTOVER.md).

The cutover must not merge the incompatible v0.3 and v0.4 histories, rewrite commits, move release tags, or force-update RC refs. Git-history cleanup remains deferred beyond v0.4 and would require a separate coordinated plan and explicit approval.

Exit criteria:

1. `main` resolves to the former `v0.4-dev` tip, and `v0.3-maintenance` resolves to the former v0.3 `main` tip.
2. GitHub default-branch settings, protections, Actions triggers, badges, links, clone instructions, deployment recipes, and contributor guidance use the intended new names.
3. Intentional tag-pinned and historical RC1/RC2 links remain unchanged.
4. Fresh recursive clones of both maintained lines work, and no tag or release artifact changes.

## 2. RC3 Documentation, Packaging, Quality, and Tutorial API Candidate

Required deliverables:

1. Implement the two approved gallery-media checkpoints: canonical `1280x720` encoding and freshness policy, followed by bounded deterministic parallel generation.
2. Finish generated C reference coverage and usable Python documentation for top-level NumPy-adapted calls and exact `datoviz.raw` calls.
3. Freeze documentation structure, release examples, gallery captures, data attribution, prepared-data provenance, licenses, known issues, and release-note candidates.
4. Deliver and validate a packaged `datoviz_qtbridge` provider, preferably conda-first, without adding Qt to base wheels.
5. Triage RC feedback and PR #132, and retain only fixes or additions that fit the declared RC3 scope.
6. Keep representative WebGPU/WASM, query/readback, compute-to-render, runtime-recovery, and native render-conformance proof current for the advertised subset.
7. Make packaging, generated artifacts, release notes, documentation, and quality checks from [../../spec/release/READINESS.md](../../spec/release/READINESS.md) clean or record the remaining limitations explicitly.
8. Implement the tutorial-enabling Canvas, vklite, shader-file, borrowed-command, resolved-frame-format, dynamic-state, optional-depth, OBJ-UV, image-upload, and direct-arcball outcomes from executable chapter spikes, without adding a tutorial-only runtime path.
9. Implement [../../spec/architecture/SHADER_TOOLCHAIN.md](../../spec/architecture/SHADER_TOOLCHAIN.md): consolidate scene, Canvas, test, and example shader builds on one `glslc` helper; add release/CI SPIR-V validation; move runtime shaderc out of DRP2 into a focused thread-safe module; land typed availability, diagnostic, file, profile, and ownership API outcomes; and guarantee the provider in official packages while retaining disabled source builds.
10. Publish and validate the first three tutorial chapters through standalone installed CMake consumers, external runtime-compiled shaders, live GLFW execution, deterministic offscreen captures, Vulkan validation, generated binding checks, and supported hosted-platform proof.
11. End RC3 with only recorded RC3 blockers plus the explicitly planned RC4 tutorial chapters, asset work, installed exact-artifact proof, and full-course freeze remaining.

Physical-validation policy:

- Hosted Linux and Windows exact-artifact validation is mandatory for RC3.
- Exact-artifact physical Linux and Windows validation should be restored when suitable machines are available.
- Unavailable hardware is recorded as unavailable, never inferred from hosted results.
- Final `v0.4.0` requires either the missing physical proof or an explicit maintainer-approved exception recorded in the final evidence and release notes.

Optional RC3 candidates that must not delay the required gate:

- GSP Texture2D mesh integration with the complete field-slot sampling contract and native/WebGPU proof.
- Scene-owned multi-light support and the Klein-bottle showcase with complete native/WebGPU proof.
- Hosted documentation preview and manual promotion of exact documentation bytes, while preserving the guarded local publisher as fallback.
- Wind-globe, prompt-widget, Pyodide-playground, hero-composition, and broad visual-polish projects unless explicitly promoted by the maintainer.

## 3. RC4 Tutorial and Installed Developer Experience Candidate

Required deliverables:

1. Complete the tutorial chapters for per-frame transforms, indexed 3D geometry and depth, Suzanne texture sampling, mouse-driven arcball interaction, and normal-based basic lighting.
2. Ship the triangulated UV-unwrapped Suzanne OBJ and deterministic Datoviz-owned texture with exact provenance, licenses, generation/export recipes, hashes, package/install rules, and explicit approval for binary assets; do not use the `data` submodule.
3. Freeze the tutorial-facing API profile and document vklite as advanced/unstable with exact release compatibility rather than promising general low-level stability.
4. Build and run every chapter from exact installed source archives and wheels through `find_package(datoviz CONFIG REQUIRED)`, packaged runtime shaderc, external asset discovery, deterministic offscreen proof, and bounded GLFW resize, depth, arcball, repeated-frame, and shutdown smoke.
5. Pass strict Vulkan validation, public-header and binding checks, documentation source synchronization, links, captures, license review, known-issue review, and supported hosted-platform validation, or record explicit limitations.
6. Collect RC4 feedback on time-to-first-result, complete-course progression, shader diagnostics, GPU/driver behavior, resize and input, asset discovery, conceptual clarity, and ownership.
7. End with only recorded release or tutorial blockers remaining before final.

Physical-validation policy continues unchanged through RC4. Hosted proof is mandatory; unavailable physical hardware is an exclusion, and final requires the missing proof or an explicit maintainer-approved exception.

## 4. Final `v0.4.0`

Exit criteria:

1. Reproducible source and binary artifacts pass the final packaging, installation, consumer, rendering, documentation, license, and known-issue gates.
2. `v0.4.0` is tagged and published with checksums, validation evidence, release notes, and public documentation.
3. Launch screenshots, short clips, README and website assets, and announcement text are generated from current canonical gallery examples and the final tutorial result where appropriate.
4. Direct feedback channels are open for early users, especially scientists whose public datasets appear in showcase examples.
5. GitHub–Zenodo archiving is enabled, and the GitHub release, `CITATION.cff`, citation documentation, final notes, and announcement contain the exact version DOI, concept DOI, and release date.
6. Any future `.zenodo.json` remains consistent with `CITATION.cff` because Zenodo gives it precedence.
7. The JOSS draft is submitted or explicitly deferred; JOSS acceptance remains separate from the software release.
8. RC4 tutorial and installed-consumer feedback is resolved or recorded as a known limitation, and the release-pinned tutorial is published with its final assets and compatibility statement.
9. The active queue resets for v0.4 patch maintenance and v0.5 planning.

## Deferred Beyond v0.4

- Coordinated repository-history cleanup or object-database shrinking.
- Explicit linear `f16`/`f32` scientific image export/readback beyond the sRGB RGBA8 v0.4 contract.
- Lower-risk structural cleanup in large scene files, broader shader/visual registry cleanup, backend modularity, long-horizon WebGPU parity, and advanced visual work unless needed for a release blocker.
- Additional package-manager channels beyond the required wheel and selected conda/vcpkg work when they cannot be validated without delaying the release.

## Publication Discipline

Prepare artifacts, reports, notes, and commands locally. Pushing the active development branch requires explicit approval in the current turn; tags, releases, package uploads, documentation deployment, branch-setting changes, comments, pull requests, and other external publication require explicit approval of the exact final content and action.

Never stage or commit the `data` submodule, `paper/paper.pdf`, generated runtime libraries, or build-local media without the repository-required exact approval.
