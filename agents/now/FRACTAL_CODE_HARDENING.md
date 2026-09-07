# Fractal Autonomous Code Hardening

Status: complete. Executed on Fractal from `b9fa60576c327230bf56e924327c30e3ca54a7fb`, 2026-09-07 through 2026-09-08 local time. This campaign prioritizes coding, focused refactoring, and static/dynamic analysis. It is development evidence, not release approval or exact-artifact proof.

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

### Remaining limits and follow-ups

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
