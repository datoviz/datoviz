# Datoviz v0.4 Status

Status: active post-RC2 work toward RC3, then RC4 and final v0.4.0. Updated: 2026-08-03.

Keep this file current and short. Durable behavior belongs in `spec/`; completed campaign detail belongs in Git history, release assets, and tagged documentation.

## Current Position

RC2 is published and closed at tag `v0.4.0rc2`, release commit `8a3bd7509`. Its six canonical wheels, hosted conformance, package-index verification, physical MacBook M3 intake, documentation, artifacts, and exclusions are immutable release evidence.

The GitHub default and active development branch remain `v0.4-dev`; old v0.3 remains at `main`. The branch cutover has not been executed.

The reusable tutorial API, typed shader compiler, shared `glslc` path, Canvas depth, OBJ UVs, explicit image upload, push constants, direct camera/arcball path, and rewritten course chapters 1-3 are implemented. The deleted pilot and its Suzanne/binary-asset plan are not current release work.

The documentation inventory, generated C reference, Python binding guidance, dataset attribution, known-limitations audit, four-page visual pilot, gallery-media tooling, regenerated animation candidates, and approved canonical Linux screenshots are implemented. Publication and maintainer-review gates remain separate.

The optional Qt bridge and local Apple Silicon split-package proof are complete. The required RC3 provider lane remains externally blocked on conda-forge publication of Vulkan-enabled Qt followed by compatible PyQt and exact Datoviz provider artifacts.

Interaction pacing, presentation policy, and scheduler admission are implemented. Continuous interaction exposed stale-frame latency in ordinary FIFO rather than a scene-throughput defect. App-owned native windows prefer refresh-paced FIFO latest-ready, then mailbox, then a one-slot ordinary-FIFO fallback; every requested native frame passes through per-view admission, while explicit present-mode, FPS-cap, frame-slot, immediate-throughput, direct-render, and hosted-surface paths retain their intended behavior. Physical Linux/X11 acceptance found the production default, explicit uncapped immediate, and explicit ordinary FIFO with one slot smooth, with paced interaction reaching approximately 60 FPS and returning to zero idle rendering; physical Windows frame-slot throughput measurements remain neutral.

The issue #137 render-product lane is implemented through R9: typed panel-local products, coherent surface/MSAA semantics, product-driven EDL/transparency/volume composition, deterministic material-aware GTAO, the semantic AO public API, legacy-path cleanup, and authoritative specifications are present. Its checkpoint commits, exact-head validation, landing evidence, and affected-QA manifest are committed and pushed on `refactor/rc3-render-products` through `19ea96758`.

The approved limitation follow-up refreshed the AO gallery reference, added the prepared US-state choropleth bundle, and made the complete GLFW/swapchain lane pass under Xvfb. The full WebGPU scenario is green after asserting three semantic panel-local render passes plus explicit presentation and updating the browser AO expectation; the validated follow-up is committed and pushed through integration head `a6203eba1`.

The scheduler-pacing chain and completed exploratory source-audit fixes are integrated locally onto `refactor/rc3-render-products` through head `545c99379`. The exact local head builds and passes 1,128/1,128 validation-enabled native tests, 95/95 DRP2 contract tests, 125/125 fixtures, 100/100 runtime-vklite, 34/34 slow/recovery cases, all seven bounded presentation paths, specifications, WebGPU, bindings, example manifests, generated docs/snippets/build, and the Vulkan course smoke. Full-tree static analysis is dispositioned; CPU sanitizer coverage is green for the new non-driver fixes, while Vulkan-backed sanitizer teardown remains inconclusive. This local convergence has not been pushed.

## Remaining RC3 Gates

| Lane | Current state | Remaining gate |
| --- | --- | --- |
| Branch cutover | GitHub still defaults to `v0.4-dev`; old v0.3 remains `main`. | Approve and execute [BRANCH_CUTOVER.md](BRANCH_CUTOVER.md), then verify refs, protections, automation, links, and fresh clones. |
| Rewritten course | Chapters 1-3, canonical programs, generated previews, source synchronization, deterministic source-install smoke, and enabling API are implemented. RC2 cannot compile the course because it predates the new API. | Verify the key input path needed by chapter 5; pass the course against the first official package newer than RC2; obtain supported hosted proof and record live-resize behavior. |
| Documentation and gallery | Generated reference, Python guidance, attribution, known limitations, gallery tooling, regenerated candidates, canonical screenshots, and the visual pilot are complete. | Review the visual pilot and rewritten course voice; approve exact animation/card publication if desired; decide PR #132 successors; draft RC3 notes and evidence after artifact scope freezes. |
| Qt/PyQt provider | Local Qt 6.11.1 build 2, PyQt6 6.11.0 build 3, split Datoviz packages, Vulkan instance, Cocoa surface, and hosted rendering proof are green. | Merge and publish Qt build 2, rerun and publish compatible PyQt, then build and validate exact split Datoviz artifacts on supported hosted platforms. |
| Distribution | RC2 wheel and package-index campaigns are complete; reusable source, wheel, conda, and vcpkg tooling exists. The checkout-backed Windows `x64-windows` overlay and standalone Debug/Release CMake consumers pass on the physical MSVC machine. | Validate the exact final RC3 source/wheel/provider artifacts, release-source vcpkg URL and SHA512, conda layouts, third-party notices, and checksum/signing decisions. |
| Release quality | The integrated render, pacing, and exploratory source-audit head passes the locally available static-analysis, practical CPU sanitizer, validation-enabled Vulkan/native, recovery, bounded presentation, docs, example-manifest, course, binding, specification, and WebGPU gates. | Freeze the exact RC3 candidate and run immutable source-archive, wheel, installed-consumer, hosted Linux/Windows, provider, remaining gallery/media, and physical-platform gates; record unavailable hardware and Vulkan-sanitizer limitations explicitly. |

Hosted Linux and Windows exact-artifact proof is mandatory for RC3. Physical Linux and Windows proof should be restored when hardware is available; final v0.4.0 requires that proof or an explicit maintainer-approved exception.

## RC4 Gate

RC4 completes rewritten course chapters 4-15 through an interactive textured and lit generated mesh, generates every chapter preview, freezes the tutorial-facing API profile, and proves every chapter from exact installed packages on supported hosted platforms. Generated geometry and a procedural texture are the required path; Suzanne and committed binary tutorial assets are optional polish, not blockers.

The Windows/NVIDIA CUDA/Vulkan external-memory port is a planned non-blocking RC4 engineering lane with a durable task record in [../../docs/tasks/2026-08-02-windows-cuda-vulkan-interop/STATUS.md](../../docs/tasks/2026-08-02-windows-cuda-vulkan-interop/STATUS.md). It does not become an RC4 release gate unless the maintainer explicitly promotes it.

## Final Gate

Final v0.4.0 resolves or records RC4 feedback, regenerates final media, passes reproducible artifact and documentation gates, records physical-validation proof or exceptions, completes Zenodo/citation metadata, submits or defers JOSS, publishes the release, and resets the active queue.

## Optional Candidates

- GSP Texture2D mesh integration and scene-owned multi-light support remain optional unless explicitly promoted.
- Hosted documentation preview, wind globe, prompt widget, Pyodide playground, hero composition, and broad visual polish must not delay required release gates.
- Point-cloud public WebGPU redistribution remains delisted without redistribution permission.

## Validation Defaults

Documentation-only work requires `git diff --check`, relevant documentation/status checks, and inspection of `git status --short`. Runtime work normally requires the focused loop followed by `just build`, relevant tests, and `just spec-check`, with Vulkan, GLFW, offscreen, or browser smoke where ownership or presentation changes require it.
