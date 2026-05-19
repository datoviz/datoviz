# Test Runner Scheduling, Parallelization, And Fixtures

> **Execution Status**
> - **Status:** `PARTIALLY IMPLEMENTED; FOLLOW-UP QUEUE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** track remaining runner scheduling and shared-fixture work after process sharding,
>   fixture lifecycle, resource-aware shard policy, fixture timing, and shared app-offscreen
>   coverage landed.


## Durable Records

The serial C test-runner modernization is complete and recorded in
[../../done/TEST_RUNNER_MODERNIZATION.md](../../done/TEST_RUNNER_MODERNIZATION.md).

The app API work needed to let tests borrow a worker GPU context is complete and recorded in
[../../done/APP_RESOURCE_INJECTION_PLAN.md](../../done/APP_RESOURCE_INJECTION_PLAN.md).


## Implemented Baseline

Implemented runner/scheduler work includes:

1. metadata-first `TstCase` registration and per-test `TstContext`;
2. per-case `setup`, `teardown`, `skip`, and `user_data` hooks;
3. resource and isolation metadata;
4. structured module/group/case/tag/resource/isolation filters;
5. `--list`, `--list-groups`, `--json`, `--repeat`, `--shuffle`, and slow-test summaries;
6. generic fixture lifecycle APIs and explicit fixture lookup from `TstContext`;
7. process sharding with parent/child JSON aggregation;
8. resource-aware shard policies for parallel-safe and serial phases;
9. progress reporting for parallel shards;
10. process-isolated test replay for `TST_ISOLATION_PROCESS`;
11. fixture setup timing in reports and JSON;
12. shared app-offscreen coverage using resource-aware app construction.

Relevant implementation commits include:

1. `668515be` — added generic test fixture lifecycle;
2. `a69f9802` — added process sharding to the test runner;
3. `9abbb2ba` — added resource-aware test sharding policy;
4. `defaf837` — reported test fixture setup time;
5. `41d05e25` — added shared GPU fixture app offscreen tests;
6. `72334b30` — added scheduler test probe;
7. `3f9ab67d` — covered test scheduler sharding policy;
8. `81db06c9` — ran process-isolated tests in child processes;
9. `fc562771` and `8593600e` — expanded shared app offscreen depth/volume tests;
10. `9b03d86e` and `8d97c9d5` — shard progress and replay reporting hardening;
11. `ecbfac35` — refactored test runner scheduling helpers.


## Remaining Work

Keep this file for follow-ups that are not yet worth promoting into a permanent testing spec.

### 1. Shared App-Offscreen Expansion

Continue migrating only low-risk cases into the shared app-offscreen lane.

Good candidates:

1. clear-color tests;
2. simple nonblank point/pixel/image/mesh/sphere smokes;
3. single-frame render-and-capture cases with no failure injection;
4. cases that do not depend on first-app construction semantics.

Keep isolated:

1. app creation/destruction tests;
2. resize/recreate tests;
3. descriptor refresh and resource lifetime tests;
4. pick/probe steady-state tests until request queues are audited;
5. failure-path and device-lost tests;
6. GLFW/present/external-surface tests;
7. video, filesystem, environment, and log-capture tests unless specifically audited.


### 2. Fixture Availability And Skip Cleanup

Continue migrating ad-hoc graphics/runtime availability exits to explicit `tst_skip(ctx, reason)`
or skip predicates when tests are touched.

Setup code must either:

1. run skip predicates before allocation;
2. clean up before returning skipped;
3. rely on runner-owned fixtures whose lifecycle is independent of per-case teardown.

Fixture creation failure should mark the fixture unavailable and let each dependent test report a
clear skip reason.


### 3. Resource Capacity Accounting

Resource flags are now useful scheduling metadata, but capacity accounting remains coarse.

Possible next step:

1. record per-resource capacities for CPU, GPU, GLFW, video, and exclusive/global resources;
2. let parent sharding choose shard policies from those capacities;
3. keep default behavior conservative for Vulkan, GLFW, app, canvas, video, environment, and
   process-global tests.

Do not overload `TST_RES_GPU` to mean "shared GPU fixture". A test can need a GPU and still require
fresh per-case state.


### 4. In-Process Thread Workers

Add in-process workers only after process sharding remains stable.

Rules:

1. only run `TST_ISOLATION_THREAD_SAFE` cases in thread workers;
2. each worker owns worker-scoped fixtures;
3. output must stay synchronized and deterministic;
4. log capture and expected-error scopes must remain per-context;
5. no Vulkan, GLFW, app, canvas, video, environment, or process-global tests should enter thread
   workers until they have explicit thread-safe contracts.


### 5. Reporting Polish

Potential reporting follow-ups:

1. include fixture name/scope consistently in parent summaries;
2. include shard or worker index in any remaining human-readable failure details;
3. preserve deterministic result ordering in all parent summaries;
4. keep slow-test and slow-group summaries based on child-reported case elapsed time;
5. make fixture setup-time amortization easy to compare across isolated and shared lanes.


## Validation

For runner scheduling changes:

1. run `git diff --check`;
2. build every touched runner target;
3. smoke `--list`, `--list-groups`, resource filters, isolation filters, exact case filters, and
   `--json`;
4. run representative CPU passes and representative graphics-unavailable skip paths;
5. verify aggregate summaries match child JSON records when process sharding is involved.

For shared GPU fixture work:

1. capture baseline timings for `build/testing/dvztest_scene --group app-offscreen --slow 12`;
2. add or expand the shared lane and capture timings with the same command shape;
3. verify the original isolated app-offscreen lane still passes;
4. run Vulkan validation smoke tests for app/canvas/runtime ownership changes;
5. inspect fixture setup time separately from per-case render/capture time.


## Guardrails

1. Keep `testing/testing.h` and `testing/testing.cpp` generic.
2. Keep Datoviz-specific Vulkan, GLFW, CUDA, video, shader-path, and log-adapter code in module
   tests or Datoviz runner adapters.
3. Do not introduce hidden global GPU fixtures.
4. Do not change subsystem behavior while migrating test metadata or scheduling.
5. Preserve deterministic terminal output and machine-readable JSON output.
6. Treat GPU resource sharing as an opt-in lane until enough coverage proves it safe.
