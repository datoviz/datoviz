# Datoviz v0.4 Release Plan

Status: active roadmap from closed RC2 through RC3 and RC4 to final `v0.4.0`, with RC5 contingency for scientific-profile acceptance. Scientific-profile scope added: 2026-10-13. Platform availability and PyQt intake updated: 2026-10-10; earlier candidate preparation retains its recorded evidence; release artifacts are not frozen.

Use [STATUS.md](STATUS.md) for current blockers, [DOCUMENTATION.md](DOCUMENTATION.md) for documentation gates, [DISTRIBUTION_RELEASE_CHECKLIST.md](DISTRIBUTION_RELEASE_CHECKLIST.md) for packaging proof, and [../../spec/release/](../../spec/release/) for durable release policy.

## Scope Boundary

Datoviz v0.4 owns the C engine, native scene/app runtime, generated low-level Python binding with NumPy adaptation, raster capture, an experimental WebGPU/WASM subset, a narrow experimental compute-to-render path, reproducible packaging, and the release-pinned Modern GPU Graphics in Vulkan course.

GSP/VisPy2 owns high-level object-oriented plotting and publication-oriented vector export. v0.4 does not require v0.3 compatibility, full native/WebGPU parity, a general compute framework, broad CUDA/CuPy interop, complex text shaping, dashboards, or long-horizon backend refactors.

## Completed Milestones

RC1 established the public v0.4 candidate surface and exposed a packaged macOS native-window loader defect. RC2 is published and closed at tag `v0.4.0rc2`, release commit `8a3bd7509`; it fixes that defect and has complete immutable hosted, package-index, artifact, documentation, and physical MacBook M3 evidence with physical Linux and Windows recorded as unavailable.

Do not replay RC1 or RC2 execution instructions. Audit their tagged releases, assets, reports, and workflow evidence.

## 1. Post-RC2 Branch Cutover

The cutover is complete: old v0.3 `main` is preserved as protected `v0.3-maintenance`, the former `v0.4-dev` is the protected default `main`, both open PR bases moved to `main`, fresh clones pass, exact validation head `d3d7142f0` is green, and no histories, tags, release refs, or data state changed. Evidence is recorded in [BRANCH_CUTOVER.md](BRANCH_CUTOVER.md).

## 2. RC3 Documentation, Packaging, Quality, And Course Foundation

Completed RC3 implementation includes gallery-media policy and tooling, generated C/Python documentation inventories, dataset attribution and provenance, known limitations, visual-system pilot, canonical Linux screenshots, tutorial-facing API, unified shader toolchain, the course foundation and the subsequent complete 16-chapter implementation, local installed-consumer proof, Qt bridge implementation, local Apple Silicon split-package proof, the R1-R9 render-product/GTAO/public-AO implementation, and the required scene-owned lighting foundation. The lighting slice is validated at implementation head `8fd98715e`; earlier integrated render and source-audit evidence remains recorded at its own exact heads.

These completed lanes remain valid historical evidence, but they do not close the newly identified pre-v0.4 [native scientific 3D profile](../../spec/scene/slices/V04_SCIENTIFIC_3D_PROFILE.md). RC3 scope for that profile is limited to capability, validation, documentation, and correctness fixes on supported paths; the [profile handoff](SCIENTIFIC_3D_PROFILE_HANDOFF.md) tracks its gaps and acceptance evidence. Preserve bounded existing volume behavior while advanced volume work remains deferrable.

The earlier validated implementation head passes the complete 1,128-case native matrix, affected DRP2/vklite/recovery/presentation gates, practical CPU sanitizer checks, full-tree static-analysis disposition, WebGPU, bindings, documentation, example-manifest, and course checks. The later differential correctness and robustness campaign is integrated and published at `42760e096`; after high-effort architecture review and corrective follow-up, its final post-rebase validation passed the full build, 1,135/1,173 native tests with zero failures and 38 expected no-display skips, 47/47 Xvfb canvas tests, 158/158 DRP2 tests, 137/137 fixtures, WebGPU smoke with 42 fixtures plus two streams and 89 negative cases, bindings and ABI policy, and installed-package and FetchContent consumers. Vulkan-backed sanitizer teardown and provider-owned GPU Valgrind diagnostics remain inconclusive, and exact artifact/host/platform proof remains separate.

Current maintainer-feedback work follows [ISSUES_139_140_HANDOFF.md](ISSUES_139_140_HANDOFF.md) and [ISSUE_138_PERFORMANCE_HANDOFF.md](ISSUE_138_PERFORMANCE_HANDOFF.md). The #140 scene-buffer lifecycle and coherent mesh replacement, #139 physical-key/committed-text split, and #138 benchmark-backed bounded corrections are integrated into `main`. The chapter-5 explicit-reload input path is locally proven without a watcher or new API; exact-candidate package and hosted validation remain separate gates. Optional RC4 sampled-field foundations and post-v0.4 GPU displacement retain the boundaries below.

The course expanded on `main` in `30ee8ea3a` and received cumulative-path corrections in `078bcb809`. On 2026-11-16, all 16 step programs pass reproducible in-tree rendering, all 173 C/GLSL excerpts synchronize, and all 18 media outputs generate. This closes implementation work, not the fresh reader audit or exact installed-package/hosted acceptance. The maintainer decided on 2026-11-16 to ship the complete course in RC3; RC4 keeps reader feedback and the tutorial API freeze. The 2026-11-16 Claude review covered a fresh-clone reader run of all 16 chapters (all built and ran with zero validation errors), voice, clarity and correctness, consistency, learner experience and pacing. Must-fix findings landed in `3d2397637`. The maintainer subsequently requested fuller explanations using chapter 12 as the model; the 2026-11-17 explanatory pass covers all chapters, followed by consistency and learner fixes in `159a18228`. Deferred past v0.4: reordering part 4 (upload, sampling, then uniform buffers), moving viewport/scissor from chapter 3 to 4, and a canvas format query before the first frame. The fuller 2026-11-17 explanatory revision received a Claude Opus 5.5 medium continuation on 2026-11-18, with independent source verification and bounded documentation corrections; all 16 unchanged programs pass validation-enabled in-tree smoke. Maintainer proofreading and exact package/platform gates remain. See [the course queue](VKLITE_GRAPHICS_TUTORIAL.md).

The maintainer accepted the [polygon paint contract](../../spec/scene/semantics/POLYGON_PAINT.md) on 2026-11-17 and approved the imminent-RC3 scope reconciliation: ship the implemented unlit ordered rendering, document conservative unsupported paint queries, and allocate composed GPU paint picking plus WebGPU device ownership/loss replacement to [RC4](../../spec/scene/slices/RC4_PAINT_QUERY_DEVICE_LIFECYCLE.md). Do not backport the broader RC4 query composition or start a device-lifecycle refactor before RC3. The bounded browser/provider investigation landed a small proven harness fix: strict ANGLE/Vulkan-profile runs pass both affected routes headless and headed/Xvfb, as recorded in [the follow-up evidence](GALLERY_REFERENCE_REVIEW.md#rc3-browser-validation-and-scope-2026-11-17). Any persistent provider-only failure needs an exact, narrowly scoped browser-validation exclusion; it is not a pass and does not waive native/Python, installed-package, exact-candidate, or media-approval gates. [Implementation and browser evidence](GALLERY_REFERENCE_REVIEW.md#polygon-paint-implementation-2026-11-17) remains tied to its recorded revisions.

Remaining RC3 deliverables:

1. Obtain maintainer acceptance of the rewritten course voice and previews. Reconcile existing visual-review decisions with exact gallery candidates and the separate four-page pilot scope rather than repeating accepted homepage or benign general reviews. The refreshed embedding-portability implementation is merged through #157, and original PR #132 is closed as superseded, so this lane is no longer an RC3 gate.
2. Complete unresolved documentation/media dispositions and the remaining multi-machine review without admitting deferred dependency work; local documentation, WebGPU, input, Qt, gallery-cache, and animation/card pipeline gates are complete.
3. Build and deterministically smoke the complete course and runtime shaderc from the first official candidate packages newer than RC2, with installed CMake consumers on available supported hosted platforms; retain honest live-resize and physical-machine exclusions. RC4 owns full-course cross-platform installed acceptance, interaction/resize coverage, feedback, and API freeze; it does not waive RC3 package or installed-consumer smoke.
4. Validate the final source bundle, six-wheel matrix, installed Python/CMake consumers, Windows vcpkg overlay, base conda layouts, third-party notices, and checksum/signing policy.
5. Carry the completed polygon rendering and configured-provider browser proof into exact-candidate validation, close the separate canonical-media promotion gate, freeze the exact RC3 candidate, carry forward the completed local source-quality and media-pipeline evidence, and run the immutable package/artifact, installed-consumer, hosted-platform, and physical-platform gates with explicit limitations.
6. Freeze RC3 notes, known issues, validation evidence, artifacts, and feedback request only after the exact release scope is fixed.
7. Carry the locally complete scientific-profile RC3 guards, effective-alpha corrections and focused regressions at `d0e904705` into exact-candidate/platform validation without broadening RC3 into the required RC4 composition work.

Hosted Linux and Windows exact-artifact validation is mandatory for RC3. The physical macOS and Windows machines are unavailable as of 2026-10-10; native/provider and physical candidate checks remain pending. Continue independent Linux development checks; unavailable hardware remains an exclusion. Final requires the missing physical proof or an explicit maintainer-approved exception.

RC4 remains the default allocation for the official Qt/PyQt provider. Upstream publication is complete, and any RC3 promotion requires an explicit maintainer decision; Linux/macOS published-runtime proof passes, while Windows runtime and exact-candidate provider proof remain pending in [the latest handoff](QT_MACOS_VULKAN_HANDOFF.md). Preserve the existing experimental scope until the maintainer decides otherwise, and do not make provider promotion a base RC3 blocker. Other deferred work must not enter the RC3 candidate: ImPlot/cimplot integration, declarative docking, GSP Texture2D mesh integration, point-light evaluation, the full multi-light Klein-bottle showcase, a public hosted documentation-preview workflow, wind globe, prompt widget, Pyodide playground, hero composition, and broad visual polish. The existing private managed WIP documentation is already available and is separate from the deferred public preview workflow. The narrower required lighting foundation is complete and must remain covered by exact-candidate validation.

The [2026-10-11 provider scope draft](QT_MACOS_VULKAN_HANDOFF.md#proposed-rc3-provider-scope-2026-10-11) recommends conditional RC3 inclusion of the managed conda provider, with RC4 as the fallback. Promotion requires Windows trust/runtime resolution, proof for each claimed platform, teardown disposition, and exact frozen-candidate validation. Public development installation guidance and the local release-notes draft now reflect the published upstream runtime without claiming released Datoviz provider packages. Candidate freeze and publication remain separate maintainer decisions.

The maintainer is not ready to freeze RC3 before proofreading the course and resolving any resulting minor code fixes. Temporary source bundles, wheels, and conda packages validate the mutable development tree only; keep the existing version metadata and do not select a final candidate.

## 3. RC4 Course And Installed Developer Experience

Required deliverables:

1. Resolve or record RC3 course feedback, and repeat the isolated-reader audit after the RC3 revision pass.
2. Regenerate and validate each chapter preview from exact candidate programs without committed binary prerequisites or `data` submodule changes; development previews already exist for every chapter.
3. Freeze the tutorial-facing API profile and document vklite as an advanced interface that may evolve during the release candidates, with exact release compatibility.
4. Build and run every chapter from exact installed source archives and wheels through `find_package(datoviz CONFIG REQUIRED)`, packaged runtime shaderc, deterministic offscreen proof, and bounded live resize, input, depth, repeated-frame, and shutdown smoke.
5. Pass Vulkan validation, public-header and binding checks, source synchronization, links, navigation, captures, license review, known-issue review, and supported hosted-platform validation, or record explicit limitations.
6. Collect and disposition feedback on setup, progression, diagnostics, GPU/driver behavior, resize, input, concepts, ownership, and cleanup.
7. Complete the official conda Qt/PyQt provider lane against the published compatible runtime, unless explicitly promoted and completed in RC3: build exact split `libdatoviz`, `datoviz`, and `datoviz-qtbridge` artifacts and validate the managed Vulkan loader, MoltenVK, `QVulkanInstance`, native surface, hosted rendering, and missing-provider diagnostics from clean prefixes.
8. Deliver non-volume scientific 3D composition and integration acceptance required by the [scientific 3D profile](../../spec/scene/slices/V04_SCIENTIFIC_3D_PROFILE.md), alongside the course and provider gates above. Broader OIT, WebGPU parity, image-based lighting, and PBR expansion remain later work.

RC4 also owns the focused [paint-query and device-lifecycle slice](../../spec/scene/slices/RC4_PAINT_QUERY_DEVICE_LIFECYCLE.md): reuse the existing scientific query composition foundation for ordered semantic paint queries, and separate runtime resource reset from replacement of a lost owned device. Bars/bands migration is optional follow-up. This allocation does not promise full scientific WebGPU parity.

The required course uses generated Datoviz geometry and a procedural asymmetric texture. Suzanne and committed binary assets are optional polish only after the course is complete.

Planned non-blocking RC4 engineering lane: implement and physically validate the experimental Windows/NVIDIA counterpart of the existing Linux CUDA/Vulkan external-memory path. Follow [../../docs/tasks/2026-08-02-windows-cuda-vulkan-interop/STATUS.md](../../docs/tasks/2026-08-02-windows-cuda-vulkan-interop/STATUS.md) and [NEXT_STEPS.md](../../docs/tasks/2026-08-02-windows-cuda-vulkan-interop/NEXT_STEPS.md). This does not become an RC4 release gate unless the maintainer explicitly promotes it.

Optional non-blocking RC4 sampled-field foundations may complete sampler addressing and bounded multi-region dirty tracking after required course work is secure and before the API freeze. They must not delay RC4, and binding-local logical field views, streaming extensions, and GPU field-displaced structured meshes remain post-v0.4 work under [SAMPLED_FIELD_VIEWS_AND_INCREMENTAL_UPDATES.md](../../spec/scene/proposals/future/SAMPLED_FIELD_VIEWS_AND_INCREMENTAL_UPDATES.md).

## RC5 Contingency

Use RC5 only if substantial scientific-profile gaps or RC4 feedback require another implementation cycle. It is not an automatic milestone; keep accepted profile scope and require explicit capability, integration, and validation evidence for remaining work.

## Prepared GitHub Milestone Drafts

A read-only check on 2026-11-17 found no configured GitHub milestones. The exact milestone titles/descriptions below and the linked issue drafts are local review material, not remote changes. No due dates are proposed; publication requires review of the exact final content and action.

| Title | Description |
| --- | --- |
| `v0.4.0rc3` | Release the implemented documentation, complete Vulkan course, packaging, and polygon rendering corrections. Accept explicit unsupported paint queries and accurately scoped browser limitations; complete exact-artifact validation, maintainer media approval, and final notes without importing deeper RC4 architecture. |
| `v0.4.0rc4` | Deliver course and installed-consumer acceptance, the planned provider/scientific integration, composed GPU paint picking using shared rendered coverage and ordering, and owned WebGPU device-loss replacement with generation-safe recovery. Preserve explicit support boundaries and validate each claimed path. |

The two [RC4 issue drafts](../../spec/scene/slices/RC4_PAINT_QUERY_DEVICE_LIFECYCLE.md#issue-draft-1-composed-gpu-paint-picking) contain exact titles, bodies, dependencies, and acceptance criteria. Composed GPU paint picking and device lifecycle belong to `v0.4.0rc4`; polygon rendering/media and the bounded browser diagnosis belong to `v0.4.0rc3`.

## 4. Final `v0.4.0`

Exit criteria:

1. Reproducible source and binary artifacts pass final packaging, installation, consumer, rendering, documentation, license, and known-issue gates, and the required scientific 3D profile has explicit capability and validation acceptance.
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

## Pre-freeze Local Evidence Recorded 2026-08-31

Moved from `STATUS.md` during the 2026-09-05 routing cleanup. These results retain their original implementation heads and environments; they do not establish current remote state or exact RC3 candidate readiness.

The 2026-08-30 local RC3 readiness audit at implementation head `846fd3049` repaired generic swapchain-error fence recovery, custom panel-light restoration after runtime recovery, stable texture identity and bounded backend object counts across resize, installed public-header and binding coverage, external-handle test width, third-party license packaging, and CMake prerelease version behavior. That head builds and passes 1,162/1,162 display-enabled validation native tests with zero skips, 125/125 DRP2 fixtures, 100/100 runtime-vklite, specifications and WebGPU checks, the 1,597-function binding/API model, example manifests, all 25 current course excerpts, the strict documentation build, 15/15 wheel-backend tests, the 18-package license inventory, source-install audit, installed CMake and FetchContent consumers, and C integration smoke. Full-tree static analysis and prior CPU sanitizer evidence remain dispositioned; Vulkan-backed sanitizer teardown remains inconclusive. This is strong mutable-tree evidence, not the still-required exact RC3 candidate artifact or hosted-platform proof.

The pre-freeze follow-up through `b6e330282` made public WebGPU staging exclude cache-local, non-redistributable bundles by default; added WebGPU data, fixture, and runner checks to documentation publication; aligned RC3/RC4 public guidance; and repaired gallery-review evidence. The tool suite passes 158 tests with one environment skip and seven subtests, all 105 reviewed canonical stills pass full pixel validation, and a bounded five-example native batch passes. Review commands now fail after child failures, strict batch 22 names its non-C Qt/ImPlot members instead of silently omitting them, prepared-data content invalidates still and animation caches, and four experimental examples outside RC3 publication have explicit checked exclusions. The nine invalidated still-cache records now verify against canonical images, all 38 animation frame caches are regenerated and current, and all 29 MP4 card candidates plus posters fit their budgets. Exact visual approval and publication remain maintainer actions.

The 2026-08-31 pre-freeze gate repaired a scene-destruction crash exposed by full animation regeneration, aligned exportable Vulkan image creation with the allocator's external-memory policy, and hardened installed-wheel validation against checkout contamination. The display-enabled native matrix passes 1,175/1,175 with zero skips; the crashing animation completes its one-frame validation smoke and full 576-frame capture; all 38 animations pass freshness; the CUDA external-buffer example renders the expected red pixel with no validation error; and installed Python, bindings, native library, CMake metadata, shaderc, rendering, native-window, CMake, and Python/C example checks resolve from a repaired diagnostic wheel even under deliberately poisoned checkout and loader paths. The 158-case DRP2 suite, 117 selected vk/vklite/DRP2 checks with ten expected no-display skips, the five-example input/probing batch, and the runnable advanced-runtime paths pass. Local source distribution validation passes, including 18 license payloads and installed CMake/pkg-config consumers. The diagnostic wheel is `manylinux_2_38` and Debug, so it is evidence for the validator and payload shape, not an RC3 artifact; `manylinux_2_34`, Release, six-platform, vcpkg, conda, signing, and exact-version proof remain frozen-candidate gates.

A fresh remote clone at `f716786a3` with every submodule except `data` initialized built successfully, passed 1,162/1,162 display-enabled native tests with zero skips, built the strict documentation site, passed `distribution-validate-local all`, and passed the installed C integration smoke. This closes the RC3 no-data-independence evidence gap without starting the post-RC3 asset-catalog migration. Later commits in this campaign touch documentation and review tooling rather than runtime data dependencies; exact frozen-candidate artifact validation remains required.

The Point Cloud promotion passes the 159-test tool suite with one environment skip and seven subtests, strict documentation generation, six-public-bundle validation at 52,793,718 total bytes, the 500k-point WASM packet smoke, and committed-data native and Python fallback checks. The browser route reaches `QueueSubmit`; headed interaction and public-site confirmation remain release evidence gates because the available headless and Xvfb runs encounter the known external WebGPU instance-loss limitation. Grantor, date, scope, and durable permission-reference details remain pending.

The optional Qt bridge and local Apple Silicon split-package proof are complete. Vulkan-enabled Qt 6.11.1 build 2 is published for both macOS architectures. At that recorded checkpoint, the compatible PyQt PR was ready for review, cleanly mergeable, and green after its final 25-job Linux, macOS, and Windows matrix plus the additional fifteen-job ready-transition Azure cycle passed against the published Qt packages. A final Linux/Xvfb source-build smoke at `50008e9ff` rendered 120 hosted frames, passed the Qt example and Vulkan validation, and produced two byte-identical 1280x720 captures. The installed system PyQt lacks `QVulkanInstance` and reports the intended diagnostic; official managed PyQt proof remains deferred to RC4. The official conda Qt/PyQt provider and exact split Datoviz provider artifacts are deferred to RC4 because upstream PyQt publication remains outside the Datoviz release schedule; the source-build bridge remains available in RC3 with experimental provider wording.

Local Qt 6.11.1 build 2, PyQt6 6.11.0 build 3, split Datoviz packages, Vulkan instance, Cocoa surface, and hosted rendering proof are green; upstream Qt build 2 is published for both macOS architectures; PyQt PR #186 was recorded ready for review after Linux run `32184052560` passed 10/10 and Azure builds `1569935` and `1570229` each passed 15/15.


## RC3 Preparation Before Maintainer Review, 2026-09-08

The maintainer authorized autonomous prose review, gate reconciliation, available local checks, candidate drafts, and local commits. Linux, macOS, and Windows machines are available to the maintainer; remote access and desktop-session details have not yet been supplied. No release identity, tag, publication, submodule pointer, or binary payload was changed.

### Prepared and validated

- `a4cf201d8` reviews the course overview and chapters 1–3, adds Windows Developer PowerShell/Release/DLL-path commands, allows prerelease package installation, and records an unreleased RC3 changelog. All C, GLSL, and CMake blocks are unchanged; all 26 C excerpts match their canonical programs. Maintainer voice, pacing, and preview review remains pending.
- The strict documentation build and status checks pass; all four course media products regenerate successfully. The exact chapter-1 C and CMake blocks compile and run against the installed development prefix with both Ninja and Ninja Multi-Config on Linux. This checks the multi-configuration command shape, not MSVC or Windows DLL execution.
- The 53 release automation, conformance, wheel validation, and wheel backend tests pass in `.venv`. System Python lacks pytest; the existing development environment supplies it.
- `release-dry-run 0.4.0rc3 --profile light` completes its planning steps. Its `pass` labels mean command planning succeeded, not that candidate artifacts or release gates passed. Exact-artifact/report/publication steps remain absent or skipped because there is no frozen RC3 state or wheel set.
- Local review artifacts are `build/release/0.4.0rc3/release-notes.md`, `version-preparation.patch`, and `platform-validation.md`. The version patch passes `git apply --check` and is not applied. Its proposed citation date must be refreshed to the actual release date; applying it also requires `just ctypes` and `just ctypes-check`. Runtime, Python, and citation metadata remain `0.4.0rc2`.
- The durable release specs now consistently defer official Qt/PyQt provider publication and validation to RC4, while retaining the RC3 source bridge and missing-provider diagnostics. Base conda remains an RC3 requirement.

Detailed logs are in `/tmp/datoviz-rc3-preparation-20260908/`. The earlier 1,195 native tests, sanitizer/static-analysis checks, and local Release source/wheel proofs remain tied to their recorded code/artifact identities in [Fractal evidence](FRACTAL_CODE_HARDENING.md); they were not gratuitously repeated for authored-prose changes.

### Remaining sequence

| Work | Concrete next action | Acceptance boundary |
| --- | --- | --- |
| Maintainer review | Review the complete course and previews, visual pilot, exact media candidates, and refreshed draft notes. | Automated review does not replace human acceptance. |
| PR #157/#136/#132 | PR #157 merged the refreshed embedding and platform-portability work at `fccf83061b05b7373f5caa1ba40bdecfc3578306` on 2026-09-17. Linux, macOS, Windows, agent-instruction, and submodule-reachability checks passed. PR #136 is merged and superseded by #157; PR #132 is closed as superseded. | Complete; no implementation or disposition gate remains in this lane. |
| Machine access | Keep machine connection details in ignored local notes; verify each checkout and desktop session before running the prepared platform checks. Run the prepared source checks first, especially the unresolved Windows AMD/NVIDIA visible asymmetry. | Source and unattended results are development evidence; each real interaction observation stays pending until performed. |
| Version and candidate freeze | Review the prepared `0.4.0rc3` metadata/workflow-default patch, finalize date/scope, apply it, refresh/validate bindings, and commit the candidate. | Version changes require approval under `spec/release/RELEASE_AUTOMATION.md`; no published-install claim changes before official package proof. |
| Exact artifacts | Build one explicit source bundle and all six wheels, then hosted conformance and installed course/shaderc/CMake checks. | Match commit, filename, and checksum; older development wheels do not satisfy this. |
| Base conda and vcpkg | Use the exact candidate source and package identity in temporary recipe/overlay preparation; validate base outputs, Windows DLL consumers, and macOS managed Vulkan without Qt. | Current templates still say `0.4.0` with placeholder hashes; do not represent RC3 source as a validated stable package. No conda/vcpkg tool or prefix is available on this Linux host. |
| Physical and headed checks | Use the same accepted wheels on available machines for the fixed live set; record browser rendering with an explicit working provider or the narrowly scoped RC3 browser exclusion. | Preserve individual GPU identities, skips, and observations; Xvfb is not physical interaction proof, and the browser exception does not waive native platform gates. |
| Final evidence/publication | Finalize artifact checksums, signing decision, notes, TestPyPI/package-index verification, and publication actions. | Do not declare artifacts signed without an actual signature; external actions require exact reviewed approval. |

The release tool currently requires macOS arm64, Linux x86_64, and Windows AMD64 machine evidence for non-RC2 candidates. The narrative policy is looser for RC3 physical Linux/Windows; with Windows currently unavailable, leave its required proof pending and prepare to satisfy the stricter matrix when a suitable host returns rather than silently weakening it. Availability of an OS does not establish availability of every architecture in the six-wheel build matrix. Commands in the local platform plan keep source checks, hosted artifact checks, and human interaction separate.


### Portable source preview and course-runner correction

The platform-plan audit found that `tools/run_vulkan_course.py` emitted unquoted native source paths and assumed all CMake generators put executables directly in the build directory. Checkpoint `ae4d5cd03` normalizes and quotes paths (including spaces and non-ASCII characters), selects Release for multi-configuration generators, and finds the resulting executable directory. Two real CMake integration tests reproduce failures against the original runner and pass with Ninja and Ninja Multi-Config after the fix. All three actual course programs pass against the fresh installed prefix using Ninja Multi-Config; chapters 2 and 3 retain reproducible captures. This is Linux proof of the portable command logic; native MSVC and Mac execution remain pending.

A fresh Release source build/install and audit from `1079eb252` passes the 18-package license inventory, 111 installed headers, CMake/pkg-config consumers, and dependency/runpath checks. Conda and vcpkg remain unavailable skips. The final transferable development archive includes the subsequent course-runner correction from `ae4d5cd03e9fe79e3ef2dc18f979a16fe74ee3fe`; the complete intervening diff is the changelog, runner, and its two integration tests, with no native, build, or packaging-input changes. Its native payload uses the validated source, and its changed runner has separate installed-prefix proof.

The development preview and its `identity.json` are local artifacts under `build/release/0.4.0rc3/source-preview/`. Regenerate them from sanitized history before transfer; the earlier preview was withdrawn after private machine-access information was removed from unpublished commits. Native code and the validated course-runner logic are unchanged. The metadata deliberately remains `0.4.0rc2`; it is neither the published RC2 source nor a frozen RC3 candidate. Keep connection details in the ignored platform plan. No machine transfer or remote execution occurred.

## RC4 dependency refresh recommendation

The 2026-11-17 read-only assessment recommends a targeted post-RC3 refresh, not a blanket dependency sweep. [External dependency notes](../../spec/architecture/EXTERNAL_DEPENDENCIES.md#rc4-refresh-assessment-2026-11-17) record the observed pins, current upstream candidates, risks and validation sequence. Prioritize a coherent Vulkan toolchain baseline without raising the Vulkan 1.3 runtime requirement, and coordinate GUI updates with the existing ImGui/cimgui/ImPlot lane. GLFW/cglm/MSDF changes remain separate, fix-driven decisions; Kvazaar has no immediate upgrade need. RC3 dependency pins remain unchanged.

## Homepage media delivery completed

[The current media handoff](DOCUMENTATION.md#homepage-motion-review) records all ten selected media approvals and the published [twelve-attachment release](https://github.com/datoviz/datoviz.github.io/releases/tag/homepage-media-ff1b026c8256). All twelve anonymous download hashes, 19 Chromium/Firefox browser cases and 18 byte-range checks pass; Safari/iPhone remain unvalidated. The private WIP documentation now uses the real public release URLs. The live homepage, website branch and Pages configuration remain unchanged. Website deployment is a separate, unapproved gate, and the source publication guard remains false. This narrow delivery slice does not make the broader data migration an RC3 requirement.

## Immediate RC3 Readiness Sequence

1. Obtain maintainer proofreading and preview acceptance after the completed revised-course review, verified corrections and source/documentation checks. Homepage video selection and release-attachment publication are already complete; do not reopen those approvals unless the selected bytes change.
2. Reconcile remaining gallery/reference dispositions, provider scope and platform exclusions against their lane evidence, then prepare concise release notes and an explicit ready/blocked/unavailable checklist. Historical checks apply to their recorded revisions; unavailable hardware is not a pass.
3. Obtain the maintainer freeze decision before changing RC3 version metadata or choosing final artifacts. Build and validate exact source/wheel/base-conda artifacts, plus provider artifacts only if explicitly promoted, for each claimed platform, including installed course/shaderc/CMake smoke.
4. Obtain the required final publication approvals for the exact release and eventual website deployment. Until then, keep datoviz.org unchanged. Dependency refresh and broader asset migration remain outside RC3.

### Current readiness, 2026-11-20

| State | Current boundary / next action |
| --- | --- |
| Ready as recorded development evidence | Course implementation and Nov18 source checks, polygon rendering/configured-provider browser proof, narrow scientific RC3 corrections and packaging tooling retain their exact recorded identities. They do not establish frozen-artifact acceptance. |
| Complete | All homepage videos/posters and compatibility fallbacks are accepted; the twelve-attachment release is published and verified, and WIP docs use its public URLs. Do not ask for those approvals again unless the selected bytes change. |
| Linux reader pass complete / awaiting maintainer | The 2026-11-20 Claude Opus 5.5 medium reader run passes all 16 student programs from rendered instructions through an isolated Release installation. Three small troubleshooting corrections follow; maintainer proofreading/preview acceptance, other platforms and exact-package gates remain before release acceptance. |
| Pending exact disposition/action | Polygon/choropleth canonical images remain unpromoted despite completed rendering review. Prepare exact candidate/destination hashes and the approved action; retain scoped provenance/disposition for the three small raster residuals and unresolved four-page pilot scope. |
| Blocked on freeze, then artifact proof | Choose RC3 identity only after maintainer freeze; build the explicit source/six-wheel/base-conda set, validate installed consumers, vcpkg and mandatory hosted Linux/Windows proof, and finalize notices, signing/checksums and release notes. |
| Unavailable / conditional | Physical macOS/Windows remain unavailable in the last recorded platform update; Windows trust/runtime and visible dual-GPU checks remain unresolved. Safari/iPhone media validation is unclaimed. Provider promotion is optional and defaults to RC4. |
| Held | datoviz.org deployment remains unapproved and its publication guard remains false. Release/package publication and any future canonical promotion require their exact approvals; RC4 architecture and dependency upgrades are not RC3 prerequisites. |
