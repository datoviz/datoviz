# Test Runner Modernization Plan

> **Execution Status**
> - **Status:** `SCENE APP SKIP SLICE LANDED`
> - **Updated on:** `2026-05-18`
> - **Purpose:** document the current C test-runner limitations and a staged refactor path toward
>   explicit grouping, cleaner filtering, and safe future parallelism.


## Summary

The current C testing framework is useful and lightweight, but it is still organized around a
single flat list of test functions executed through one shared mutable `TstSuite`. That model keeps
the runner simple, but it makes semantic grouping, resource-aware scheduling, and parallel execution
hard to add without risky ad-hoc rules.

The testing API itself is not a compatibility boundary in the v0.4 refactor. We should use that
freedom to replace the legacy test-runner API with a metadata-first registry, an explicit per-case
run context, cleaner CLI filtering, and independently buildable component runners.

The recommended strategy is still not to immediately parallelize tests. First, make the serial
runner structurally ready for process scheduling: explicit modules, groups, cases, tags, resource
requirements, isolation constraints, stable result records, and clear terminal/JSON output. Serial
execution should remain the default until tests opt into stronger parallel-safety guarantees.

Implementation update on `2026-05-18`: the first serial runner slice has landed. The generic
framework now has a metadata-first case registry, per-test `TstContext`, context-owned log capture
and expected-error state, module/group/case filtering, `--list`, `--list-groups`, JSON result
output, per-case/module/group/suite timing, slow-test summaries, color controls, repeat/shuffle
options, and a Datoviz-specific log adapter installed by the Datoviz runners. Test functions have
been migrated to the new `TstContext*` / `const TstCase*` signature, CPU modules have explicit
groups, and independent CPU runner targets now exist for `common`, `ds`, `fileio`, `math`,
`thread`, and aggregate `core`.

Follow-up update on `2026-05-18`: the first registration-time Vulkan/vklite probes have been moved
to runner-time skip predicates. `vk` and `vklite` cases now register unconditionally with
`TST_RES_GPU | TST_RES_VULKAN` and process isolation metadata, so `--list`, `--list-groups`, and
CPU-only filters no longer initialize Vulkan. When the Vulkan runtime is unavailable, selected
Vulkan/vklite cases are reported as `SKIP` with a JSON `skip_reason` instead of being hidden behind
placeholder pass cases.

Remaining follow-up: some graphics, scene, app, GUI, and integration tests still contain in-test
ad-hoc skips that log a warning and return success. They should be migrated gradually to explicit
skip predicates or a context-level `tst_skip(ctx, reason)` helper so skip counts become accurate
across the full suite.

Second follow-up update on `2026-05-18`: the runner now exposes `tst_skip(ctx, reason)` for
availability checks that can only be resolved inside a test body. The video test slice is the first
migration: disabled NVENC, unavailable kvazaar/Vulkan setup, and no-backend offline/headless
conditions now report `SKIP` with JSON reasons, and video registrations carry explicit video,
filesystem, GPU, and Vulkan resource metadata where appropriate.

Third follow-up update on `2026-05-18`: the canvas slice now reports GLFW/offscreen capability
exits as explicit `SKIP` results instead of silent pass-style warning returns. Canvas registrations
also carry CPU, GPU, Vulkan, GLFW, filesystem, environment, video, log-capture, global-state, and
isolation metadata, so `dvztest_canvas --list` exposes the resource shape of each case. Focused
validation covered `dvztest_canvas` rebuilds, a CPU-only case, and representative Vulkan/GLFW
unavailable paths.

Fourth follow-up update on `2026-05-18`: the scene runtime slices now expose GPU/Vulkan skip
results and metadata for frame-plan emission, pick/probe GPU requests, scene-graph runtime smoke
tests, and app-offscreen shared Vulkan-runtime gates. Focused validation covered `dvztest_scene`
rebuilds, representative scene GPU cases reporting `SKIP` when Vulkan is unavailable, and `--list`
output showing GPU/Vulkan/process metadata for the migrated scene cases.

Fifth follow-up update on `2026-05-18`: the scene app-offscreen helper and direct setup skip
branches now report explicit `SKIP` results. Helper capture structs carry skip reasons back to the
runner context, and direct app, GLFW, surface, and offscreen window setup exits call `tst_skip()`
before cleanup. Focused validation covered `dvztest_scene` rebuilds and representative
app-offscreen cases reporting Vulkan-unavailable skips in this environment.

Two boundaries should stay clean:

1. `testing/testing.h` and `testing/testing.cpp` should remain a generic C/C++ test framework.
2. Datoviz-specific module registration, Vulkan/GLFW/CUDA/video skip logic, shader paths, and log
   adapters should live in Datoviz test runners and module test sources.


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

8. Test compilation is too coupled in broad targets.
   - The current global test-source glob makes it easy for one broken module's test file to break
     unrelated component test builds.
   - A developer working on component `X` should be able to build and run component `Y` tests
     without compiling `X` tests.

9. Terminal output is serviceable but not structured enough.
   - Results should show stable `module/group/case` identifiers, duration, skip/failure metadata,
     and a copy-pasteable rerun command.
   - Color and Unicode are useful locally, but must remain optional and line-oriented for CI logs.


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
TST_MODULE(suite, "scene");

TST_GROUP("frame-plan");
TST_CASE(test_frame_plan_static_render);
TST_CASE(test_frame_plan_graph_validation_missing_usage);

TST_GROUP("app-offscreen");
TST_CASE_EX(
    test_app_offscreen_has_nonblank_pixels,
    .resources = TST_RES_GPU | TST_RES_VULKAN,
    .isolation = TST_ISOLATION_PROCESS,
    .timeout_ms = 10000);
```

The exact macro names and descriptor types should be chosen during implementation. The stable design
goal is that tests are filterable and schedulable by explicit metadata instead of long function
names.

Preferred runtime shape:

```c
typedef struct TstCase TstCase;
typedef struct TstContext TstContext;
typedef int (*TstFunction)(TstContext* ctx, const TstCase* test);
```

`TstSuite` should hold registry state. `TstContext` should hold mutable run state for the current
test: log capture, expected-error scopes, unexpected-error state, captured logs, temporary paths,
user data, timing, and result details.


## Generic Versus Datoviz-Specific Boundary

The generic testing framework should own:

1. registry data structures,
2. module/group/case/tag metadata,
3. resource and isolation labels as generic scheduler metadata,
4. CLI parsing and selection,
5. serial scheduling and future process scheduling,
6. result collection,
7. terminal formatting,
8. JSON output,
9. per-test temporary directories,
10. timeout/repeat/shuffle mechanics,
11. generic log-capture and expected-error concepts through a callback adapter.

The generic framework should not know about:

1. Datoviz module names beyond strings supplied at registration time,
2. Vulkan, GLFW, CUDA, shader directories, video encoders, or app/scene runtime details,
3. Datoviz build options except through runner-provided metadata and skip predicates,
4. `_log.h` internals.

Datoviz runners and tests should own:

1. which Datoviz modules are registered in each binary,
2. Datoviz-specific skip predicates and optional backend checks,
3. resource metadata for individual tests,
4. the adapter from Datoviz logging to the generic framework's log-capture callback,
5. component runner composition and CMake target dependencies.

This implies replacing direct `log_set_intercept()` usage inside the generic framework with a small
log adapter interface installed by Datoviz runners.


## Refactor Strategy

### Phase 1: Clean Registry/Runtime Split

Break the testing API deliberately and split concepts directly:

1. Make `TstSuite` primarily a test registry after registration.
2. Introduce `TstContext` as the only mutable per-test run state for:
   - log capture,
   - expected-error state,
   - unexpected-error state,
   - captured logs,
   - per-test temporary paths,
   - result details.
3. Change test functions to receive `TstContext*` and `const TstCase*`.
4. Replace suite-based helper calls with context-based helper calls:
   - `tst_log_capture_begin(ctx)`,
   - `tst_log_capture_end(ctx)`,
   - `tst_expect_error_begin(ctx)`,
   - `tst_expect_error_end(ctx)`.
5. Replace legacy registration macros with metadata-first registration.

This phase should not introduce parallel execution. It may change test registration code and helper
call sites.


### Phase 2: First-Class Metadata API

Add explicit registration helpers and migrate CPU-only modules as the reference pattern.

Recommended first modules:

1. `common`,
2. `ds`,
3. `fileio`,
4. `math`,
5. selected `thread` tests.

Expected additions:

1. module/group scope helpers,
2. case descriptor registration,
3. resource/isolation enums,
4. result records that preserve module/group/name,
5. `--list` output showing the registry,
6. `--list-groups` output for semantic groups.

The old name-substring filter does not need to remain as a public API. A temporary internal alias is
acceptable during migration if it reduces churn, but new runner behavior should be structured.


### Phase 3: CLI Filtering Cleanup

Add structured filtering before adding parallel execution:

1. `--module <name>`
2. `--group <name>`
3. `--case <name>`
4. `--tag <tag>`
5. `--exclude-tag <tag>`
6. `--resource <resource>`
7. `--isolation <mode>`
8. `--list`
9. `--list-groups`
10. `--json <path>` for machine-readable results and CI integration
11. `--fail-fast`
12. `--repeat <count>`
13. `--shuffle --seed <seed>`


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


## Terminal Output

The runner should use restrained, line-oriented terminal formatting. Color is useful locally, but
the output must remain readable and parse-safe in CI logs.

Default compact output should look like:

```text
PASS  math/array/create                         0.18 ms
PASS  math/array/resize                         0.11 ms
FAIL  scene/frame-plan/missing-usage            1.42 ms
SKIP  canvas/glfw/present-recovery              requires GLFW
```

Failure output should include metadata:

```text
FAIL scene/frame-plan/missing-usage
  function   test_scene_frame_plan_missing_usage
  resources  cpu, log-capture
  isolation  serial
  elapsed    1.42 ms

  unexpected error log emitted during test
```

Formatting controls:

1. `--color auto|always|never`, respecting `NO_COLOR`.
2. `--unicode auto|always|never`.
3. `--compact` for one-line results.
4. `--verbose` for captured logs and full metadata.
5. Plain deterministic output for non-TTY streams and CI.
6. Summary grouped by module.
7. Final failure list with copy-pasteable rerun commands.

Avoid live progress spinners or TUI-style output. GPU/window tests can hang; the last emitted line
should always identify the currently running or most recently started test.


## Timing And Duration Reporting

Timing should be first-class runner data, not formatting-only output. The runner should measure and
store durations for every executed case and for every aggregate level derived from selected cases.

Required timing records:

1. Per case:
   - wall-clock start time,
   - wall-clock end time,
   - elapsed duration,
   - timeout limit when one is configured,
   - repeat index when `--repeat` is used.
2. Per group:
   - number of selected, passed, failed, and skipped cases,
   - summed elapsed duration for cases in the group,
   - wall-clock span for the group when cases are executed contiguously by the serial scheduler.
3. Per module:
   - number of selected, passed, failed, and skipped cases,
   - summed elapsed duration for cases in the module,
   - wall-clock span for the module when cases are executed contiguously by the serial scheduler.
4. Entire suite:
   - total selected, passed, failed, and skipped cases,
   - summed test-case duration,
   - full runner wall-clock duration, including setup, teardown, listing/filtering overhead, and
     scheduler overhead.

The distinction between summed case duration and full runner wall-clock duration matters. In the
serial runner they should usually be close, but they will diverge once process-level scheduling,
timeouts, fixture setup/teardown, JSON writing, or future parallel execution are added.

Terminal output should show per-case duration in compact mode and aggregate duration in summaries.
Verbose output should also show group and module timing breakdowns. JSON output should include raw
duration values in a stable unit, preferably nanoseconds or microseconds as integers, plus any
human-readable formatted strings only as optional convenience fields.

Slow-test diagnostics should be added before parallel execution:

1. `--slow <count>` prints the slowest selected cases.
2. `--slow-groups <count>` prints the slowest groups by summed case duration.
3. `--timeout <ms>` optionally sets a default timeout for cases without explicit `timeout_ms`.

Timing should use a monotonic clock for elapsed durations. Wall-clock timestamps may be included in
JSON for CI correlation, but pass/fail decisions and elapsed durations must not depend on system
clock changes.


## Independent Test Compilation

Focused component tests should be independently buildable. A broken test source in component `X`
should not prevent building and running tests for component `Y`.

Target shape:

1. `testing` builds the generic framework only.
2. `dvztest_common` compiles only common tests and common dependencies.
3. `dvztest_math` compiles only math tests and math dependencies.
4. `dvztest_drp2` compiles only DRP2 tests and DRP2 dependencies.
5. `dvztest_scene` compiles only scene tests and scene dependencies.
6. `dvztest_canvas` compiles only canvas/window/stream/video tests and graphics dependencies.
7. Aggregate runners such as `dvztest_core` and `dvztest` are convenience targets, not the only
   way to build tests.

The CMake implementation should avoid feeding one global `src/*/tests/*.c*` source list into every
runner. Prefer an explicit helper that defines one runner from a bounded source list and dependency
set, for example:

```cmake
dvz_add_test_runner(
    TARGET dvztest_math
    MODULE math
    SOURCES
        ${PROJECT_SOURCE_DIR}/src/math/tests/test_math.c
        ${PROJECT_SOURCE_DIR}/src/math/tests/test_array.c
        ${PROJECT_SOURCE_DIR}/src/math/tests/test_box.c
    LIBS
        datoviz_common
        datoviz_math
)
```

The developer workflow should support focused builds:

```bash
cmake --build build --target dvztest_math
./build/testing/dvztest_math --module math
```

Optional `just` helpers can wrap these targets later:

```bash
just build-test math
just test math
just test scene/frame-plan
```


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

1. Do not migrate all graphics-heavy tests in one pass.
2. Break the testing API intentionally; do not preserve legacy macros just for compatibility.
3. Migrate by module or semantic group, not by mechanical global replacement.
4. Start with CPU-only modules and independent component runners:
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
2. context-based test functions,
3. metadata-first registration macros,
4. explicit `TstCase` descriptor fields,
5. module/group/case filtering,
6. `--list` and `--list-groups`,
7. compact color-aware terminal output,
8. per-case, per-group, per-module, and full-suite timing records,
9. slow-test summary support for the selected serial run,
10. generic log adapter interface with Datoviz-specific installation outside the generic framework,
11. independently buildable CPU component runners,
12. migrated `common`, `ds`, `fileio`, `math`, and selected `thread` tests,
13. serial execution only,
14. passing `git diff --check`, focused component builds, and focused CPU test runs.

This establishes the architecture without destabilizing the active scene/DRP2/Vulkan validation
lanes.


## Non-Goals For The First Slice

1. Do not add thread-level parallel execution immediately.
2. Do not require every graphics test to declare perfect resource metadata in the first pass.
3. Do not preserve legacy test-runner macros as a public compatibility layer.
4. Do not rewrite graphics fixtures just to fit the new metadata API.
5. Do not change graphics test ordering until their metadata has been audited.
6. Do not introduce a second testing framework dependency.
7. Do not add a rich terminal UI or spinner-based progress display.


## Design Decisions

1. Use readable scoped registration macros over an explicit descriptor API.
   - `TST_MODULE(suite, "math")`, `TST_GROUP("array")`, and `TST_CASE(...)` should be the normal
     test-file style.
   - Internally, every macro should populate an explicit `TstCaseDesc`.
   - `TST_CASE_EX(...)` should allow per-case descriptor overrides.

2. Keep process sharding out of the first implementation slice.
   - First implement stable `--list --json` output and a way to run an exact selected case.
   - Add parent-process orchestration only after the JSON/listing contract is stable.
   - The child runner should remain simple and serial.

3. Treat log-capture and expected-error tests as not thread-safe initially.
   - Tests using these helpers should declare `TST_RES_LOG_CAPTURE`.
   - `TST_RES_LOG_CAPTURE` should imply serial or process isolation.
   - Thread-local log contexts can be revisited only if there is a strong later need.

4. Keep CTest at component-binary granularity initially.
   - CTest should invoke targets such as `dvztest_math`, `dvztest_scene`, and `dvztest_canvas`.
   - Semantic filtering should live in the runner itself.
   - Generated CTest entries for individual groups can be added later only if CI benefits.

5. Use custom JSON as the only machine-readable CI result format.
   - JSON should be the canonical result format for resources, isolation, skip reasons, seeds,
     repeat indices, elapsed time, and captured logs.
   - Do not add JUnit XML unless a future CI requirement appears.
   - Terminal output should remain separate from machine-readable output.

6. Make CPU runners mandatory when their modules are enabled and backend runners optional.
   - Mandatory CPU-side targets: `testing`, `dvztest_common`, `dvztest_ds`, `dvztest_fileio`,
     `dvztest_math`, `dvztest_thread`, and aggregate `dvztest_core`.
   - Optional feature/backend targets: `dvztest_vk`, `dvztest_vklite`, `dvztest_canvas`,
     `dvztest_drp2`, `dvztest_scene`, `dvztest_app`, `dvztest_video`, and `dvztest_gui`.
   - Aggregate `dvztest` should exist only when its dependencies are enabled and should not be
     required for focused CPU testing.

7. Start with built-in resource bit flags.
   - Use `uint64_t resources` for simple C filtering and scheduling.
   - Built-ins should cover CPU, GPU, Vulkan, GLFW, filesystem, environment mutation, video, log
     capture, and global state.
   - Optional string-valued labels such as `cuda`, `nvenc`, `validation-layers`, or
     `display-server` can be added later if bit flags become too coarse.
