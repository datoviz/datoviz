# Datoviz v0.4-dev Canvas Offscreen Refactor Plan

This document is the execution plan for adding first-class offscreen rendering to `canvas` while
preserving the current presentation stack behavior. It is designed for agent-driven implementation
in small, testable increments.

Primary objective:
1. Make offscreen rendering a first-class canvas mode, not a fallback side effect.
2. Support headless/offscreen execution without swapchain present dependency.
3. Keep output backends backend-agnostic (CPU readback, video encode, live image streaming).
4. Preserve deterministic synchronization and frame lifecycle semantics across modes.
5. Keep existing present-mode behavior stable and regression-safe.


## Progress tracking (live)

Legend:
1. `[x]` done
2. `[~]` in progress
3. `[ ]` not started

Current status:
1. Current milestone: `M1 - Mode plumbing and sink gating`
2. Current task: `OFFS-080 - Add canvas offscreen tests`
3. Last completed task: `OFFS-030 - Add offscreen frame sink backend for no-present execution`
4. Last updated: `2026-02-13`
5. Note: foundational mode plumbing is implemented; output/capture contracts are still pending.

Task board status:
1. `[x] OFFS-000` Baseline verification of canvas/swapchain coupling and headless gaps.
2. `[x] OFFS-005` Freeze offscreen mode API and runtime contracts.
3. `[x] OFFS-010` Add explicit canvas render mode plumbing (present vs offscreen).
4. `[x] OFFS-020` Make swapchain sink optional and mode-gated.
5. `[x] OFFS-030` Add offscreen frame sink backend for no-present execution.
6. `[ ] OFFS-040` Add transport-oriented live-image sink contract/API.
7. `[ ] OFFS-050` Wire CPU readback and video capture contracts to offscreen mode.
8. `[ ] OFFS-060` Implement deterministic frame lifecycle/state machine for offscreen mode.
9. `[ ] OFFS-070` Harden synchronization and handle-refresh semantics across mode changes.
10. `[~] OFFS-080` Add canvas offscreen tests (headless + capability-gated).
11. `[ ] OFFS-085` Add end-to-end distributed/live-image smoke validation hooks.
12. `[ ] OFFS-090` Cleanup dead paths and boundary violations.
13. `[ ] OFFS-100` Final validation gate.

Immediate next actions:
1. `M1 stabilization`:
   1. [ ] run full canvas/vklite tests and validate present-mode regression coverage.
   2. [ ] extend offscreen tests with explicit WAIT_SURFACE non-regression assertions.
   3. [ ] document current offscreen limitations (capture/video pending OFFS-050).


## Ground rules

1. Module boundaries are the source of truth:
   1. `window` owns native window/platform events and optional surface ownership.
   2. `vklite` owns presentation wrappers (`DvzSurface`, `DvzSwapchain`) only for present mode.
   3. `canvas` owns frame orchestration and render-mode selection.
   4. `stream` owns sink lifecycle and per-frame delivery semantics.
   5. `video` owns encoding behavior and capability gating.
2. Offscreen mode must not require a valid `VkSurfaceKHR` or swapchain acquire/present.
3. Present mode behavior must remain backwards-compatible with current tests.
4. Public API additions should be minimal, explicit, and documented.
5. Synchronization guarantees (`wait_value`, monotonicity, refresh ordering) remain mandatory.
6. Build and tests must stay green at each milestone.


## Current implementation snapshot (branch reality)

Snapshot date: `2026-02-13`.

1. `canvas` frame flow currently calls swapchain acquire then stream submit/present path.
2. Swapchain sink is attached by default and treated as mandatory for stream setup.
3. Headless/offscreen window backend exists in `window` but does not create platform surfaces.
4. `dvz_live_canvas` is currently GLFW-only and does not expose backend/mode selection.
5. Video capture supports external-handle and CPU-readback strategies but is integrated through
   current canvas stream lifecycle, which still assumes present-oriented frame acquisition.

Baseline gap summary:
1. No explicit first-class canvas mode for no-present rendering.
2. No primary offscreen sink in canvas stream pipeline.
3. No end-to-end canvas workflow that guarantees useful frame output without swapchain.


## Target architecture (for this phase)

1. Canvas render modes:
   1. `PRESENT` mode (current behavior): acquire -> draw -> submit -> present.
   2. `OFFSCREEN` mode (new behavior): draw -> submit -> output sinks; no swapchain present.
2. Stream sink model:
   1. Mode-gated mandatory sink:
      1. present mode: swapchain sink mandatory.
      2. offscreen mode: offscreen sink mandatory.
   2. Optional sinks in both modes: video, profiling/diagnostic, transport/live-image sink.
3. Output strategy:
   1. CPU readback APIs work in both modes.
   2. Video encode works in both modes with capability-gated capture paths.
   3. Live-image transport sink exposes frame metadata + bytes/handles to higher layers.
4. State model:
   1. Shared frame lifecycle invariants regardless of mode.
   2. Mode-specific transitions for acquire/present states.


## API and contract model

### Public API additions (proposed)

1. Add canvas render mode enum:
```c
typedef enum DvzCanvasRenderMode
{
    DVZ_CANVAS_RENDER_MODE_PRESENT = 0,
    DVZ_CANVAS_RENDER_MODE_OFFSCREEN = 1,
} DvzCanvasRenderMode;
```

2. Extend `DvzCanvasConfig` with explicit mode and offscreen extent policy:
```c
DvzCanvasRenderMode render_mode; // default: PRESENT
```

3. Optional helpers (if needed by tests/apps):
```c
DvzCanvasRenderMode dvz_canvas_render_mode(const DvzCanvas* canvas);
int dvz_canvas_set_render_mode(DvzCanvas* canvas, DvzCanvasRenderMode mode); // runtime switch optional
```

Public API semantics:
1. `PRESENT` mode requires a valid presentation-capable window/surface path.
2. `OFFSCREEN` mode must work with `DVZ_BACKEND_OFFSCREEN` and no native surface.
3. `dvz_canvas_frame()` and `dvz_canvas_submit()` are mode-agnostic entry points.
4. Capture APIs (`dvz_canvas_capture_rgba*`, `dvz_canvas_capture_png`) must remain valid in both modes.
5. Mode-specific unsupported operations must fail with explicit diagnostics.

Public API non-goals:
1. No renderer/scene API redesign in this plan.
2. No hard dependency on a specific transport protocol for live-image streaming.

### Internal integration contracts

1. Ownership/lifecycle:
   1. `canvas` owns offscreen render targets and their frame metadata.
   2. Swapchain objects are only created/used in `PRESENT` mode.
   3. Offscreen sink resources are only created/used in `OFFSCREEN` mode.
2. Synchronization:
   1. `canvas` remains authoritative for per-frame timeline `wait_value`.
   2. `wait_value` remains monotonic across both modes.
   3. Handle refresh ordering remains refresh-before-submit for all sinks.
3. Mode invariants:
   1. `PRESENT` mode: successful submit implies present attempt unless recoverable skip state.
   2. `OFFSCREEN` mode: successful submit never calls acquire/present.
4. Capture/live-image invariants:
   1. Offscreen frames are accessible via CPU readback and sink callbacks after submit.
   2. Live-image sink receives complete per-frame metadata (extent, format, frame id, wait value).


## State machine contract

Runtime states (mode-agnostic superset):
1. `UNINITIALIZED`
2. `READY`
3. `ACQUIRE_PENDING` (present mode only)
4. `DRAW_PENDING`
5. `PRESENT_PENDING` (present mode only)
6. `OUTPUT_PENDING` (offscreen mode only)
7. `WAIT_SURFACE` (present mode only)
8. `FATAL_DEVICE_LOST`

Transition rules:
1. `UNINITIALIZED -> READY` after successful mode-specific initialization.
2. `READY -> ACQUIRE_PENDING` at frame begin in `PRESENT` mode.
3. `READY -> DRAW_PENDING` at frame begin in `OFFSCREEN` mode.
4. `ACQUIRE_PENDING -> DRAW_PENDING` when acquire succeeds.
5. `ACQUIRE_PENDING -> WAIT_SURFACE` when surface/extent is not ready.
6. `DRAW_PENDING -> PRESENT_PENDING` after submit in `PRESENT` mode.
7. `DRAW_PENDING -> OUTPUT_PENDING` after submit in `OFFSCREEN` mode.
8. `PRESENT_PENDING -> READY` on successful present.
9. `OUTPUT_PENDING -> READY` after offscreen output sinks complete.
10. any state -> `FATAL_DEVICE_LOST` on unrecoverable device loss.


## Non-goals for this refactor phase

1. No DRP/WebGPU work.
2. No scene/client rewrite.
3. No transport protocol standardization beyond a generic sink contract.
4. No mandatory runtime mode switching if it introduces unacceptable lifecycle risk.


## Migration map (file/symbol level)

Primary files expected to change:
1. `include/datoviz/canvas.h` (config + optional helpers)
2. `src/canvas/canvas.c` (mode-aware frame/submit orchestration)
3. `src/canvas/canvas_stream.c` (mode-gated sink wiring)
4. `src/canvas/swapchain_sink.c` (strictly present-mode path)
5. `src/canvas/canvas_internal.h` (internal mode/state structs)
6. `src/canvas/tests/test_canvas.c` (offscreen coverage + regressions)
7. `testing/dvz_live_canvas.c` (backend/mode option support)
8. `testing/dvztest.c` (if new test entrypoints are added)

Potential new files:
1. `src/canvas/offscreen_sink.c`
2. `src/canvas/tests/test_canvas_offscreen.c` (or merged into current canvas tests)
3. `include/datoviz/canvas/offscreen.h` (only if public types are required)


## Milestones

## M0 - Baseline and contract freeze

### Goal
Freeze a minimal, explicit offscreen contract before implementation.

### Deliverables
1. `OFFS-000`: code-audited baseline notes in this plan.
2. `OFFS-005`: API and state-machine wording finalized.
3. Initial test matrix with clear expected pass/skip criteria.

### Exit criteria
1. Required API surface and invariants are frozen.
2. No ambiguity remains about mode behavior and ownership boundaries.


## M1 - Mode plumbing and sink gating

### Goal
Add explicit render-mode configuration and make sink setup mode-aware.

### Deliverables
1. `OFFS-010`: add mode field/defaulting and internal storage.
2. `OFFS-020`: swapchain sink attach path becomes present-mode only.
3. `OFFS-030`: offscreen sink backend created and attached in offscreen mode.

### Tests
1. Canvas creation/destruction in both modes.
2. `dvz_canvas_frame()/submit()` return semantics per mode.
3. No regressions in present-mode tests.

### Exit criteria
1. Offscreen mode runs frame loop without acquire/present dependency.
2. Present mode remains behaviorally unchanged.


## M2 - Output contracts (capture/video/live image)

### Goal
Make offscreen mode useful for real workflows (testing, encode, streaming).

### Deliverables
1. `OFFS-040`: generic live-image sink contract and minimal implementation.
2. `OFFS-050`: CPU readback + video sink integration validated in offscreen mode.

### Tests
1. Offscreen readback checksum/size sanity tests.
2. Offscreen video encode smoke tests with capability gating.
3. Live-image sink callback contract tests.

### Exit criteria
1. Offscreen mode can produce frames for capture and sinks without present.
2. Capability-based skips are explicit and deterministic.


## M3 - Synchronization and lifecycle hardening

### Goal
Guarantee deterministic sync behavior and safe handle refresh semantics.

### Deliverables
1. `OFFS-060`: offscreen state transitions implemented and enforced.
2. `OFFS-070`: monotonic wait-value + refresh-before-submit guarantees across both modes.

### Tests
1. Wait-value monotonicity in offscreen mode.
2. Refresh ordering after recreate/reconfigure where applicable.
3. Device-loss and fatal-transition behavior parity.

### Exit criteria
1. Sync contracts hold under stress/restart scenarios.
2. No stale-handle consumption in sinks.


## M4 - App-level UX and distributed workflow readiness

### Goal
Expose mode selection in tooling and validate headless operator workflows.

### Deliverables
1. `OFFS-085`: `dvz_live_canvas` supports backend/mode selection.
2. Documented runbook for headless and distributed live-image scenarios.

### Tests
1. `dvz_live_canvas --backend offscreen` smoke path.
2. Non-interactive scripted frame loop with screenshot/video/live-image sink.

### Exit criteria
1. Offscreen workflows are runnable without code changes.
2. Developer/test automation can rely on stable headless path.


## M5 - Cleanup and validation gate

### Goal
Finalize implementation quality and lock regression gates.

### Deliverables
1. `OFFS-090`: remove dead coupling logic and sharpen diagnostics.
2. `OFFS-100`: run final validation matrix and update plan status.

### Tests
1. `just build`
2. `direnv exec . just test canvas`
3. `direnv exec . just test stream`
4. `direnv exec . just test video`
5. targeted `dvz_live_canvas` smoke checks for both modes

### Exit criteria
1. Required filters pass or skip with explicit capability reasons.
2. Offscreen and present modes both satisfy contracts in this document.
3. Task board is fully complete.


## Agent task board

1. `OFFS-000` Baseline verification of canvas/swapchain coupling and headless gaps.
2. `OFFS-005` Freeze offscreen mode API and runtime contracts.
3. `OFFS-010` Add explicit canvas render mode plumbing (present vs offscreen).
4. `OFFS-020` Make swapchain sink optional and mode-gated.
5. `OFFS-030` Add offscreen frame sink backend for no-present execution.
6. `OFFS-040` Add transport-oriented live-image sink contract/API.
7. `OFFS-050` Wire CPU readback and video capture contracts to offscreen mode.
8. `OFFS-060` Implement deterministic frame lifecycle/state machine for offscreen mode.
9. `OFFS-070` Harden synchronization and handle-refresh semantics across mode changes.
10. `OFFS-080` Add canvas offscreen tests (headless + capability-gated).
11. `OFFS-085` Add end-to-end distributed/live-image smoke validation hooks.
12. `OFFS-090` Cleanup dead paths and boundary violations.
13. `OFFS-100` Final validation gate.


## Canonical execution order

1. M0 (`OFFS-000`, `OFFS-005`)
2. M1 (`OFFS-010`, `OFFS-020`, `OFFS-030`)
3. M2 (`OFFS-040`, `OFFS-050`)
4. M3 (`OFFS-060`, `OFFS-070`)
5. M4 (`OFFS-080`, `OFFS-085`)
6. M5 (`OFFS-090`, `OFFS-100`)


## Completion checklist

1. Canvas has explicit first-class render mode (`PRESENT` vs `OFFSCREEN`).
2. Offscreen mode frame loop does not require acquire/present.
3. Swapchain path remains isolated to present mode.
4. CPU capture, video capture, and live-image sink are validated in offscreen mode.
5. Synchronization and handle-refresh invariants are enforced across modes.
6. `dvz_live_canvas` supports practical headless/offscreen workflows.
7. Required build/tests pass with explicit capability-gated skips.
8. Plan status and code comments reflect final behavior.
