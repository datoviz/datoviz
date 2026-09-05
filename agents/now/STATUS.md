# Datoviz v0.4 Status

Status: active post-RC2 work toward RC3, then RC4 and final v0.4.0. Release evidence updated: 2026-08-31. Condensed: 2026-09-05; release and remote state were not revalidated.

Keep this file current and short. Durable behavior belongs in `spec/`; completed campaign detail belongs in Git history, release assets, and tagged documentation.

## Current Position

RC2 is closed at `v0.4.0rc2`, release commit `8a3bd7509`; its artifacts and exclusions remain immutable. Active development is `main`; protected `v0.3-maintenance` preserves v0.3 and `dev` remains frozen. [Branch cutover evidence](BRANCH_CUTOVER.md) records validation head `d3d7142f0`.

RC3 implementation is integrated. Remaining work is review, candidate freeze, and exact artifact/platform proof; completed local validation is not release approval. Use the lane records below rather than replaying completed campaigns.

| Completed lane | Evidence and boundary |
| --- | --- |
| Render products and lighting | Render head `545c99379`, merge `2d83d0b63`: [landing](RC3_RENDER_PRODUCTS_LANDING.md) and [affected QA](RC3_RENDER_PRODUCTS_AFFECTED_QA.md). Lighting head `8fd98715e`: [foundation slice](../../spec/scene/slices/RC3_LIGHTING_FOUNDATION_SLICE.md). |
| Input, mesh replacement, and surfaces | [Issues #139/#140](ISSUES_139_140_HANDOFF.md) and [issue #138](ISSUE_138_PERFORMANCE_HANDOFF.md) retain implementation and benchmark evidence; sampled fields remain optional RC4 work and GPU-displaced surfaces remain post-v0.4. |
| Interaction pacing | [Contract and benchmarks](../../spec/testing/INTERACTION_LATENCY.md): physical Linux/X11 acceptance was smooth, approximately 60 FPS while interacting and zero idle rendering; Windows frame-slot throughput measurements remain neutral. |
| Local readiness, gallery, packaging, and no-data builds | [Pre-freeze release evidence](RELEASE.md#pre-freeze-local-evidence-recorded-2026-08-31) retains heads `846fd3049`, `b6e330282`, and `f716786a3`, test counts, skips, and artifact limitations. The Debug `manylinux_2_38` diagnostic wheel is not an RC3 Release artifact. |
| Differential correctness and robustness | Published head `42760e096`: [campaign](QA_DIFFERENTIAL_CAMPAIGN.md) and [release evidence](RELEASE.md). Native validation passed 1,135/1,173 with 38 expected no-display skips and zero failures; focused CPU sanitizer/Valgrind checks passed, broader Vulkan sanitizer/GPU Valgrind proof remains limited. Coarse DRP2 error codes and exhaustive allocation-failure injection are non-blocking follow-ups. |
| Qt bridge | [Provider handoff](QT_MACOS_VULKAN_HANDOFF.md): local split-package proof and Linux/Xvfb source smoke at `50008e9ff` passed. System PyQt lacks `QVulkanInstance`; official managed provider proof is deferred to RC4. |

The protected data branch remains `datoviz/data:v0.4-dev`, recovered baseline `3c862beb2e9885f5af9dc624dc22a6d5bb856026`, transitional tip `b94d32d9c0a0a4c47e7e5c393b4ccc570159ed96`; parent `996825aec` published the approved public WebGPU route. Preserve unrelated historical `data:main`. Follow [reachability and commit rules](../rules/REPO_HYGIENE.md) and the [asset migration contract](../../spec/data/ASSET_ARCHITECTURE.md); migration is not current RC3 work.

Point Cloud headed interaction and public-site confirmation remain pending: headless/Xvfb reaches `QueueSubmit` but encounters external WebGPU instance loss. Grantor, date, scope, and durable permission-reference details also remain pending; [release evidence](RELEASE.md#pre-freeze-local-evidence-recorded-2026-08-31) retains the completed bundle and fallback checks.

## Remaining RC3 Gates

| Lane | Current state | Remaining gate |
| --- | --- | --- |
| Rewritten course | Chapters 1-3, canonical programs, generated previews, source synchronization, deterministic source-install smoke, enabling API, and the chapter-5 explicit-reload input path are implemented and locally proven. RC2 cannot compile the course because it predates the new API. | Pass the course against the first official package newer than RC2; obtain supported hosted proof and record live-resize behavior. |
| Documentation and gallery | Generated reference, Python guidance, attribution, known limitations, gallery tooling, canonical screenshots, and the visual pilot are complete. The release guidance now separates RC3 from RC4, all 105 reviewed stills pass full pixel validation, the nine invalidated still-cache records verify against canonical images, all 38 animation caches are current, all 29 video cards and posters fit budget, and experimental exclusions are explicit. Focused successor PR #136 is open for the residual PR #132 work. | Review the visual pilot, rewritten course voice, and exact animation/card candidates; approve publication if desired; await the original author's feedback on PR #136; draft RC3 notes and evidence after artifact scope freezes. |
| Distribution | RC2 wheel and package-index campaigns are complete; reusable source, wheel, conda, and vcpkg tooling exists. The checkout-backed Windows `x64-windows` overlay and standalone Debug/Release CMake consumers pass on the physical MSVC machine. Current local source packaging, licenses, installed consumers, and isolated diagnostic-wheel payload checks pass. | Validate the exact final RC3 Release source and six-wheel artifacts in supported builders, release-source vcpkg URL and SHA512, base conda layouts, third-party notices, and checksum/signing decisions. |
| Release quality | The integrated render, pacing, exploratory source-audit, and differential-QA heads pass the locally available static-analysis, practical CPU sanitizer, validation-enabled Vulkan/native, recovery, bounded presentation, docs, example-manifest, course, binding, specification, WebGPU build/data/stream, no-data, bounded example-review, input, Qt-hosting, and full gallery-pipeline gates. Headless browser routes reach `QueueSubmit` but retain the known external WebGPU instance-loss skip. ImPlot was explicitly probed and remains unavailable because its intentionally deferred source dependency is absent. Publication staging excludes local-only WebGPU data. | Complete subjective documentation/media review and the multi-machine batch plan; freeze the exact RC3 candidate; then run immutable Release source/wheel, installed-consumer, hosted Linux/Windows, headed-browser, and physical-platform gates with explicit limitations. |
| Lighting foundation | Complete at validated implementation head `8fd98715e`: scene-owned ambient/directional lights, panel-local ordered sets, one shared panel payload, material/light ownership separation, explicit direct/indirect shader composition, and native/WGSL validation are integrated. | Carry the completed slice into the exact RC3 candidate and rerun its focused native and browser checks on supported hosted environments. |

Hosted Linux and Windows exact-artifact proof is mandatory for RC3. Physical Linux and Windows proof should be restored when hardware is available; final v0.4.0 requires that proof or an explicit maintainer-approved exception.

The hosted Windows ARM64 smoke uses native Python 3.11-3.14 because the setup-python provider's current Windows ARM64 manifest has no CPython 3.10 package. AMD64 retains Python 3.10-3.14 coverage; this is a provider-availability exclusion, not a Datoviz compatibility claim.

## RC4 Gate

RC4 completes rewritten course chapters 4-15 through an interactive textured and lit generated mesh, generates every chapter preview, freezes the tutorial-facing API profile, and proves every chapter from exact installed packages on supported hosted platforms. Generated geometry and a procedural texture are the required path; Suzanne and committed binary tutorial assets are optional polish, not blockers.

RC4 also owns the official conda Qt/PyQt provider: monitor PyQt PR #186 through publication, build the exact split `libdatoviz`, `datoviz`, and `datoviz-qtbridge` artifacts against the managed runtime, and complete clean-prefix hosted provider validation. Upstream publication remains a dependency of this provider lane, not of the base RC3 release.

The Windows/NVIDIA CUDA/Vulkan external-memory port is a planned non-blocking RC4 engineering lane with a durable task record in [../../docs/tasks/2026-08-02-windows-cuda-vulkan-interop/STATUS.md](../../docs/tasks/2026-08-02-windows-cuda-vulkan-interop/STATUS.md). It does not become an RC4 release gate unless the maintainer explicitly promotes it.

## Final Gate

Final v0.4.0 resolves or records RC4 feedback, regenerates final media, passes reproducible artifact and documentation gates, records physical-validation proof or exceptions, completes Zenodo/citation metadata, submits or defers JOSS, publishes the release, and resets the active queue.

## Optional Candidates

- Official default-on ImPlot/cimplot integration and the declarative docking refactor are deferred intact until after RC3 under [GUI_IMPLOT_DOCKING_HANDOFF.md](GUI_IMPLOT_DOCKING_HANDOFF.md). The experimental example remains default-off and no partial dependency, context, raw surface, or docking implementation enters the RC3 candidate.
- GSP Texture2D mesh integration, point-light evaluation, and the full multi-light Klein-bottle slice remain optional. The required narrower scene-owned ambient/directional lighting foundation is complete.
- Hosted documentation preview, wind globe, prompt widget, Pyodide playground, hero composition, and broad visual polish must not delay required release gates.

## Validation Defaults

Documentation-only work requires `git diff --check`, relevant documentation/status checks, and inspection of `git status --short`. Runtime work normally requires the focused loop followed by `just build`, relevant tests, and `just spec-check`, with Vulkan, GLFW, offscreen, or browser smoke where ownership or presentation changes require it.
