# Test Runner Scheduling, Parallelization, And Fixtures

> **Execution Status**
> - **Status:** `SOON`
> - **Updated on:** `2026-05-19`
> - **Purpose:** define the next runner work after the completed serial modernization baseline:
>   process sharding, optional thread workers, resource-aware scheduling, and shared fixture reuse.

The serial C test-runner modernization is complete and recorded in
[../done/TEST_RUNNER_MODERNIZATION.md](../done/TEST_RUNNER_MODERNIZATION.md). This file is the
active design note for the remaining scheduling work.

The app API work needed to let tests borrow a worker GPU context is separate and lives in
[APP_RESOURCE_INJECTION_PLAN.md](APP_RESOURCE_INJECTION_PLAN.md).


## Current Baseline

The runner already has:

1. metadata-first `TstCase` registration and per-test `TstContext`,
2. per-case `setup`, `teardown`, `skip`, and `user_data` hooks,
3. resource and isolation metadata,
4. runner-visible `PASS`/`FAIL`/`SKIP` results with skip reasons,
5. structured module/group/case/tag/resource/isolation filters,
6. `--list`, `--list-groups`, `--json`, `--repeat`, `--shuffle`, and slow-test summaries,
7. focused component binaries such as `dvztest_common`, `dvztest_math`, `dvztest_core`,
   `dvztest_drp2`, `dvztest_scene`, `dvztest_vk`, `dvztest_canvas`, `dvztest_gui`,
   `dvztest_integration`, and aggregate `dvztest`.

The runner does not yet have:

1. suite-level or group-level fixture lifecycle hooks,
2. process sharding,
3. in-process parallel workers,
4. scheduler-owned fixture pools,
5. resource capacity accounting beyond metadata and filters.

Current `TST_ISOLATION_PROCESS` values are metadata only; the C runner does not fork per case.


## Design Direction

Shared fixtures and parallel execution must be designed together. Avoid hidden global fixtures that
tests touch implicitly. A global GPU context may improve a serial run today, but it becomes a
parallelization hazard as soon as multiple cases execute at once.

The scheduler should own all reusable resources:

1. selected tests declare resource and fixture requirements,
2. the scheduler assigns tests to processes or workers,
3. each process or worker owns its own fixture instances,
4. tests receive fixture handles through `TstContext` or explicit setup state,
5. mutable app, canvas, scene, and runtime state is never shared concurrently.

Default execution can stay serial. The design should still make fixture scope explicit from the
start so serial fixture reuse does not have to be unwound later.


## Fixture Metadata

Extend `TstCaseDesc` and `TstCase` with explicit fixture information rather than inferring reuse
from broad resource flags:

```c
typedef enum
{
    TST_FIXTURE_SCOPE_NONE = 0,
    TST_FIXTURE_SCOPE_CASE,
    TST_FIXTURE_SCOPE_WORKER,
    TST_FIXTURE_SCOPE_PROCESS,
    TST_FIXTURE_SCOPE_EXCLUSIVE,
} TstFixtureScope;
```

Possible per-case fields:

```c
const char* fixture;
TstFixtureScope fixture_scope;
```

The string name keeps the generic runner independent of Datoviz modules. Datoviz-specific fixture
creation belongs in test modules or runner adapters, not in `testing/testing.cpp`.


## Fixture Lifecycle

Add generic lifecycle hooks that can support serial execution now and parallel execution later:

```c
typedef void* (*TstFixtureCreate)(TstSuite* suite, uint32_t worker_index);
typedef void (*TstFixtureDestroy)(void* fixture);
```

The runner should expose a small registry:

```c
void tst_suite_register_fixture(
    TstSuite* suite, const char* name, TstFixtureScope scope,
    TstFixtureCreate create, TstFixtureDestroy destroy);
```

Serial execution is worker index `0`. Parallel execution creates one fixture instance per worker or
process as required by scope.

Fixture lookup should be explicit from the test context, for example:

```c
void* tst_context_fixture(TstContext* ctx, const char* name);
```

The runner must destroy worker/process fixtures after all assigned tests complete, including after
fail-fast termination.


## Resource Scheduling Model

Keep resource flags as descriptive metadata, but add capacity-aware scheduling separately.

Initial resource policy:

1. CPU-only and `TST_ISOLATION_THREAD_SAFE` tests may run concurrently in thread workers later.
2. GPU/Vulkan tests may run in parallel only through separate processes or worker-owned fixtures
   after the owning subsystem is proven compatible.
3. GLFW, video, environment-mutating, log-capture, and process-global tests remain serial or
   process-isolated unless a narrower contract is proven.
4. `TST_ISOLATION_EXCLUSIVE` always runs alone in its process or worker.
5. `TST_ISOLATION_PROCESS` should mean a real child process once process sharding is implemented.

Do not overload `TST_RES_GPU` to mean "shared GPU fixture". A test can need a GPU and still require
fresh per-case state.


## Process Sharding First

Add process-level sharding before any in-process worker model.

Plan:

1. Parent runner lists selected cases with stable IDs and metadata.
2. Parent partitions cases into child runs using exact filters.
3. Child runners remain serial and write JSON result files.
4. Parent collects JSON, preserves deterministic result ordering, and prints one aggregate summary.
5. Parent returns failure if any child failed or if a child crashed.

This gives immediate wall-clock gains while preserving crash isolation and avoiding thread-safety
assumptions in Vulkan, GLFW, video, loader, and environment paths.

CLI candidates:

```text
--jobs N
--shard-index I --shard-count N
--child-json path
--parent-json path
```

Keep exact case filters working so CI can still schedule groups or individual tests directly.


## Thread Workers Later

Add in-process workers only after process sharding is stable.

Rules:

1. Only run `TST_ISOLATION_THREAD_SAFE` cases in thread workers.
2. Each worker owns its worker-scoped fixtures.
3. The runner output path must be synchronized and deterministic.
4. Log capture and expected-error scopes must remain per-context.
5. No Vulkan, GLFW, app, canvas, video, environment, or process-global tests should enter thread
   workers until they have explicit thread-safe contracts.


## GPU Fixture Policy

The target performance issue is that `scene/app-offscreen` tests currently pay full app/Vulkan
setup per case. A focused run showed many simple cases in the 240-350 ms range, and repeated
capture variants in the 500-850 ms range. Much of this is resource setup and explicit GPU
synchronization, not CPU-side C computation.

Use a worker-owned GPU fixture:

```c
typedef struct DvzTestGpuFixture
{
    DvzGpuCtx* gpu_ctx;
    DvzWindowHost* window_host;
    bool available;
    const char* skip_reason;
} DvzTestGpuFixture;
```

Per-worker candidates:

1. `DvzGpuCtx`, because it owns the expensive Vulkan instance, device, and allocator.
2. `DvzWindowHost`, for offscreen-only tests if backend registration and polling remain
   worker-confined.

Per-case resources:

1. `DvzApp`,
2. `DvzDrp2Runtime` for app tests,
3. `DvzAppWindow`,
4. `DvzCanvas`,
5. `DvzScene`, `DvzFigure`, visuals, fields, controllers, requests, and captures.

Do not share one `DvzApp` across unrelated cases. It binds to a borrowed scene, registers scene
callbacks, owns a growing fixed window array, and carries mutable runtime/window/canvas state.

Do not share one `DvzCanvas` across unrelated cases. It owns stream state, sink registry, frame
pool, timeline semaphore, offscreen image/view/command buffer, and capture state.

Do not share one `DvzDrp2Runtime` across app tests initially. It has mutable object tables,
borrowed frame-target state, deferred destroys, and active command-buffer state. Runtime reuse can
be reconsidered for low-level DRP2 tests after `dvz_drp2_runtime_reset()` has dedicated coverage.


## App Offscreen Migration

After [APP_RESOURCE_INJECTION_PLAN.md](APP_RESOURCE_INJECTION_PLAN.md) lands, introduce a fast
offscreen lane that borrows worker resources:

```c
DvzAppResources resources = {
    .gpu_ctx = worker->gpu_ctx,
    .runtime = NULL,
    .window_host = worker->window_host,
};

DvzApp* app = dvz_app_with_resources(scene, NULL, &resources);
```

Create a new group first:

```text
scene/app-offscreen-shared
```

Good first candidates:

1. clear-color tests,
2. simple nonblank point/pixel/image/mesh/sphere smoke tests,
3. single-frame render-and-capture cases with no failure injection,
4. cases that do not depend on first-app construction semantics.

Keep isolated:

1. app creation/destruction tests,
2. resize/recreate tests,
3. descriptor refresh and resource lifetime tests,
4. pick/probe steady-state tests until request queues are audited,
5. failure-path and device-lost tests,
6. GLFW/present/external-surface tests,
7. video, filesystem, environment, and log-capture tests unless specifically audited.

The original isolated `app-offscreen` lane should remain as correctness coverage while the shared
lane proves performance and fixture safety.


## Skip Handling

Continue migrating ad-hoc graphics/runtime availability exits to explicit `tst_skip(ctx, reason)`
or skip predicates when tests are touched.

Important current caveat: if setup allocates resources and then calls `tst_skip()`, teardown may not
run because the skip reason is already set. Setup code must either:

1. run skip predicates before allocation,
2. clean up before returning skipped,
3. rely on runner-owned fixtures whose lifecycle is independent of per-case teardown.

Fixture creation failure should mark the fixture unavailable and let each dependent test report a
clear skip reason.


## JSON And Reporting

Process sharding and fixtures should preserve machine-readable output:

1. Include fixture name and scope in per-case JSON.
2. Include shard or worker index in per-case JSON when parallel execution is enabled.
3. Preserve deterministic result ordering in parent summaries.
4. Keep slow-test and slow-group summaries based on child-reported case elapsed time.
5. Consider reporting fixture setup time separately from test case time so setup amortization is
   visible.


## Validation And Benchmarks

For runner scheduling changes:

1. run `git diff --check`,
2. build every touched runner target,
3. smoke `--list`, `--list-groups`, resource filters, isolation filters, exact case filters, and
   `--json`,
4. run representative CPU passes and representative graphics-unavailable skip paths,
5. verify aggregate summaries match child JSON records when process sharding is involved.

For shared GPU fixture work:

1. capture baseline timings for `build/testing/dvztest_scene --group app-offscreen --slow 12`,
2. add the shared lane and capture timings with the same command shape,
3. verify the original isolated app-offscreen lane still passes,
4. run Vulkan validation smoke tests for app/canvas/runtime ownership changes,
5. inspect fixture setup time separately from per-case render/capture time.


## Guardrails

1. Keep `testing/testing.h` and `testing/testing.cpp` generic.
2. Keep Datoviz-specific Vulkan, GLFW, CUDA, video, shader-path, and log-adapter code in module
   tests or Datoviz runner adapters.
3. Do not introduce hidden global GPU fixtures.
4. Do not change subsystem behavior while migrating test metadata or scheduling.
5. Preserve deterministic terminal output and machine-readable JSON output.
6. Treat GPU resource sharing as an opt-in lane until enough coverage proves it safe.
