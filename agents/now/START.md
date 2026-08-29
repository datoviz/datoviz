# Datoviz v0.4 Dispatch

Status: active post-RC2 work toward RC3, then RC4 and final v0.4.0. Updated: 2026-08-30.

Use [../../AGENTS.md](../../AGENTS.md) as the mandatory entry point. This file identifies only the current route; durable contracts belong in `spec/`, public guidance belongs in `docs/`, and completed evidence belongs in release records and Git history.

## Current Position

Datoviz `v0.4.0rc2` is published and closed. The active source branch and GitHub default are now `main`; the next release milestone is RC3.

The RC3 tutorial-enabling API, rewritten course chapters 1-3 with generated previews, documentation inventory, gallery tooling, canonical screenshot promotion, local Qt bridge proof, required scene-owned lighting foundation, and branch cutover are complete. RC3 remains blocked on exact package proof, maintainer documentation/media decisions, final release-quality gates, and publication of compatible Qt/PyQt provider packages.

The active runtime path is:

```text
scene frame plans -> drp2 command streams -> vklite runtime -> canvas/stream frame execution -> optional app presentation
```

## Start Work

1. Read [STATUS.md](STATUS.md) for current blockers and decisions.
2. Read [RELEASE.md](RELEASE.md) for the remaining RC3, RC4, and final gates.
3. Use [BRANCH_CUTOVER.md](BRANCH_CUTOVER.md) for the executed `main`/`v0.3-maintenance` transition and its final verification evidence.
4. Use [VKLITE_GRAPHICS_TUTORIAL.md](VKLITE_GRAPHICS_TUTORIAL.md) for the rewritten course execution queue and [../../spec/docs/VKLITE_GRAPHICS_TUTORIAL.md](../../spec/docs/VKLITE_GRAPHICS_TUTORIAL.md) for its durable contract.
5. Read [DOCUMENTATION.md](DOCUMENTATION.md) before public documentation, generated-reference, gallery, attribution, or release-communication work.
6. Use [C_DISTRIBUTION.md](C_DISTRIBUTION.md) and [DISTRIBUTION_RELEASE_CHECKLIST.md](DISTRIBUTION_RELEASE_CHECKLIST.md) for C/C++ packaging and exact-artifact work.
7. Use [QT_MACOS_VULKAN_HANDOFF.md](QT_MACOS_VULKAN_HANDOFF.md) for the externally blocked Qt/PyQt provider sequence.
8. Use [HANDOFF_VISUAL_DOCUMENTATION_PASS.md](HANDOFF_VISUAL_DOCUMENTATION_PASS.md) for the implemented visual pilot; obtain maintainer review before broad rollout.
9. Use [HANDOFF_GPU_SELECTION.md](HANDOFF_GPU_SELECTION.md) for the implemented GPU-selection contract and remaining physical Windows matrix.
10. Use [HANDOFF_WINDOWS_VALIDATION.md](HANDOFF_WINDOWS_VALIDATION.md) for the physical Windows machine baseline, remaining AMD/NVIDIA and Qt/vcpkg work, and exact-candidate proof boundary.
11. Use [../../spec/testing/INTERACTION_LATENCY.md](../../spec/testing/INTERACTION_LATENCY.md) for interaction pacing, refresh-aware FIFO latest-ready policy, the one-slot FIFO fallback, and the accepted benchmark contract.
12. Use [QA_SOURCE_AUDIT.md](QA_SOURCE_AUDIT.md) for the completed exploratory source-audit evidence at validated implementation head `545c99379`, now integrated into the active branch.
13. Use [GRAPH_TECHNIQUES.md](../../spec/scene/implementation/GRAPH_TECHNIQUES.md), [OCCLUSION_EFFECTS.md](../../spec/scene/implementation/OCCLUSION_EFFECTS.md), and [TRANSPARENCY_MSAA.md](../../spec/scene/implementation/TRANSPARENCY_MSAA.md) as authority for the implemented render-product architecture; the [promoted proposal](../../spec/scene/proposals/promoted/RENDER_PRODUCTS_AND_TECHNIQUE_COMPOSITION.md) is historical rationale.
14. Use [RC3_RENDER_PRODUCTS_LANDING.md](RC3_RENDER_PRODUCTS_LANDING.md) and [RC3_RENDER_PRODUCTS_AFFECTED_QA.md](RC3_RENDER_PRODUCTS_AFFECTED_QA.md) for final render integration and affected QA. The removed R0-R9 orchestration handoffs remain available in Git history; current semantic authority lives in the specialized specifications.
15. Use [../../docs/tasks/2026-08-02-windows-cuda-vulkan-interop/STATUS.md](../../docs/tasks/2026-08-02-windows-cuda-vulkan-interop/STATUS.md) and [NEXT_STEPS.md](../../docs/tasks/2026-08-02-windows-cuda-vulkan-interop/NEXT_STEPS.md) for the planned non-blocking RC4 Windows/NVIDIA CUDA/Vulkan interop lane.
16. Read [../../spec/scene/README.md](../../spec/scene/README.md), [../../spec/drp2/README.md](../../spec/drp2/README.md), or the binding policies before changing those boundaries.
17. Use [ISSUES_139_140_HANDOFF.md](ISSUES_139_140_HANDOFF.md) for the approved physical-key/text-input API split and coherent mesh-geometry replacement work.
18. Use [ISSUE_138_PERFORMANCE_HANDOFF.md](ISSUE_138_PERFORMANCE_HANDOFF.md) for the benchmark-first rolling-field/structured-surface performance lane and its strict RC4 versus post-v0.4 boundary.
19. The required [RC3_LIGHTING_FOUNDATION_SLICE.md](../../spec/scene/slices/RC3_LIGHTING_FOUNDATION_SLICE.md) is complete at validated implementation head `8fd98715e`. Preserve its scene-owned panel-light and direct/indirect contracts during candidate validation; do not expand it into full PBR or the optional multi-light showcase.
20. Use [GUI_IMPLOT_DOCKING_HANDOFF.md](GUI_IMPLOT_DOCKING_HANDOFF.md) for the non-blocking pre-RC3 official ImPlot/cimplot and declarative docking lane after PR #145 merges; finish it before candidate freeze or defer it intact without delaying RC3.
21. Use [../../spec/data/ASSET_ARCHITECTURE.md](../../spec/data/ASSET_ARCHITECTURE.md) for the approved no-LFS asset catalog and active-submodule retirement plan. Preserve `datoviz/data:v0.4-dev` as the protected frozen v0.4 line during migration; do not rename, merge, or repoint it to the unrelated historical `data:main`.

## Guardrails

- Do not rewrite RC history or infer unavailable physical validation as a pass.
- Keep the runtime path unified and ownership explicit.
- Treat chapters as executable API and documentation quality gates.
- Prefer release blockers and exact proof over optional feature expansion.
- Follow the repository prohibition on hard-wrapped Markdown prose.
