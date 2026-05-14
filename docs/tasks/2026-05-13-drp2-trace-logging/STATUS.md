# Status

- Task: make live DRP2 stream logging useful from the app loop.
- Started: 2026-05-13
- State: completed implementation, focused revalidation recommended after later app/pick changes

## Work Completed

Recent commits implemented the plan in `PLAN.md`:

1. fixed the trace test build issue,
2. added semantic trace fingerprints and snapshots that ignore transient frame/pass ids,
3. made normal trace output changed/unchanged aware,
4. expanded full trace output into a human-readable command dump,
5. combined DRP2 trace status with FPS/status output in the app,
6. removed temporary pick-hover example tracing after debugging was complete,
7. colorized trace output by command category and enabled colors by default,
8. aligned Datoviz logging color controls with runtime `NO_COLOR` / `DVZ_LOG_COLOR` policy.

Relevant commits:

1. `6cc973d5` — stable fingerprints for live trace deduplication,
2. `9aedb7a6` — semantic live trace deduplication,
3. `325a3999` and `5eb1063b` — compact/full DRP2 trace output,
4. `ffc9cdbd` and `b14055c8` — trace output cleanup,
5. `f55d6dce` — combined FPS and DRP2 status output,
6. `77504953` — removed temporary pick-hover tracing,
7. `e38de1fb` and `f5cdbce2` — trace colorization and default-on color,
8. `1e62678d` — runtime logger color controls.

## Validation

Focused app trace tests were added in `src/app/tests/test_app.c`.

This status refresh did not re-run the app tests. Before changing trace/status code again, run:

```bash
just build
just test app
git diff --check
```

## Current State

The app trace lane is no longer just a plan. It is implemented and should be treated as active app
infrastructure. Future work should keep trace fingerprints semantic rather than raw-struct based, and
normal live output prints full command streams for changed frames only; unchanged repeated frames
should stay compact enough for interactive debugging.

Trace controls:

1. `DVZ_DRP2_TRACE=normal` enables changed-frame stream output.
2. `DVZ_DRP2_TRACE=full` enables the verbose command dump.
3. Trace colors are enabled by default.
4. `NO_COLOR=1` disables Datoviz terminal color broadly.
5. `DVZ_DRP2_TRACE_COLOR=0` disables only DRP2 trace colors.
6. `DVZ_DRP2_TRACE_COLOR=1` forces DRP2 trace colors.
7. `DVZ_LOG_COLOR=0` / `DVZ_LOG_COLOR=1` controls general logger colors.
