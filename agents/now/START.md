# Datoviz v0.4 Dispatch

The active branch is `main`. RC2 is closed; RC3 is the next release milestone, followed by RC4 and final v0.4.0. Release state is recorded in [STATUS.md](STATUS.md), last updated 2026-08-31; this routing cleanup does not revalidate it.

Use [../../AGENTS.md](../../AGENTS.md) for ordinary implementation routing. Read [STATUS.md](STATUS.md) when work affects release scope or readiness and [RELEASE.md](RELEASE.md) for release planning or packaging. Read only the matching lanes below.

## Task Routes

| Lane | Context |
| --- | --- |
| Documentation and media | [Documentation gates](DOCUMENTATION.md), [visual pilot and required rollout review](HANDOFF_VISUAL_DOCUMENTATION_PASS.md) |
| Vulkan course | [Execution queue](VKLITE_GRAPHICS_TUTORIAL.md), [durable contract](../../spec/docs/VKLITE_GRAPHICS_TUTORIAL.md) |
| Packaging | [C/C++ distribution](C_DISTRIBUTION.md), [exact-artifact checklist](DISTRIBUTION_RELEASE_CHECKLIST.md) |
| Qt/PyQt | [Provider sequence and local proof](QT_MACOS_VULKAN_HANDOFF.md); official provider artifacts are deferred to RC4 |
| Physical platforms | [GPU selection](HANDOFF_GPU_SELECTION.md), [Windows baseline and remaining matrix](HANDOFF_WINDOWS_VALIDATION.md) |
| Frame pacing | [Interaction latency contract and benchmarks](../../spec/testing/INTERACTION_LATENCY.md) |
| Render products | [Graph techniques](../../spec/scene/implementation/GRAPH_TECHNIQUES.md), [occlusion](../../spec/scene/implementation/OCCLUSION_EFFECTS.md), [transparency/MSAA](../../spec/scene/implementation/TRANSPARENCY_MSAA.md); [landing evidence](RC3_RENDER_PRODUCTS_LANDING.md) and [affected QA](RC3_RENDER_PRODUCTS_AFFECTED_QA.md) |
| Input and mesh updates | [Integrated issues #139/#140](ISSUES_139_140_HANDOFF.md) |
| Rolling fields and surfaces | [Issue #138 benchmarks and RC4/post-v0.4 boundary](ISSUE_138_PERFORMANCE_HANDOFF.md) |
| Lighting | [Completed RC3 foundation](../../spec/scene/slices/RC3_LIGHTING_FOUNDATION_SLICE.md); preserve panel-light ownership and direct/indirect contracts without expanding to full PBR or the optional multi-light showcase |
| ImPlot and docking | [Deferred integration](GUI_IMPLOT_DOCKING_HANDOFF.md); keep the experimental example default-off and partial integration out of RC3 |
| Windows CUDA/Vulkan interop | [Status](../../docs/tasks/2026-08-02-windows-cuda-vulkan-interop/STATUS.md), [next steps](../../docs/tasks/2026-08-02-windows-cuda-vulkan-interop/NEXT_STEPS.md); non-blocking RC4 work unless explicitly promoted |
| Asset catalog and submodule migration | [Asset architecture](../../spec/data/ASSET_ARCHITECTURE.md); preserve protected `datoviz/data:v0.4-dev`, never repoint it to unrelated historical `data:main` |

## Release Evidence

For candidate validation, use [source-audit evidence](QA_SOURCE_AUDIT.md), [differential-QA evidence](QA_DIFFERENTIAL_CAMPAIGN.md), and the lane-specific records above. Evidence applies to its recorded commit and environment; unavailable physical or browser validation is not a pass.

For branch-policy work, read the completed [branch cutover](BRANCH_CUTOVER.md). Do not replay the cutover or rewrite RC history.

Release work prioritizes blockers and exact-candidate proof. Completed local checks do not replace artifact, hosted-platform, physical-platform, or maintainer-review gates in `STATUS.md` and `RELEASE.md`.
