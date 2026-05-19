# Test Runner Scheduling Follow-Up

> **Execution Status**
> - **Status:** `SOON`
> - **Updated on:** `2026-05-19`
> - **Purpose:** track runner work that remains after the completed serial modernization baseline.

The serial C test-runner modernization is complete and recorded in
[../done/TEST_RUNNER_MODERNIZATION.md](../done/TEST_RUNNER_MODERNIZATION.md). Keep this file short:
it is only for future scheduling, CI, and residual skip cleanup.


## Current Baseline

The runner already has:

1. metadata-first `TstCase` registration and per-test `TstContext`,
2. resource and isolation metadata,
3. runner-visible `PASS`/`FAIL`/`SKIP` results with skip reasons,
4. structured module/group/case/tag/resource/isolation filters,
5. `--list`, `--list-groups`, `--json`, `--repeat`, `--shuffle`, and slow-test summaries,
6. focused component binaries such as `dvztest_common`, `dvztest_math`, `dvztest_core`,
   `dvztest_drp2`, `dvztest_scene`, `dvztest_vk`, `dvztest_canvas`, `dvztest_gui`,
   `dvztest_integration`, and aggregate `dvztest`.


## Open Work

1. Add process-level sharding before any in-process worker model.
2. Define a parent-runner or script that lists selected cases, launches child runner processes with
   exact filters, collects JSON results, and prints one deterministic summary.
3. Keep each child process serial; use process isolation for crashes, environment mutation, GLFW,
   Vulkan loader state, fixed global hooks, and video backends.
4. Add optional thread workers only for cases explicitly marked `TST_ISOLATION_THREAD_SAFE`.
5. Continue migrating remaining ad-hoc graphics/runtime availability exits to explicit
   `tst_skip(ctx, reason)` or skip predicates when the owning tests are touched.
6. Decide whether CI should stay at component-binary granularity or add generated group/case jobs
   only where wall-clock time justifies the extra scheduling surface.


## Guardrails

1. Keep `testing/testing.h` and `testing/testing.cpp` generic; Datoviz-specific Vulkan, GLFW,
   CUDA, video, shader-path, and log-adapter code belongs in module tests or Datoviz runners.
2. Do not change subsystem behavior while migrating test metadata or scheduling.
3. Preserve deterministic terminal output and machine-readable JSON output.
4. Keep GPU, GLFW, video, environment-mutating, log-capture, and process-global tests serial or
   process-isolated unless their narrower contract is proven.


## Validation

For runner scheduling changes:

1. run `git diff --check`,
2. build every touched runner target,
3. smoke `--list`, `--list-groups`, resource filters, exact case filters, and `--json`,
4. run representative CPU passes and representative graphics-unavailable skip paths,
5. verify aggregate summaries match child JSON records when process sharding is involved.
