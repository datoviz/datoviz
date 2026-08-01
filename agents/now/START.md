# Datoviz v0.4 Dispatch

Status: active post-RC2 work toward RC3, then RC4 and final v0.4.0. Updated: 2026-08-01.

Use [../../AGENTS.md](../../AGENTS.md) as the mandatory entry point. This file identifies only the current route; durable contracts belong in `spec/`, public guidance belongs in `docs/`, and completed evidence belongs in release records and Git history.

## Current Position

Datoviz `v0.4.0rc2` is published and closed. The active source branch and GitHub default remain `v0.4-dev`; the next release milestone is RC3.

The RC3 tutorial-enabling API, rewritten course chapters 1-3 with generated previews, documentation inventory, gallery tooling, canonical screenshot promotion, and local Qt bridge proof are implemented. RC3 remains blocked on branch cutover, exact package proof, maintainer documentation/media decisions, final release-quality gates, and publication of compatible Qt/PyQt provider packages.

The active runtime path is:

```text
scene frame plans -> drp2 command streams -> vklite runtime -> canvas/stream frame execution -> optional app presentation
```

## Start Work

1. Read [STATUS.md](STATUS.md) for current blockers and decisions.
2. Read [RELEASE.md](RELEASE.md) for the remaining RC3, RC4, and final gates.
3. Use [BRANCH_CUTOVER.md](BRANCH_CUTOVER.md) for the pending `main`/`v0.3-maintenance` transition; external branch operations require explicit approval of the exact actions.
4. Use [VKLITE_GRAPHICS_TUTORIAL.md](VKLITE_GRAPHICS_TUTORIAL.md) for the rewritten course execution queue and [../../spec/docs/VKLITE_GRAPHICS_TUTORIAL.md](../../spec/docs/VKLITE_GRAPHICS_TUTORIAL.md) for its durable contract.
5. Read [DOCUMENTATION.md](DOCUMENTATION.md) before public documentation, generated-reference, gallery, attribution, or release-communication work.
6. Use [C_DISTRIBUTION.md](C_DISTRIBUTION.md) and [DISTRIBUTION_RELEASE_CHECKLIST.md](DISTRIBUTION_RELEASE_CHECKLIST.md) for C/C++ packaging and exact-artifact work.
7. Use [QT_MACOS_VULKAN_HANDOFF.md](QT_MACOS_VULKAN_HANDOFF.md) for the externally blocked Qt/PyQt provider sequence.
8. Use [HANDOFF_VISUAL_DOCUMENTATION_PASS.md](HANDOFF_VISUAL_DOCUMENTATION_PASS.md) for the implemented visual pilot; obtain maintainer review before broad rollout.
9. Use [HANDOFF_GPU_SELECTION.md](HANDOFF_GPU_SELECTION.md) for the implemented GPU-selection contract and remaining physical Windows matrix.
10. Use [HANDOFF_FRAME_DEMAND.md](HANDOFF_FRAME_DEMAND.md) for the approved interaction-pacing and extensible frame-demand implementation plan.
11. Read [../../spec/scene/README.md](../../spec/scene/README.md), [../../spec/drp2/README.md](../../spec/drp2/README.md), or the binding policies before changing those boundaries.

## Guardrails

- Do not rewrite RC history or infer unavailable physical validation as a pass.
- Keep the runtime path unified and ownership explicit.
- Treat chapters as executable API and documentation quality gates.
- Prefer release blockers and exact proof over optional feature expansion.
- Follow the repository prohibition on hard-wrapped Markdown prose.
