# Test Runner Scheduling

Status: optional test-runner roadmap.

The stable baseline is metadata-first case registration, explicit skips, structured filters, JSON
output, process sharding, resource-aware shard policy, fixture setup timing, and shared fixtures for
selected low-risk lanes. Serial execution remains the default; `--jobs N` is explicit.


## Shared Fixture Expansion

Migrate only low-risk cases into shared app/offscreen or shared GPU fixtures:

1. clear-color tests;
2. simple nonblank point, pixel, image, mesh, and sphere smokes;
3. single-frame render-and-capture cases;
4. cases independent of first-app construction semantics.

Keep isolated until audited:

1. app creation and destruction;
2. resize and recreate;
3. descriptor refresh and resource lifetime;
4. query/readback steady-state paths;
5. failure-path and device-lost tests;
6. GLFW, presentation, external surfaces, video, filesystem, environment, and log-capture tests.


## Skip And Fixture Rules

Replace ad-hoc availability exits with explicit skip predicates or `tst_skip(ctx, reason)` when
tests are touched.

Setup code must:

1. run skip predicates before allocation;
2. clean up before returning skipped; or
3. rely on runner-owned fixtures whose lifecycle is independent of per-case teardown.


## Future Scheduling Work

1. record resource capacities for CPU, GPU, GLFW, video, and exclusive/global resources;
2. keep `TST_RES_GPU` distinct from shared GPU fixture eligibility;
3. add in-process workers only for explicitly `TST_ISOLATION_THREAD_SAFE` tests;
4. keep Vulkan, GLFW, app, canvas, video, environment, and process-global tests out of thread
   workers until they have explicit contracts;
5. preserve deterministic output and JSON summaries;
6. include fixture scope, shard/worker index, slow groups, and setup-time amortization in reports.


## Validation

Runner scheduling changes should cover:

1. `git diff --check`;
2. build every touched runner target;
3. `--list`, `--list-groups`, resource filters, isolation filters, exact filters, and `--json`;
4. representative CPU passes and graphics-unavailable skip paths;
5. aggregate summaries matching child JSON records when sharding is involved.

Shared GPU fixture work should also capture timing before/after, keep the original isolated lane
green, and run Vulkan validation smoke when ownership changes.
