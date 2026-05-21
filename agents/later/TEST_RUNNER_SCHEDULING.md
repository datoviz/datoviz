# Test Runner Scheduling Follow-Up

> **Execution Status**
> - **Status:** `BACKLOG / OPTIONAL FOLLOW-UP NOTE`
> - **Updated on:** `2026-05-19`
> - **Purpose:** track optional runner scheduling, shared-fixture, skip-cleanup, and reporting
>   follow-ups after the stable process-sharding and shared-fixture baseline landed.


## Current State

This is not an active `soon/` execution lane. The stable runner modernization and low-risk
process-sharding baseline are complete; this file preserves optional backlog slices for future
runner work.

Durable records live in:

1. [`../done/TEST_RUNNER_MODERNIZATION.md`](../done/TEST_RUNNER_MODERNIZATION.md)
2. [`../done/APP_RESOURCE_INJECTION_PLAN.md`](../done/APP_RESOURCE_INJECTION_PLAN.md)

Use this file only for the remaining follow-up queue. Do not duplicate the serial runner
modernization history, app resource-injection API design, or long commit lists here.

The runner now has metadata-first case registration, per-test contexts, explicit skip reporting,
resource and isolation metadata, structured filters, JSON output, process sharding,
resource-aware shard policy, fixture setup timing, shard progress reporting, and shared GPU/DRP2
fixtures for selected low-risk lanes.

Serial execution remains the default. `--jobs N` is available for explicit process sharding. The
latest focused baseline recorded on `2026-05-19` passed the full scene runner serially and with
`--jobs 4`; keep `652bb2b2 Record stable test runner performance baseline` as the current
performance reference.


## Remaining Follow-Up Queue

### Shared App-Offscreen Expansion

Continue migrating only low-risk cases into shared app-offscreen or shared GPU fixture lanes.

Good candidates:

1. clear-color tests;
2. simple nonblank point, pixel, image, mesh, and sphere smokes;
3. single-frame render-and-capture cases with no failure injection;
4. cases that do not depend on first-app construction semantics.

Keep isolated:

1. app creation and destruction tests;
2. resize and recreate tests;
3. descriptor refresh and resource lifetime tests;
4. pick/probe steady-state tests until request queues are audited;
5. failure-path and device-lost tests;
6. GLFW, present, and external-surface tests;
7. video, filesystem, environment, and log-capture tests unless specifically audited.


### Fixture Availability And Skip Cleanup

Continue migrating ad-hoc graphics/runtime availability exits to explicit `tst_skip(ctx, reason)`
or skip predicates when tests are touched.

Setup code must either:

1. run skip predicates before allocation;
2. clean up before returning skipped;
3. rely on runner-owned fixtures whose lifecycle is independent of per-case teardown.

Fixture creation failure should mark the fixture unavailable and let each dependent test report a
clear skip reason.


### Resource Capacity Accounting

Resource flags are useful scheduling metadata, but capacity accounting remains coarse.

Possible next steps:

1. record per-resource capacities for CPU, GPU, GLFW, video, and exclusive/global resources;
2. let parent sharding choose shard policies from those capacities;
3. keep defaults conservative for Vulkan, GLFW, app, canvas, video, environment, and
   process-global tests.

Do not overload `TST_RES_GPU` to mean "shared GPU fixture". A test can need a GPU and still require
fresh per-case state.


### In-Process Thread Workers

Add in-process workers only after process sharding remains stable.

Rules:

1. only run `TST_ISOLATION_THREAD_SAFE` cases in thread workers;
2. each worker owns worker-scoped fixtures;
3. output must stay synchronized and deterministic;
4. log capture and expected-error scopes must remain per-context;
5. no Vulkan, GLFW, app, canvas, video, environment, or process-global tests should enter thread
   workers until they have explicit thread-safe contracts.


### Reporting Polish

Potential reporting follow-ups:

1. include fixture name and scope consistently in parent summaries;
2. include shard or worker index in remaining human-readable failure details;
3. preserve deterministic result ordering in all parent summaries;
4. keep slow-test and slow-group summaries based on child-reported elapsed time;
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

1. capture baseline timings for the relevant group before changing fixture ownership;
2. add or expand the shared lane and capture timings with the same command shape;
3. verify the original isolated lane still passes;
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
