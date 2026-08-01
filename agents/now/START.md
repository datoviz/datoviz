# Datoviz v0.4 Dispatch

Status: active post-RC2 work toward RC3, then RC4 and final v0.4.0. Updated: 2026-08-01.

Use [../../AGENTS.md](../../AGENTS.md) as the mandatory entry point. This file identifies only the current work; durable contracts belong in `spec/`, public guidance belongs in `docs/`, and completed evidence belongs in release records and Git history.

## Current Position

Datoviz `v0.4.0rc2` is published and closed. The active source branch and GitHub default remain `v0.4-dev` until the approved post-RC2 branch cutover is executed. The next release milestone is RC3.

The conda-first Qt bridge implementation and local macOS ARM64 package proof are complete, but the required RC3 provider lane is externally blocked on maintainer merge and publication of the Vulkan-enabled Qt package, followed by a green PyQt rebuild and published compatible package. See [QT_MACOS_VULKAN_HANDOFF.md](QT_MACOS_VULKAN_HANDOFF.md).

The active runtime path is:

```text
scene frame plans -> drp2 command streams -> vklite runtime -> canvas/stream frame execution -> optional app presentation
```

## Start Work

1. Read [STATUS.md](STATUS.md) for the ordered post-RC2 queue and current gates.
2. Read [RELEASE.md](RELEASE.md) for RC3, RC4, and final-release exit criteria.
3. Use [BRANCH_CUTOVER.md](BRANCH_CUTOVER.md) for the pending `main`/`v0.3-maintenance` transition; do not execute external branch operations without explicit approval of the exact actions.
4. Use [GALLERY_MEDIA_SINGLE_RESOLUTION.md](GALLERY_MEDIA_SINGLE_RESOLUTION.md) for the approved gallery resolution, encoding, freshness, and bounded-parallelism plan; implementation still requires its own execution approval.
5. Read [DOCUMENTATION.md](DOCUMENTATION.md) before public documentation, generated-reference, gallery, attribution, or release-communication work.
6. Use [C_DISTRIBUTION.md](C_DISTRIBUTION.md) and [DISTRIBUTION_RELEASE_CHECKLIST.md](DISTRIBUTION_RELEASE_CHECKLIST.md) for C/C++ packaging and distribution work.
7. Use [QT_MACOS_VULKAN_HANDOFF.md](QT_MACOS_VULKAN_HANDOFF.md) for the active upstream Qt/PyQt publication dependency, verified local artifacts, CI interpretation, and remaining provider-package sequence.
8. Use [HANDOFF_VISUAL_DOCUMENTATION_PASS.md](HANDOFF_VISUAL_DOCUMENTATION_PASS.md) only for the approved visual-documentation pilot; request maintainer review after the pilot before broad rollout.
9. Use [VKLITE_GRAPHICS_TUTORIAL.md](VKLITE_GRAPHICS_TUTORIAL.md) for the required RC3 tutorial-facing API and three-chapter pilot, RC4 course completion, installed-consumer proof, asset work, and final tutorial freeze; shader work must also follow the first-class runtime GLSL and unified `glslc` contract in [../../spec/architecture/SHADER_TOOLCHAIN.md](../../spec/architecture/SHADER_TOOLCHAIN.md).
10. Read [../../spec/scene/README.md](../../spec/scene/README.md) before changing scene semantics or runtime boundaries, [../../spec/drp2/README.md](../../spec/drp2/README.md) before changing DRP2, and [../../spec/bindings/ARRAY_FACADE.md](../../spec/bindings/ARRAY_FACADE.md) plus [../../spec/bindings/CTYPES_POLICY.md](../../spec/bindings/CTYPES_POLICY.md) before changing bindings.
11. Use [HANDOFF_GPU_SELECTION.md](HANDOFF_GPU_SELECTION.md) for the implemented native test-runner GPU selection contract, Linux multi-GPU evidence, explicit exemption policy, and remaining physical Windows validation matrix.

## Guardrails

- Do not rewrite v0.4 RC or final-release history; repository-history cleanup is deferred beyond v0.4.
- Keep RC1 and RC2 tags, artifacts, reports, checksums, physical evidence, and release records immutable.
- Do not treat unavailable physical Linux or Windows validation as a pass.
- Keep the runtime path unified; do not create parallel renderers, presentation layers, frame streams, or Vulkan wrappers.
- Treat tutorial chapter spikes as API quality gates: improve existing general subsystem boundaries, preserve explicit ownership, and do not add tutorial-only runtime abstractions.
- Prefer focused blocker fixes and release-proof improvements over optional feature expansion.
- Follow the repository prohibition on hard-wrapped Markdown prose.
