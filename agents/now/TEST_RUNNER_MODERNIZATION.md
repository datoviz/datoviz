# Test Runner Modernization Plan

> **Execution Status**
> - **Status:** `ACTIVE DESIGN NOTE`
> - **Updated on:** `2026-05-18`
> - **Purpose:** document the current C test-runner limitations and a staged refactor path toward
>   explicit grouping, cleaner filtering, and safe future parallelism.


## Summary

The current C testing framework is useful and lightweight, but it is still organized around a
single flat list of test functions executed through one shared mutable `TstSuite`. That model keeps
the runner simple, but it makes semantic grouping, resource-aware scheduling, and parallel execution
hard to add without risky ad-hoc rules.

The recommended strategy is not to immediately parallelize tests. First, upgrade the runner into a
metadata-rich registry with explicit modules, groups, cases, tags, resource requirements, and
isolation constraints. Serial execution should remain the default until tests opt into stronger
parallel-safety guarantees.


## Current Runner Audit

1. The runner owns one shared `TstSuite` for registration and execution state.
   - The same object stores the item registry, user context, log-capture state, expected-error
     state, unexpected-error state, captured logs, and result state.
   - Any in-process parallel execution would race on these fields unless run state is split from
     registry state.

2. Log interception is process-global.
   - `tst_suite_run()` installs one global log intercept for the whole suite.
   - Expected-error and log-capture helpers depend on that intercept mutating the current shared
     suite.
   - This is the main blocker for thread-level parallelism.

3. Grouping is implicit and based on fixture mechanics.
   - Tests are bucketed by matching `setup`, `teardown`, and flags.
   - That is useful for execution reuse, but it does not express semantic groups such as
     `math/array`, `drp2/runtime-validation`, `scene/frame-plan`, or `scene/app-offscreen`.

4. Test names encode too much structure.
   - Long fixture names currently carry module, subsystem, behavior, and sometimes backend.
   - This makes substring filtering possible, but it is brittle and pushes organization into names
     instead of first-class metadata.

5. Current flags are too coarse.
   - `TST_ITEM_FLAGS_STANDALONE` only separates per-test fixture execution from grouped fixture
     execution.
   - Future scheduling needs explicit metadata for thread safety, process isolation, GPU usage,
     GLFW usage, filesystem usage, environment mutation, video backends, and global hooks.

6. Some tests mutate process-wide or shared external resources.
   - Examples include environment variables, fixed `/tmp` paths, GLFW process-global state, Vulkan
     loader/runtime state, video encoder paths, and global logging hooks.
   - These tests are valid, but they must be marked as serial or process-isolated before any
     broader parallel scheduler exists.

7. Component runners already provide a useful sharding foundation.
   - Existing binaries such as `dvztest_core`, `dvztest_drp2`, `dvztest_scene`, `dvztest_vk`, and
     `dvztest_canvas` are a good first layer for process-level parallelization.
   - They should remain part of the design rather than being replaced by in-process threads.


## Target Model

The target runner should treat tests as registry entries with explicit metadata:

1. `module`: broad owning module, for example `common`, `math`, `drp2`, `scene`, or `canvas`.
2. `group`: semantic group, for example `array`, `runtime-validation`, `frame-plan`, or
   `app-offscreen`.
3. `name`: short case name. The C function name can remain long if useful, but it should not be
   the only source of structure.
4. `tags`: free-form compatibility labels for flexible filtering.
5. `resources`: required external/process resources such as CPU-only, GPU, Vulkan, GLFW,
   filesystem, environment, video, or logging capture.
6. `isolation`: scheduler contract such as thread-safe, process-only, or serial-only.
7. `setup` and `teardown`: fixture hooks, retained as execution mechanics rather than semantic
   grouping.
8. `timeout_ms`: optional per-case timeout metadata for live, GPU, and video paths.
9. `tmpdir`: optional runner-assigned per-case scratch directory for filesystem isolation.

Illustrative API shape:

```c
TST_MODULE_BEGIN(suite, "scene");
TST_GROUP_BEGIN(suite, "frame-plan");
TST_CASE(test_frame_plan_static_render);
TST_CASE(test_frame_plan_graph_validation_missing_usage);

TST_GROUP_BEGIN(suite, "app-offscreen");
TST_CASE_EX(
    test_app_offscreen_has_nonblank_pixels,
    TST_RES_GPU | TST_RES_VULKAN,
    TST_ISOLATION_PROCESS);
```

The exact macro names and descriptor types should be chosen during implementation, but the design
goal is stable: tests should be filterable and schedulable by explicit metadata instead of long
function names.


## Refactor Strategy

### Phase 1: Registry/Runtime Split

Keep behavior serial and backward-compatible while splitting concepts internally:

1. Make `TstSuite` primarily a test registry after registration.
2. Introduce a per-test or per-worker run context for:
   - log capture,
   - expected-error state,
   - unexpected-error state,
   - captured logs,
   - per-test temporary paths,
   - result details.
3. Keep `TEST_SIMPLE()` and `TEST()` working exactly as today.
4. Assign default metadata to legacy registrations:
   - `module = NULL`,
   - `group = NULL`,
   - `resources = TST_RES_NONE`,
   - `isolation = TST_ISOLATION_SERIAL`.

This phase should not change test ordering or pass/fail behavior.


### Phase 2: First-Class Metadata API

Add explicit registration helpers and migrate one small module as the reference pattern.

Recommended pilot module: `math`.

Expected additions:

1. module/group scope helpers,
2. case descriptor registration,
3. resource/isolation enums,
4. result records that preserve module/group/name,
5. `--list` output showing the registry,
6. `--list-groups` output for semantic groups.

The old name-substring filter should remain supported during migration.


### Phase 3: CLI Filtering Cleanup

Add structured filtering before adding parallel execution:

1. `--module <name>`
2. `--group <name>`
3. `--tag <tag>`
4. `--exclude-tag <tag>`
5. `--resource <resource>`
6. `--isolation <mode>`
7. `--list`
8. `--list-groups`
9. `--json <path>` for machine-readable results

The current single positional substring filter can remain as a compatibility alias, but new scripts
should use structured filters.


### Phase 4: Resource-Aware Serial Scheduler

Still run serially, but route all selected tests through a scheduler that understands resources and
isolation metadata.

This phase validates the future scheduling contract without introducing concurrency. It should also
warn when a test declares conflicting metadata, for example:

1. thread-safe plus environment mutation,
2. thread-safe plus process-global log capture,
3. thread-safe plus GLFW,
4. filesystem usage without a unique temp path.


### Phase 5: Process-Level Parallelism

Add process-level sharding before thread-level workers.

Preferred approach:

1. The runner can list selected cases/groups as machine-readable units.
2. A parent runner or script launches child processes with explicit filters.
3. Child processes run serially and write JSON result records.
4. The parent aggregates results and prints the normal summary.

This gives isolation for crashes, environment variables, GLFW, Vulkan loader state, fixed global
hooks, and video backends. It is the safest first parallel execution mode.


### Phase 6: In-Process Thread Workers

Only add thread workers for tests explicitly marked `TST_ISOLATION_THREAD_SAFE`.

Requirements before enabling this:

1. log capture and expected-error state are per-worker or thread-local,
2. result aggregation is synchronized,
3. output is buffered per case and printed deterministically,
4. shared suite registry is immutable during execution,
5. tests that use GPU, GLFW, video, environment mutation, fixed files, or process-global hooks are
   excluded unless they have a proven narrower contract.

Thread workers should be opt-in and remain secondary to process sharding for heavy graphics tests.


## Resource And Isolation Taxonomy

Recommended resource flags:

1. `TST_RES_NONE`: no known external resource.
2. `TST_RES_CPU`: CPU-only work.
3. `TST_RES_GPU`: any GPU runtime dependency.
4. `TST_RES_VULKAN`: Vulkan instance/device/runtime dependency.
5. `TST_RES_GLFW`: GLFW/windowing dependency.
6. `TST_RES_FILESYSTEM`: reads or writes files.
7. `TST_RES_ENV`: mutates process environment.
8. `TST_RES_VIDEO`: video encoder/backend dependency.
9. `TST_RES_LOG_CAPTURE`: relies on captured log records or expected-error scopes.
10. `TST_RES_GLOBAL_STATE`: uses unavoidable process-global state.

Recommended isolation modes:

1. `TST_ISOLATION_SERIAL`: default; safe only in the current serial runner.
2. `TST_ISOLATION_PROCESS`: safe to shard by subprocess, not by thread.
3. `TST_ISOLATION_THREAD_SAFE`: safe for in-process workers.
4. `TST_ISOLATION_EXCLUSIVE`: must run alone in the whole runner process.


## Migration Guidance

1. Do not bulk-edit all tests in one pass.
2. Keep `TEST_SIMPLE()` as a compatibility layer until most modules have migrated.
3. Migrate by module or semantic group, not by mechanical global replacement.
4. Start with CPU-only modules:
   - `common`,
   - `ds`,
   - `fileio`,
   - `math`,
   - selected `thread` tests if they do not depend on process-wide timing assumptions.
5. Delay broad migration of graphics-heavy modules until the metadata model stabilizes:
   - `vk`,
   - `vklite`,
   - `canvas`,
   - `scene/app-offscreen`,
   - `video`.
6. Replace fixed file paths with runner-assigned temporary paths where practical.
7. Mark environment-mutating tests explicitly and prefer subprocess isolation.
8. Keep GPU/GLFW tests serial by default until proven otherwise.


## Initial Milestone

The first implementation slice should deliver:

1. registry/run-context separation,
2. backward-compatible `TEST_SIMPLE()` and `TEST()` behavior,
3. explicit metadata fields in `TstItem` or a new descriptor struct,
4. module/group registration helpers,
5. `--list` and `--list-groups`,
6. one migrated pilot module, preferably `math`,
7. serial execution only,
8. passing `git diff --check`, `just build`, and `just test math` or `just test core`.

This establishes the architecture without destabilizing the active scene/DRP2/Vulkan validation
lanes.


## Non-Goals For The First Slice

1. Do not add thread-level parallel execution immediately.
2. Do not require every existing test to declare resources in the first pass.
3. Do not remove name-substring filtering yet.
4. Do not rewrite graphics fixtures just to fit the new metadata API.
5. Do not change test ordering for existing serial runs unless explicitly requested.
6. Do not introduce a second testing framework dependency.


## Open Questions

1. Should module/group state be stored as mutable registration scope on `TstSuite`, or should each
   new test registration pass an explicit descriptor?
2. Should process sharding live inside `dvztest`, or should it be a separate helper script that
   consumes `dvztest --list --json`?
3. Should expected-error scopes be supported in thread-safe tests through thread-local log context,
   or should all log-capture tests remain process-isolated?
4. Should CTest be taught about semantic groups as individual tests, or should CTest keep invoking
   component binaries only?
5. What is the desired result format for CI: JUnit XML, custom JSON, or both?
