# Datoviz v0.4 Status

Status: active post-RC2 work toward RC3, then RC4 and final v0.4.0. Updated: 2026-07-24.

Keep this file current and short. Durable behavior belongs in `spec/`; completed implementation and campaign detail belongs in Git history, release assets, and tagged documentation.

## Current Position

RC2 is published and closed. Tag `v0.4.0rc2` resolves to release commit `8a3bd7509`; the GitHub prerelease has 11 verified assets; all six canonical wheels are on PyPI; production package-index verification run `29703021322` passed every clean-install lane and the aggregate gate; and post-publication source Test run `29703704413` passed Linux, macOS, and Windows.

The canonical RC2 evidence is immutable: Wheels run `29695746332`, exact-artifact hosted conformance run `29696169890`, physical MacBook M3 intake run `29697580837`, TestPyPI verification run `29698900673`, and artifact-neutral source Test run `29696633533` all passed their declared gates. Physical Linux and Windows machines were unavailable for the replacement campaign and remain explicit exclusions, not passes.

Website commit `5e9e6a4` deploys the refreshed gallery media produced from source commit `04225a00b`, preserves `/v0.3/`, and matches the audited local media hashes. The immutable RC2 PyPI-description limitation is recorded in the RC2 release notes.

The GitHub default and active development branch remain `v0.4-dev`. The old v0.3 line remains at `main` until the branch cutover in [BRANCH_CUTOVER.md](BRANCH_CUTOVER.md) is explicitly approved and executed.

## Post-RC2 Execution Order

1. Preserve old `main` as `v0.3-maintenance`, rename `v0.4-dev` to `main`, and update live branch-specific automation and guidance without merging the incompatible histories or rewriting commits.
2. Implement the two gallery-media checkpoints in [GALLERY_MEDIA_SINGLE_RESOLUTION.md](GALLERY_MEDIA_SINGLE_RESOLUTION.md): first the single-resolution encoding/freshness policy, then bounded parallel generation.
3. Execute the RC3 tutorial checkpoints in [VKLITE_GRAPHICS_TUTORIAL.md](VKLITE_GRAPHICS_TUTORIAL.md): prove and improve the tutorial-facing Canvas, vklite, shader-file, depth, OBJ-UV, image-upload, and arcball boundaries, then publish the three-chapter pilot through an installed CMake consumer.
4. Freeze the non-tutorial RC3 documentation and gallery surface, complete generated C and Python reference coverage, finish data attribution and provenance, and triage remaining feedback including PR #132.
5. Package and validate a `datoviz_qtbridge` provider, preferably conda-first, without adding Qt to the base wheel contract.
6. Cut RC3 after its required API, pilot, packaging, documentation, and quality gates pass; then complete and freeze the full tutorial in RC4 before final v0.4.0.
7. Reassess optional candidates only after the required RC3 and RC4 release work is on track.

## Active Gates

| Lane | Current state | Next decision or proof |
| --- | --- | --- |
| Branch cutover | Pending; GitHub defaults to `v0.4-dev`, old v0.3 remains `main`, and `v0.3-maintenance` does not yet exist. | Audit settings and live references, approve the exact external operations, execute the cutover, then verify branches, protections, workflows, links, and fresh clones. |
| Gallery media pipeline | The approved plan is recorded but not implemented. Published animation freshness is current for 38 animations and 29 video cards. The separate source-screenshot checker currently reports four stale and 100 uncached screenshots, with the former invalid dimensions and missing datetime-axis image resolved. | Implement and test the `1280x720` encoding ladder and freshness policy, then add bounded parallel execution as a separate checkpoint; keep generated media and `data` changes uncommitted unless separately approved. |
| Tutorial API and pilot | The durable educational contract, shader-toolchain architecture, and RC3/RC4 execution plan are recorded; the current advanced offscreen triangle is a seed, not the beginner tutorial, and the tutorial-facing API spikes have not begun. | Complete the first-result checkpoint, consolidate native shaders on `glslc`, move runtime shaderc into a focused thread-safe module, land the typed diagnostics API, package and validate the provider, and publish the three-chapter pilot before RC3. |
| Documentation and gallery freeze | Public RC2 documentation is deployed; generated-reference completion, source-screenshot cache reconciliation, attribution, provenance, visual-pilot review, final RC3 inventory, and tutorial pilot remain. | Freeze the non-tutorial surface in RC3 while publishing the pilot; reserve the planned chapters and tutorial asset freeze for RC4 rather than treating them as accidental post-RC3 polish. |
| Qt/PyQt provider | Source-built hosting and diagnostics are proven; canonical RC2 wheels do not contain the native bridge. | Deliver a tested packaged provider for RC3, preferably conda-first, while retaining optional base-wheel Qt probes and explicit missing-provider diagnostics. |
| Linux and Windows physical proof | Hosted build, packaging, clean-install, rendering, and consumer evidence is current; fresh physical machines are unavailable to the maintainer. | Hosted proof is mandatory for RC3. Attempt exact-artifact physical proof when suitable machines are available; final release requires either that proof or an explicit maintainer-approved exception. |
| Runtime and experimental paths | Native runtime, retained visuals, broad query paths, WebGPU/WASM subset, and narrow compute-to-render interop have recorded proof. | Fix concrete lifetime, resize, descriptor, repeated-frame, recovery, or parity blockers; keep WebGPU and compute scope honest and do not broaden v0.4 into a general compute or browser-parity project. |

## Required RC4 Gate

RC4 completes the installed "Modern GPU Graphics in C" course through textured, lit, mouse-rotatable Suzanne; freezes the tutorial-facing API profile; ships reviewed OBJ and texture assets outside the `data` submodule; and proves every chapter from exact installed packages on supported hosted platforms. Follow [VKLITE_GRAPHICS_TUTORIAL.md](VKLITE_GRAPHICS_TUTORIAL.md); do not move first-time public API design, asset provenance, or source-tree-only assumptions into the final release gate.

## Optional RC3 Candidates

- Reassess the GSP Texture2D mesh integration only if stabilization leaves room for the complete public sampling API, deterministic fixtures, conversion-free linear RGBA behavior, and native/WebGPU validation.
- Reassess the multi-light Klein-bottle slice only if the complete scene-owned light API, fixed-capacity panel-local sets, two-sided lighting, shared example preset, and native/WebGPU validation can land without delaying RC3.
- Improve hosted documentation preview and exact-byte promotion when practical before final, but do not delay RC3 solely for that infrastructure.
- Treat wind-globe, prompt-widget, Pyodide-playground, hero-composition, and broad visual-polish work as optional unless the maintainer explicitly promotes an item into the required release gate.

## Locked Decisions

- Git history cleanup is deferred beyond v0.4; do not rewrite RC or final-release refs.
- The v0.4.0 screenshot/export contract is sRGB RGBA8; explicit linear `f16`/`f32` export/readback is deferred.
- v0.3 source and ABI compatibility must not constrain v0.4 architecture.
- High-level object-oriented plotting and publication vector export remain external GSP/VisPy2 scope.
- Point-cloud public WebGPU redistribution remains delisted because the source dataset is not licensed for redistribution; native capture and localhost-only development proof may remain.
- The modern GPU graphics tutorial is required for final v0.4. RC3 owns enabling API and the three-chapter pilot; RC4 owns full-course completion and freeze; final owns feedback fixes and publication.
- Runtime GLSL compilation is a first-class supported v0.4 capability for external user and tutorial shaders. RC3 must unify native build-time compilation on `glslc`, keep SPIR-V as the runtime contract, package the lazy shaderc provider in official artifacts, and remove normal-build dependence on `glslangValidator`.

## Validation Defaults

Documentation-only work requires `git diff --check`, the relevant documentation/status checks, and an inspected `git status --short`.

Scene, DRP2, or runtime code normally requires the narrow focused loop followed by `just build`, the relevant `just test` scope, and `just spec-check`; add bounded Vulkan, GLFW, offscreen, or browser smoke when the changed ownership or presentation path requires it.
