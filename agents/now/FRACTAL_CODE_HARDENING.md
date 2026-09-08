# Fractal Autonomous Code Hardening

Status: complete, including the constructor/provider and resize/vendor follow-ups below on 2026-09-08. Originally executed on Fractal from `b9fa60576c327230bf56e924327c30e3ca54a7fb`, 2026-09-07 through 2026-09-08 local time. This campaign prioritizes coding, focused refactoring, and static/dynamic analysis. It is development evidence, not release approval or exact-artifact proof.

The maintainer authorized concurrent code hardening and a bounded prose pass, then explicitly approved the final push to `origin/main` in the execution conversation. This supersedes the original separate-prose scheduling and local-only publication limit below for this run. Course rewriting, media publication, and `data` changes remain excluded.

## Objective And Scope

Run a sustained autonomous campaign, initially targeting roughly two hours of useful work, to strengthen failure handling and resource ownership with reproducible tests and focused fixes. Finish a coherent checkpoint rather than starting another large task near the time limit. Do not spend time merely to consume tokens.

Read [dispatch](START.md), [current status](STATUS.md), the completed [differential campaign](QA_DIFFERENTIAL_CAMPAIGN.md), and applicable repository rules before implementation. Do not restart the completed broad source audit. Record the starting commit and inspect production changes since the differential campaign's validated integration before choosing affected paths.

Prioritize deterministic allocation-failure coverage for buffer/descriptor transactions: rollback, ownership, cleanup, retry behavior, and preservation of the previous valid state. The prior campaign explicitly leaves exhaustive allocation-failure injection incomplete. Confirm the gap still exists before implementing narrowly scoped test hooks. Improve coarse DRP2 diagnostics only as a secondary task if evidence supports a bounded change that preserves protocol contracts.

Refactor cleanup or transaction code when a reproduced defect or a concrete testability problem justifies it. Preserve existing subsystem boundaries and borrowed Vulkan ownership. Broad architectural redesign, new features, unrelated style cleanup, public prose rewriting, and release publication are outside this campaign.

## Autonomous Execution

1. Establish the Linux baseline: commit, working-tree state, compiler/tool versions, GPU/driver, Vulkan provider, available sanitizers, and build/test routes. Preserve unrelated local changes; use an isolated checkout/worktree if needed under repository hygiene rules.
2. Inventory affected production paths and existing regression coverage. Select a bounded ownership or transaction boundary, document acceptance criteria, and assign distinct worker responsibilities.
3. Review failure paths and reproduce credible defects. Add deterministic failure injection only where existing facilities cannot exercise the relevant allocation/unwind behavior. Test externally meaningful invariants rather than mirroring implementation.
4. Run differential clang-tidy and bounded cppcheck on relevant translation units. Inspect underlying diagnostics and distinguish actionable findings from provider, configuration, and advisory output.
5. Run focused CPU ASan/UBSan/LSan and selected Valgrind checks where available. Bound potentially hanging runs; provider-induced Vulkan/shaderc stalls remain inconclusive. Continue independent checks instead of repeatedly retrying an unchanged blocker.
6. Implement focused fixes and justified refactors, then run appropriate regression, contract, binding, and native validation. After public-header/API/binding changes, run `just ctypes` and `just ctypes-check` before Python-facing validation. Follow build/GPU locks and applicable validation rules.
7. Obtain a fresh worker review of each substantive final change, resolve findings, and make coherent local checkpoint commits after required checks pass. The coordinator owns integration and commits.
8. Finish with a durable evidence update in this file: exact commits, changes, commands, results, timeouts, limitations, and prioritized follow-ups. Run `git diff --check` and inspect repository status and the staged set before committing. Do not claim unavailable validation passed.

Routine implementation decisions and bounded local fixes should proceed without manual intervention. If a material scope decision or an explicit approval boundary blocks one lane, record it and continue independent authorized work. No push, workflow dispatch, upload, PR, or other external publication is authorized by this execution plan. The user's approval to publish this plan does not authorize publishing later implementation commits. Leave `data`, runtime libraries, generated binaries, and unrelated user files unstaged and uncommitted.

## Subagent Plan

Use at most three workers alongside the coordinator, subject to the runner's actual concurrency limit. This plan explicitly requests subagent delegation and the model/effort assignments below. Pass narrow task context and the relevant contracts when using model overrides; do not rely on inherited full conversation history.

| Role | Preferred model / effort | Bounded responsibility |
| --- | --- | --- |
| Coordinator | Current strong model | Scope, architecture decisions, shared infrastructure, integration, validation decisions, and commits |
| Failure-path reviewer | `gpt-5.6-sol`, high | Review one ownership/transaction boundary; report contract violations, reproducers, and proposed fixes |
| Analysis worker | `gpt-5.6-luna`, medium | Run prescribed analysis, deduplicate diagnostics, and report relevant findings with file locations and evidence |
| Test worker | `gpt-5.6-sol`, medium | Implement deterministic regression tests in an assigned area and run focused sanitizer checks |

Give every worker explicit files, acceptance criteria, expected output, and a bounded assignment. Assign disjoint edit ownership; the coordinator owns shared allocation/test infrastructure. Serialize expensive builds and GPU runs against shared outputs and hardware. Workers must not commit, publish, or modify another worker's files without coordination.

Rotate completed workers into new bounded tasks. Use a fresh review assignment for the final diff and contracts. Escalate difficult Vulkan lifetime and transactional-semantics questions to the coordinator or a stronger available model. If a named model is unavailable, use an available equivalent or the current model and record the substitution; model availability must not block useful work. Reduce task scope before reducing reasoning effort for correctness review.

## Separate Prose Campaign

The alternative broad prose pass is a separate future campaign, not an automatic fallback or part of this coding run. It would use a strong coordinator, two `gpt-5.6-luna` medium-effort editors with disjoint documentation sections, and a `gpt-5.6-sol` high-effort technical reviewer. Apply the available `humanizer` skill after locating and reading it on Fractal; its availability on the planning Mac does not establish Linux availability.

Calibrate against a small representative batch and existing documentation conventions before expanding. Preserve technical meaning, qualifications, API contracts, code examples, and terminology; humanizer is an editing guide, not a word blacklist. Start with user guides and API explanations. Respect the existing maintainer voice-review gate before broad course rewriting. Follow documentation gates and finish with an integrated terminology, link, and semantic review.

## Execution Results

### Baseline and acceptance criteria

The starting checkout is `main` at `b9fa60576c327230bf56e924327c30e3ca54a7fb`. Existing untracked `agents/now/DOC_PROSE_AGENT_INSTRUCTIONS.md`, `agents/now/QA_DIFFERENTIAL_CAMPAIGN_PROMPT.md`, and `json_export.json` remain untouched. Prose work uses sibling worktree `/home/cyrille/GIT/Viz/datoviz-fractal-prose`, local branch `docs/fractal-prose-20260907`, from the same commit. Older campaign worktrees are not reused.

Compared with the previous published integration `42760e096`, production changes concern scene invalidation/strokes and Vulkan memory handling; the DRP2 buffer/descriptor transaction implementation and its deterministic allocation-failure coverage gap remain unchanged. This run targets host allocation failures in semantic state cloning, buffer construction, descriptor staging, and deferred retirement. Acceptance requires preserved old objects and ownership on failure, cleanup of staged allocations, same-runtime retry for semantic failures, and backend quarantine until reset for execution failures. Direct helper tests may retry a transaction without claiming public runtime recovery.

Fractal runs Ubuntu Linux x86-64, kernel `7.0.0-28-generic`, GCC 13, Clang/clang-tidy 18.1.3, cppcheck 2.13.0, Valgrind 3.22.0, CMake 3.28.3, and just 1.58.0. Vulkan loader 1.4.328 exposes NVIDIA RTX 5090 driver 595.84, Intel Mesa, and llvmpipe providers; Khronos validation is available. No display is set. The existing Debug build and separate Clang ASan/UBSan/LSan build are available. The coordinator serializes builds and GPU tests; CPU static analysis runs independently.

Baseline `DVZ_BUILD_JOBS=12 timeout 600 just build` passed with no work needed. `timeout 180 just test drp2` passed 158/158 selected tests (136 DRP2 and 22 scene), zero failures or skips. Logs for this run are under `/tmp/fractal-hardening/` and `/tmp/fractal-hardening-*`; these machine-local files are supporting evidence, not published artifacts.

The concurrent worker allocation is coordinator implementation/integration, `gpt-5.6-sol` high-effort failure-path review, `gpt-5.6-sol` medium-effort tests/analysis, and `gpt-5.6-luna` medium-effort prose editing. This combines the original analysis and test roles to fit four slots. A fresh review assignment follows implementation.

### Reproduced findings

- Real host allocation failure aborted in `dvz_buffer_create_wrapper()`, `dvz_descriptors_create_wrapper()`, and `dvz_allocation_create()`, despite each constructor documenting a `NULL` result. An exclusive CPU regression uses the existing stateless allocator callback and restores the allocator before assertions. Three incremental pre-fix runs reproduced each assertion, recorded in `wrapper-repro.log`, `descriptor-repro.log`, and `allocation-repro.log`. The constructors now return `NULL`, and `dvz_buffer_create()` returns failure when its allocation wrapper cannot be created.
- Direct bind-group replacement destroyed or deferred the old object before allocating its replacement descriptors. `bind-group-repro.log` records the old group disappearing after injected descriptor allocation failure. Replacement now stages descriptors first, reserves retirement storage, and publishes into the existing slot only after preparation succeeds. Public backend failures still quarantine the runtime until reset; the transaction fix does not relax that contract.
- Initial deferred-retirement allocation published capacity before storage existed. A failed allocation left a null pointer with capacity eight, allowing a subsequent reserve to report success without storage. The exclusive CPU regression failed at `!reserved` before the fix (`deferred-repro.log`). Capacity and storage now publish together after allocation succeeds, preserving the reservation guarantee used by buffer and descriptor replacement.

- Bounded cppcheck found caller-owned `pNext` pointers escaping to local stack structures in both external-memory import helpers. A deterministic Unix FD-query stub reproduced the mutation (`import-repro.log`). The helpers now pass local create-info copies to VMA and return immediately on VMA allocation failure before querying allocation metadata or output handles. The early-failure regression passes twice on the same input; the later VMA failure guards have source-review and successful-import regression coverage, not injected VMA-failure proof.

### Prose lane

The bounded batch covers `docs/start/{core-concepts,choose-your-layer,install}.md`, `docs/how-to/{create-a-scene,use-python,input-events,render-offscreen}.md`, and `docs/reference/callbacks.md`. Editing simplifies padded sentences, removes a repeated workflow paragraph, consolidates excerpt guidance, and keeps Markdown paragraphs on one source line. Review corrected an intermediate data-copy timing regression and clarified callback ownership wording. All fenced code and Markdown link targets remain unchanged. Independent technical review found no remaining lost lifetime, threading, binding, pixel-format, platform, or release-status qualification. The course and generated reference remain outside the batch.

### Validation

`just ctypes` and `just ctypes-check` passed before Python-facing integration checks: 16 binding tests, policy/facade validation, and ABI checks for 203 records. No generated binding changes are staged. `just docs-status-check` passed after integrating the eight-page prose patch. The full strict documentation build also passed, including its build dependency and both six-test gallery/tutorial helper suites. Prose checkpoint: `c4adddfd8` (`docs: clarify scene and Python workflow prose`).


The final validation build uses `DVZ_CMAKE_ARGS=-DDVZ_USE_VALIDATION=ON DVZ_BUILD_JOBS=12 just build`. The starting cached build had validation disabled; the final build explicitly enables it. Builds completed without compiler warnings. During test development, an accidental fixture edit caused a compiler error and the new test source required CMake reconfiguration; both were resolved before final validation. The existing `runtime_lifecycle.c` and `vklite_runtime.c` files remain unchanged.

| Check | Result |
| --- | --- |
| `timeout 120 just test allocation_failure` | 6/6 passed: five new DRP2 cases plus the existing scene failure-unwind test. |
| `timeout 45 just test drp2_runtime_vklite_buffer_allocation_wrapper_failure` | 1/1 passed; actual allocation-wrapper failure leaves the same configured buffer reusable. |
| `timeout 30 just test memory_import_failure_preserves_create_info` | 1/1 passed; CPU-only provider stub, restored before assertions. |
| `timeout 300 just test drp2` | 164/164 passed: 142 DRP2 and 22 scene tests, zero skips. |
| `timeout 180 just test vklite` | 79/89 passed, zero failures, 10 no-display presentation skips. |
| `timeout 180 just test memory` | 6/6 passed, including the CUDA memory-import regression. |
| `timeout 300 just test` | 1,149/1,187 passed, zero failures, 38 no-display skips. |
| `timeout 180 xvfb-run -a build/testing/dvztest --case-list /tmp/fractal-hardening/display-cases.txt` | All 38 previously skipped display cases passed: 24 canvas, 10 vklite, three GUI, and one scene case. |
| `timeout 180 just spec-check` | All groups passed: 137 DRP2 fixtures, 44 WebGPU preflight fixtures, runner smoke with 42 positives/two streams/89 negatives, 55 fixture-runner tests, 19 preflight tests, 15 scheduler tests, and scene source/boundary guards. |
| CPU ASan/UBSan/LSan | Three DRP2 allocation cases and the Unix import case passed, without suppressions or sanitizer reports. |
| CPU Valgrind | The same four cases passed with zero errors, zero bytes retained, and no suppressions in all four process reports. |
| GPU ASan/UBSan/LSan | Three-case attempt timed out at 60 seconds after GPU enumeration, with no sanitizer report. Inconclusive; not retried. |

The sanitizer build was configured in `build-asan` with Clang, `Debug`, `DVZ_ENABLE_ASAN_IN_DEBUG=ON`, `DVZ_ENABLE_CUDA=OFF`, `DVZ_USE_VALIDATION=OFF`, and `DVZ_SANITIZER=asan`; `cmake --build build-asan --target dvztest_drp2 dvztest_vk --parallel 12` completed. CPU runs used `ASAN_OPTIONS=halt_on_error=1:detect_leaks=1:detect_stack_use_after_return=1` and `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`. The DRP2 case list selected `wrapper_allocation_failure`, `deferred_initial_allocation_failure`, and `semantic_clone_allocation_failure`; the Vulkan component selected `memory_import_failure_preserves_create_info`. Valgrind used `--trace-children=yes --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=definite,indirect --error-exitcode=99` against the unified runner with those four exact cases.

Differential clang-tidy covered eight production translation units (`drp2/{backend,objects,pipeline,runtime,semantic}.c`, `vk/memory.c`, and `vklite/{buffers,descriptors}.c`) against the unchanged starting source in the prose worktree, using the same compilation configuration. Each run had a 60-second bound. Final diagnostics contain only the pre-existing `Drp2Object` padding advisory; a speculative zero-count descriptor-array warning exposed during development was resolved by returning before allocation for an empty table and checking the allocation result unconditionally. The exact command lists and comparison are in `/tmp/fractal-hardening/tidy-differential.json`.

Bounded cppcheck used the same eight-unit compilation database with `--enable=warning,performance,portability --inconclusive --inline-suppr --xml --xml-version=2 --error-exitcode=1` and a 180-second bound. Baseline analysis reported the two confirmed import stack-pointer escapes; final analysis completed all eight units with zero diagnostics. Earlier worker attempts with mismatched per-file filters produced setup errors and are excluded from analysis evidence.

### Remaining limits at the original checkpoint

1. Exercise real Vulkan descriptor-pool and VMA allocation failures separately. This run covers selected host allocations and early import failure; it does not inject every allocation, force device-memory exhaustion, or prove every import-failure path.
2. Extend the documented-NULL constructor checks to `dvz_drp2_runtime_vklite()` and `dvz_allocator_create()`, which still assert on allocation failure, and audit callers deliberately rather than broadening this transaction patch.
3. Investigate the GPU sanitizer timeout with provider-specific tracing. CPU sanitizer/Valgrind evidence and native Vulkan validation do not turn that timeout into a pass.
4. Coarse DRP2 diagnostic codes remain unchanged. Windows/macOS physical validation, hosted runs, exact-artifact checks, broad course voice review, and media publication are not claimed by this campaign.

### Integration checkpoints

- `c4adddfd8`: `docs: clarify scene and Python workflow prose`, eight authored guide/reference pages, independently reviewed and validated with the strict documentation and status checks.
- `2a2b2f99c`: `fix(runtime): preserve resources across allocation failures`, reviewed production fixes, internal fault injection, six DRP2 regressions in a dedicated allocation test file, and one Unix import regression.

Fresh independent high-effort review found no remaining blocking defect in the final production changes, ownership rules, or regression suite. The prose review checked technical fidelity separately. A final incremental build passed after arranging the new test file's include section; no runtime logic changed after the reported validation. `git diff --check`, staged-path/stat inspection, and commit preflights passed. The recorded `data` gitlink `b94d32d9c0a0a4c47e7e5c393b4ccc570159ed96` passed `python3 tools/check_submodule_reachability.py data --revision HEAD` against advertised `v0.4-dev`.

The temporary prose worktree and branch were removed after verifying that all eight files exactly matched committed checkpoint `c4adddfd8`. Older worktrees and the three original untracked user files remain untouched.

Only the maintainer-approved `origin/main` push is authorized. No submodule, binary payload, unrelated user file, release artifact, or media promotion enters these checkpoints. The final evidence commit contains this record and the dispatch update; publication verification is reported in the execution conversation.


## Constructor and provider follow-up, 2026-09-08

Starting head: `8225bf26ff9f9fddedb0f536a442a2536f16f933`. The maintainer authorized autonomous QA, prose, subagents, and local checkpoint commits. No push or other publication was requested. Three workers handled constructors/callers, descriptor/VMA failures and static analysis, and prose/review; the coordinator integrated image unwind, validation, packaging, and commits. Existing untracked files and `data` remain untouched.

### Changes and reproduced failures

- `dvz_drp2_runtime_vklite()` and `dvz_allocator_create()` return `NULL` when allocation fails. The extended constructor regression reproduced the allocator assertion before the fix and verifies successful construction and semantic execution after restoring allocation. App, GPU-context, and scene-query callers already propagate `NULL`.
- Canvas propagates allocator-wrapper failure and releases partially initialized primary/readback allocators. Its new offscreen failure/retry test exposed a second assertion: destruction disabled video before a stream existed. Stream teardown now checks that the stream exists. The regression creates and renders a fresh canvas after the failed creation. Readback-wrapper-specific injection remains unimplemented; that unwind path has source-review coverage.
- Descriptor sets publish their handles and ownership only after successful Vulkan allocation. DRP2 checks the allocated set count before writing descriptors, preserving the old bind group on failure. Tests inject `VK_ERROR_OUT_OF_POOL_MEMORY`, verify safe free/retry and preserved old binding state, and exercise an empty layout without calling Vulkan with an invalid zero set count. Running the regression against the original descriptor implementation reproduced the zero-count Vulkan validation errors and incorrect wrapper publication. Direct transaction-helper retry does not relax public runtime quarantine/reset semantics.
- Image creation propagates allocation-wrapper failure through the existing unwind loop. The regression reproduced the assertion before the fix and covers failure both before any image and after the first image has been created, followed by successful reuse and repeated destruction.
- A test-owned VMA function table rejects actual `vkAllocateMemory` calls with `VK_ERROR_OUT_OF_DEVICE_MEMORY`. Dedicated buffer/image allocations fail twice with clear handles, then the same allocation wrappers succeed with a healthy allocator. This covers the regular VMA allocation boundary, not every external-import or physical device-exhaustion path.
- Four additional prose pages cover visual updates, sampled fields, colormaps, and object lifetimes. The humanizer-guided pass clarifies wording and removes repeated introductions while preserving fenced code and link targets. Source checks covered sampled-field restrictions and Python array-copy behavior. Course rewriting and visual rollout remain outside this batch.

Independent review checked constructor callers, partial canvas teardown, image rollback, descriptor publication, DRP2 validation, VMA function-table ownership, and restoration of process-wide test hooks before assertions. Review added the empty-layout descriptor regression. No blocking findings remain in these changes.

### Checkpoints and validation

- `6d4affab6`: `docs: clarify visual updates and resource lifetimes`, four authored pages.
- `41ab7c28d`: `fix(runtime): unwind constructor and Vulkan allocation failures`, implementation and regressions. All runtime validation below applies to this code; no public headers or signatures changed.

Logs and machine-local artifacts are under `/tmp/fractal-followup-20260908/`; initial constructor/canvas reproductions also use `/tmp/datoviz-constructors-*.log` and `/tmp/datoviz-canvas-allocator-after.log`. Environment remains the Fractal Linux/GCC 13/Clang 18/RTX 5090 setup recorded above. The native Debug cache has `DVZ_USE_VALIDATION=ON`.

| Check | Result |
| --- | --- |
| `DVZ_BUILD_JOBS=12 timeout 300 just build` | Passed, no compiler warnings in the incremental build. |
| `timeout 180 just test allocation_failure` | 9/9 passed, zero skips. |
| Focused constructor and canvas wrapper tests | 1/1 each passed; canvas performed offscreen rendering. |
| `timeout 300 xvfb-run -a just test` | 1,191/1,191 passed, zero failures or skips; runner time 73 seconds. |
| `timeout 180 just spec-check` | Passed: 137 DRP2 fixtures, 44 preflight fixtures, runner smoke (42 positives/two streams/89 negatives), Python fixture/preflight/scheduler checks and source guards. |
| `just ctypes` then `just ctypes-check` | Passed, including 203-record ABI validation; no generated binding changes. |
| `just docs-status-check` and `DVZ_BUILD_JOBS=12 timeout 300 just docs-build-check` | Passed, including both six-test media helper suites and the strict documentation build. |
| Bounded cppcheck 2.13.0 | Six changed production translation units completed with zero diagnostics, using the filtered compilation database, warning/performance/portability and inconclusive checks, and a 180-second timeout. |
| CPU ASan/UBSan/LSan | Extended constructor regression passed without suppressions. |
| llvmpipe ASan/UBSan/LSan | Six GPU regressions passed without suppressions or skips: buffer wrapper, VMA provider, canvas wrapper, DRP2 bind-group replacement, images, and descriptors. |

The existing Clang `build-asan` configuration retains ASan/UBSan/LSan, CUDA off, and Vulkan validation off. Refreshed `dvztest_drp2`, `dvztest_vk`, `dvztest_canvas`, and unified `dvztest` targets with `cmake --build build-asan --target <target> --parallel 12`; no compiler warnings. Software-provider checks used `VK_DRIVER_FILES=/usr/share/vulkan/icd.d/lvp_icd.json`, `DEBUGINFOD_URLS=`, `ASAN_OPTIONS=halt_on_error=1:detect_leaks=1:detect_stack_use_after_return=1:symbolize=0`, and `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`, with 60-second bounds. The image/descriptor cases require the unified runner because the component runner manually registers only selected vklite cases.

### NVIDIA sanitizer diagnosis

The allocation-wrapper case timed out after 25 seconds with default symbolization. Process inspection found it blocked reading the llvm-symbolizer pipe. Under GDB with leak detection disabled, it completed successfully in about two seconds. Unsymbolized leak detection completed and reported 6,792 bytes in 33 allocations, including libdbus and unresolved module frames. Setting `DEBUGINFOD_URLS=''` and `ASAN_SYMBOLIZER_PATH=/usr/bin/llvm-symbolizer-18` also produced the report promptly. This narrows the observed stall to post-test leak symbolization; it is not a clean NVIDIA LSan result, and the unresolved leaks are not all classified as external. The same case passes unsuppressed leak detection with llvmpipe.

### Local Release source proof

From clean tracked implementation head `41ab7c28d`, ran `DATOVIZ_DIST_VALIDATE_WORKDIR=/tmp/fractal-followup-20260908/source-release CMAKE_BUILD_PARALLEL_LEVEL=12 timeout 900 just distribution-validate-local source-install`, followed by `just distribution-validate-local audit` with the same work directory. Both passed: 18-package notice inventory, fresh archive extraction, Release build/install, installed CMake/pkg-config consumers, metadata, headers, and dynamic-dependency audit. vcpkg and conda prefixes are unavailable and were explicitly skipped by the audit. The fresh build reported one warning in vendored `external/msdf-atlas-gen/msdf-atlas-gen/json-export.cpp:69` about a null `%s` argument; vendored code remains unchanged.

The diagnostic archive is `datoviz-0.4.0-source.tar.gz`, SHA512 `cc29337e81fad7df9a36ad9f98a0a1c4a5f0c58932c7baba3b0eb9f68ab64b71642be967b8303d6ca49c379ed8bbe5f614ada93015289a8d8164012fe04d0a1d`. The recipe's default archive label is `0.4.0`; embedded project version remains `0.4.0rc2`. No version freeze or release identity is claimed. Source bundling reads tracked working-tree bytes, so the code/prose checkpoints were committed before this run. Artifacts remain local and unstaged.

### Local manylinux wheel proof

Built from a separate extraction of the same source archive, mounted at `/workspace` in disposable `datoviz-manylinux:latest` (starting image `sha256:0e8dbfbc99a911cd7aea6bc7261a9e46400018eca0ed24f25a373f11785584a8`). The initial cached image lacked prerequisites; the existing `tools/release_wheels/manylinux_build_inside.sh x86_64` installed them inside the container. The 900-second attempt used a 12-CPU/24-GB container limit, a 12-job Ninja wrapper, `CMAKE_BUILD_PARALLEL_LEVEL=12`, and `DATOVIZ_MANYLINUX_GENERATE_CTYPES=0` to retain the already-refreshed archive bindings. The primary checkout and its wheelhouse were not mounted. GCC 14.2.1 built Release successfully; the compiler reported a vendored Kvazaar AVX2 `-Wstringop-overflow` warning at `external/kvazaar/src/strategies/avx2/intra-avx2.c:862`, which remains undispositioned and unchanged.

The resulting local wheel is `/tmp/fractal-followup-20260908/manylinux-source/wheelhouse/datoviz-0.4.0rc2-py3-none-manylinux_2_34_x86_64.whl` (21,236,692 bytes), SHA512 `20b44759419825b4ac78608865e4a65d0f55b964cfdbfa223648354b7c37303a602892a871a86c93188ce69cb10743444ac31202dd53f8999aa9f39ff898e2a3`. Native-dependency inspection and clean Python 3.13 installation passed in the builder, including Release-build, precompiled-SPIR-V, runtime shaderc, and installed CMake-consumer checks. The optional Qt probe correctly reports unavailable PyQt6.

On Fractal, `timeout 300 xvfb-run -a python3 tools/release_wheels/check_wheel.py --wheel <wheel> --work-dir /tmp/fractal-followup-20260908/wheel-host --release-build --precompiled-shaders --shaderc --cmake-consumer --render --window --examples render --qt-probe optional --keep` passed. This covers a clean Python 3.12 installation, installed package/binding/native-library/CMake path assertions, offscreen rendering, a native-window smoke under Xvfb, installed Python/C rendering examples, and runtime shader compilation. The validator used its existing `uv` fallback because system Python lacks ensurepip. No hosted or subjective physical-display proof is claimed.

`tools/run_vulkan_course.py --installed-prefix <installed-wheel-package> --runtime-dir <installed-wheel-package>` also passed all three current course programs using the configured SDK environment; steps 2 and 3 produced reproducible captures. An initial run that cleared `LD_LIBRARY_PATH` while retaining the SDK layer configuration failed to create a GPU context. `VK_LOADER_DEBUG=error` identified the missing `libVkLayer_khronos_validation.so`; this was an inconsistent layer/library search environment, not a wheel rendering failure. Clearing the corresponding SDK/layer variables as well (`VK_LAYER_PATH`, `VK_ADD_LAYER_PATH`, `VULKAN_SDK`, and `VK_SDK_PATH`) passes all three steps too; that run does not establish active validation-layer coverage. These results exercise the new API from a locally built wheel whose development version remains `0.4.0rc2`; they do not claim that the published immutable RC2 wheel can compile the course or close the first-official-post-RC2-package gate.

### Remaining work

1. Resolve NVIDIA leak reports with attributable provider/module stacks; preserve the unsuppressed llvmpipe proof separately. Disposition the two recorded vendored Release-build warnings without silently suppressing or modifying third-party code.
2. Cover readback-allocator-specific failure and later external-memory import failures if suitable instance-scoped seams become available; physical exhaustion is not tested by deterministic provider rejection.
3. Resolve `dvz_buffer_resize()` semantics deliberately: `include/datoviz/vklite/buffers.h` says it only changes requested size and does not recreate a live buffer, while `src/vklite/buffers.c` destroys and recreates on growth without checking creation failure. Production callers are absent, but changing behavior or the public contract deserves its own focused decision and regression coverage.
4. Preserve the remaining exact RC3 version/artifact, six-platform wheel, hosted/physical platform, conda/vcpkg, headed browser, course voice, and publication gates. Local checks and commits do not close them.


## Resize, late failures, and vendor diagnosis, 2026-09-08

Starting head: `31ca4541c`. The maintainer approved the combined next steps, autonomous implementation, cheap subagents, and local checkpoint commits. A low-cost Luna worker developed vendor reproductions; a Sol worker covered Canvas readback and late imports; a reused worker attributed NVIDIA leaks and independently reviewed the buffer change. The coordinator resolved resize semantics, integrated changes, and ran validation. No publication, submodule pointer update, or binary payload was authorized or performed. Original untracked user files remain untouched.

### Runtime checkpoint

`b67d0a95d` makes `dvz_buffer_resize()` a fallible, grow-only operation on live buffers. Same-size and smaller requests are successful no-ops; zero size and invalid state fail. Successful growth replaces the allocation without preserving contents; an already mapped buffer remains mapped through a new pointer. The caller must finish GPU work and refresh references to the old Vulkan handle. Failed heap allocation, Vulkan allocation, or replacement mapping preserves the old handle, size, mapping, and contents. This matches historical grow-only behavior while correcting the contradictory header contract and destructive failure path. The public return type changes from `void` to `int`; tracked bindings and generated API references were refreshed, including pre-existing reference drift from committed app/DRP2 headers.

The heap-failure regression reproduced destruction of the old buffer before the fix. Two exclusive regressions cover heap, provider, and mapping failures, invalid/no-op requests, allocation-count cleanup, and successful mapped/unmapped growth. The provider test uses a test-owned VMA table forwarding scoped allocation/map callbacks; global hooks are restored before assertions. Independent source and test review found no blocking issue.

Canvas now has an instance-scoped readback-wrapper rejection after primary VMA creation. The regression verifies both allocators unwind and retries the same private allocator lifecycle successfully. The earlier full-constructor failure/render retry remains separate coverage. Late external-buffer/image imports clear stale wrapper metadata and output handles on VMA failure. Their Unix regression seeds metadata through real allocations, rejects dedicated Vulkan allocation twice, preserves caller create-info chains and FD ownership, then reuses the wrappers for ordinary successful allocations. This does not claim successful imported-memory recovery or physical exhaustion coverage; the Unix-only regression explicitly skips on Windows.

### Validation

Logs are under `/tmp/datoviz-next-20260908/`; native validation uses the same Fractal environment and validation-enabled Debug configuration as the preceding campaign.

| Check | Result |
| --- | --- |
| Native build and full Xvfb suite | 1,195/1,195 passed, zero failures or skips, 74-second runner time. |
| Focused resize, Canvas readback, and late import tests | Four new regressions passed; broader memory and Canvas allocator selections also passed. |
| Refreshed Clang ASan/UBSan/LSan unified runner | All four new regressions passed with llvmpipe, no suppressions and no skips. |
| Bounded cppcheck | Three changed production translation units, zero diagnostics with warning/performance/portability and inconclusive checks. |
| `just ctypes` and `just ctypes-check` | Passed, including 203-record ABI validation, before Python-facing checks. |
| `just spec-check` | Passed all fixture, preflight, scheduler, runner, and source-guard checks. |
| `just docs-api-check`, `just docs-build-check`, `just docs-status-check` | Passed generated-reference consistency, strict build, media helpers, and status policy. |

The four software-provider sanitizer cases use `VK_DRIVER_FILES=/usr/share/vulkan/icd.d/lvp_icd.json`, empty `DEBUGINFOD_URLS`, `ASAN_OPTIONS=halt_on_error=1:detect_leaks=1:detect_stack_use_after_return=1:symbolize=0`, and `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`. No clean NVIDIA leak result is implied.

### Vendor warning reproductions

Checkpoint `4a4b3c3e9` adds `tools/check_vendor_warnings.py`, a bounded diagnostic against actual checked-out vendor sources. It reports compiler/submodule provenance, uses temporary harnesses and sanitizer builds, and accepts source overrides for temporary patches. Exit 77 means unavailable or partially skipped; a real failure takes precedence. Its default combined invocation intentionally fails on the current msdf defect and is not part of the normal passing test suite.

- Kvazaar `6040962bed5cc68c5ad01234c38c08b8b2822068`: the generic and AVX2 angular implementations match across four widths (4, 8, 16, 32), 33 modes, and 1,000 deterministic input patterns: 132,000 comparisons under GCC 13 ASan/UBSan. The actual GCC 14.2.1 Release warning reproduces in the cached manylinux image. A separate nonsanitized GCC 14 `-O3 -DNDEBUG` run passes the same 132,000 comparisons while emitting the warning. The scalar bounds for width 32 write indices 31 through 63 of a 64-byte array. Adding one or 32 padding bytes does not remove the GCC 14 warning, so padding is not adopted as a fix. This is bounded evidence against the reported overflow for supported widths, not a proof for every input/compiler. GCC 14 sanitizer execution is unavailable because the cached image lacks libasan/libubsan; no suppression or vendor change was made.
- msdf-atlas-gen `6148900d59423059bafde2f51a0cb303184404bd`: the real `exportJSON()` accepts representable unnamed `ImageType(6)`, truncates an existing file, and passes null to `%s`. The harness checks all six supported names and requires invalid input rejection before touching a sentinel file. The baseline fails; a temporary source patch validates the image-type string before `fopen()` and passes with ASan/UBSan. This confirms a vendor input-validation defect; it does not demonstrate a reachable invalid enum in Datoviz's current caller. The vendor revision stays unchanged pending a reviewed upstream/fork disposition.

Reproduce with `python3 tools/check_vendor_warnings.py --case kvazaar` and `python3 tools/check_vendor_warnings.py --case msdf`; test a patched copy with `--msdf-source /absolute/path/to/json-export.cpp`. Host pass/fail and missing-compiler exit-77 evidence is in `vendor/host-*.log` and `vendor/missing-compiler.log`; GCC 14 optimized evidence is in `vendor/gcc14-differential.log`.

The exact tested temporary change in `vendor-msdf-MFyo.cpp` is the following guard at the start of `exportJSON()`, before `fopen(filename, "w")`:

```cpp
if (!imageTypeString(imageType)) return false;
```

It is retained here as a reviewable patch proposal, not applied to `external/`. An upstream-quality change can store the validated string and reuse it in `fprintf`; that refinement has not been tested in this campaign.

### NVIDIA attribution

Minimal context create/destroy and context-plus-buffer runs report the same leak totals: one lifecycle produces 2,168 bytes in 11 allocations; five lifecycles produce 11,432 bytes in 55 allocations. Buffer work adds no observed leak to the context baseline. Repetition increases retained allocations, so these results do not justify calling it a fixed one-time cache.

Loader traces resolve every formerly unknown frame in the original runner leak report to `libnvidia-glcore.so.595.84`, Build ID `f6a4d1811f1a23de23b5d525e956758e89fe7e4b`, at offsets `0x9e7c51`, `0x9e7f0d`, `0xa191f6`, `0xa6c373`, or `0xfc4580`. The child reports 6,792 bytes/33 allocations across three glcore load/unload lifecycles; the parent reports 2,168 bytes/11 allocations across one. All eleven child and four parent unknown PCs match loader bases, and the DBus allocation stacks enter glcore. Explicit NVIDIA selection and unrestricted ICD discovery agree. The evidence identifies the allocating module and lifecycle trigger; it does not prove who should perform cleanup or turn NVIDIA LSan into a pass. No leak suppression or speculative Datoviz workaround was added.

Machine-local harnesses, loader traces, address mapping, and differential totals are in `nvidia/`, with machine-readable `attribution.json`. Empty `DEBUGINFOD_URLS` and `/usr/bin/llvm-symbolizer-18` avoid the earlier symbolizer stall.


### Fresh Release source proof

From runtime checkpoint `b67d0a95d`, `DATOVIZ_DIST_VALIDATE_WORKDIR=/tmp/datoviz-next-20260908/source-release CMAKE_BUILD_PARALLEL_LEVEL=12 timeout 900 just distribution-validate-local source-install` and the subsequent `audit` passed. The fresh archive builds and installs in Release, validates the 18-package notice inventory, and passes installed CMake/pkg-config consumers, 111-header inventory, metadata, dependency, and runpath checks. The absent vcpkg and conda prefixes are explicit skips. The only compiler warning is the investigated msdf null `%s` warning.

The local archive is `source-release/source-bundle/datoviz-0.4.0-source.tar.gz` (15,108,954 bytes), SHA512 `2ef4ba0c8153dbf18b40b8d10f0d221de0062e7382b6bbcbe8703ac03607ecbcaaeb0dee5799eb4323031cb13cacc708ef441310053258c05a02539bc0658f9a`; the installed prefix is `source-release/source-prefix`. As before, the diagnostic archive label is `0.4.0` and embedded development version is `0.4.0rc2`; this is not an immutable RC3 release candidate. Logs are `source-install.log` and `source-audit.log` under the campaign scratch directory.


### Fresh local manylinux wheel proof

A separate extraction of the same `b67d0a95d` source archive built in cached `datoviz-manylinux:latest` with a scratch-only writable mount, 12-CPU/24-GB limits, 12-job Ninja, and `DATOVIZ_MANYLINUX_GENERATE_CTYPES=0`. The 900-second-bounded Release build, repair, dependency inspection, and clean Python 3.13 builder installation passed. The known Kvazaar AVX2 warning remains visible; unavailable CUDA, optional tinyxml2 SVG support, and PyQt6 are reported rather than counted as passes.

The wheel is `manylinux-source/wheelhouse/datoviz-0.4.0rc2-py3-none-manylinux_2_34_x86_64.whl` (21,237,804 bytes), SHA512 `b200d9577bf74e053e9299c4d1501bbc37bb629e1669b96441341284a5602dfa5670238e2c6f5508009a0e4a017b7004492b4c2126cf893e4cc02d4b3ba656c8`. It remains local and unstaged.

The Fractal Xvfb wheel validator passed in a fresh Python 3.12 environment with `--release-build --precompiled-shaders --shaderc --cmake-consumer --render --window --examples render --qt-probe optional --keep`. The installed binding additionally verifies `dvz_buffer_resize.restype is ctypes.c_int`. All three installed course steps pass with consistent cleared SDK/layer/library environment; steps 2 and 3 produce reproducible captures. This course run does not claim validation-layer coverage. Logs are `manylinux.log`, `wheel-host.log`, `wheel-restype.log`, and `wheel-course.log`. The wheel is development evidence, with no hosted or physical-display acceptance claim.

### Remaining limits after this follow-up

1. Review and publish the msdf patch through the chosen upstream/fork workflow before updating its vendor revision; the baseline diagnostic continues to fail until then. No external submission has been made.
2. Retain the unsuppressed llvmpipe proof and NVIDIA module attribution separately. A driver comparison or vendor report is the next investigation step; allocation responsibility is not fully resolved.
3. Keep the Kvazaar warning visible with the bounded scalar/sanitized/optimized differential evidence. No observed supported-width defect warrants a local padding patch, and GCC 14 sanitizer coverage remains unavailable in the cached builder.
4. Exact RC3 identity/artifacts, the six-platform wheel matrix, hosted/physical platform checks, conda/vcpkg, headed browser, course voice, and publication gates remain as recorded in the release lanes. These local development artifacts do not close them.
