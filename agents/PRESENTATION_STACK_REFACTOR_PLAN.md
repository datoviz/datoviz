# Datoviz v0.4-dev Presentation Stack Refactor Plan

This document is the execution plan for completing the presentation stack refactor before broader
renderer work. It is designed for agent-driven implementation in small, testable increments.

Primary objective:
1. Move Vulkan surface and swapchain mechanics out of `canvas` into `vklite`.
2. Keep `window` responsible for platform surface lifecycle.
3. Keep `canvas` responsible for frame orchestration, stream integration, and draw callback flow.
4. Preserve and formalize video synchronization for both live screencast and offline/headless
   encoding paths.


## Progress tracking (live)

Legend:
1. `[x]` done
2. `[~]` in progress
3. `[ ]` not started

Current status:
1. Current milestone: `M5 - Final validation gate`
2. Current task: `PRES-100 - Final validation gate`
3. Last completed task:
   `PRES-100 - Final validation gate`
4. Last updated: `2026-02-12`
5. Note: M2 migration is complete in code (canvas now uses `vklite` wrappers only); follow-up
   hardening/tests landed for fail-fast slot init, GLFW present recovery, handle refresh order,
   queue-submit error propagation, startup-time timeline-handle wiring for video sinks, and
   deterministic wait-handle fallback on export failure with instance-scoped test controls; recreate
   update-path fallback coverage landed in canvas tests. M5 validation now passes with
   `just build`, `direnv exec . just test canvas`, `direnv exec . just test vklite`, and targeted
   video checks (`test_video_offline_headless_encode`, `test_video_kvazaar`).

Task board status:
1. `[x] PRES-000` Baseline verification of current raw Vulkan call sites and handle flow.
2. `[x] PRES-005` Freeze API signatures, return semantics, and state-machine mapping in headers/docs.
3. `[x] PRES-010` Add public vklite presentation headers and exports.
4. `[x] PRES-020` Implement `vklite` surface wrapper.
5. `[x] PRES-030` Implement `vklite` swapchain wrapper.
6. `[x] PRES-080`
   Add vklite presentation-layer tests (surface/swapchain/recreate + API contract guards).
7. `[x] PRES-040` Integrate window-canvas-vklite surface handoff and lifecycle.
8. `[x] PRES-050` Migrate canvas presentation path to vklite API.
9. `[x] PRES-082` Add end-to-end present recovery test (OUT_OF_DATE/SUBOPTIMAL -> recreate -> resume).
10. `[x] PRES-055` Implement deterministic handle-refresh and sink-restart policy.
11. `[x] PRES-060` Harden resize/out-of-date/zero-extent state machine.
12. `[x] PRES-070` Finalize queue/semaphore/fence ownership rules.
13. `[x] PRES-075` Finalize video synchronization and sink ordering contract.
14. `[x] PRES-085` Add capture-mode validation tests (live + offline/headless).
15. `[x] PRES-090` Cleanup dead code and boundary violations.
16. `[x] PRES-100` Final validation gate.

Immediate next actions:
1. `Presentation-stack plan complete for this phase`:
   1. [x] M3 synchronization contract hardening complete (`PRES-075`).
   2. [x] M4 capture-mode validation complete (`PRES-085`).
   3. [x] M5 cleanup/final validation complete (`PRES-090`, `PRES-100`).


## Ground rules

1. Module boundaries are the source of truth:
   1. `window` owns native window and native surface creation/destruction.
   2. `vklite` owns surface capability queries and swapchain lifecycle.
   3. `canvas` owns frame orchestration and stream sink wiring.
2. `canvas` must not directly call raw Vulkan swapchain/surface APIs once migration is complete.
3. Public API changes must be minimal and stable; behavioral contracts must be explicit.
4. Video synchronization is first-class, not optional.
5. Live desktop capture and offline/headless encoding are two supported modes and must both pass.
6. Build and tests must stay green at each milestone.


## Current implementation snapshot (branch reality)

Snapshot date: `2026-02-11` (post-M2 verification + M3 partial hardening).

1. Active modules in `src/CMakeLists.txt`: `input`, `window`, `canvas`, `stream`, `video`, `vk`,
   `vklite`.
2. Raw Vulkan surface/swapchain call sites now live in `src/vklite/surface.c` and
   `src/vklite/swapchain.c`; canvas no longer calls those entrypoints directly.
3. `include/datoviz/vklite/surface.h` and `include/datoviz/vklite/swapchain.h` now exist and
   freeze the public presentation API surface for M1.
4. Canvas uses `DvzSurface`/`DvzSwapchain` wrappers for create/recreate/acquire/present and exports
   frame handles to stream/video sinks via `DvzStreamFrame`.
5. Canvas tests now include:
   1. fail-fast slot initialization rollback (`test_canvas_swapchain_failfast_slot_init`)
   2. GLFW out-of-date recovery (`test_canvas_glfw_present_recovery`)
   3. handle refresh ordering contract (`test_canvas_handle_refresh_order`)
   4. wait-value propagation (`test_canvas_video_wait_value_propagation`)
   5. first-start wait-handle readiness (`test_canvas_video_wait_handle_ready_on_first_start`)
   6. wait-handle export fallback (`test_canvas_video_wait_handle_export_fallback`)
   7. real video sink startup/submit integration (`test_canvas_video_sink_start_submit_integration`)
   8. post-recreate refresh + wait continuity (`test_canvas_video_handle_refresh_after_recreate`)
   9. recreate/update-path wait-handle export fallback
      (`test_canvas_video_wait_handle_export_fallback_after_recreate`)
   10. device-lost fatal transition (`test_canvas_device_lost_fatal_transition`)
6. Video tests now include an explicit backend-neutral offline/headless capture-mode validation
   entrypoint (`test_video_offline_headless_encode`) plus backend-specific tests (`test_video_kvazaar`,
   `test_video_nvenc`).
7. Test runner now registers `video` tests when either CUDA or kvazaar backends are enabled.
8. Current environment status: offline/headless entrypoint executes quickly and backend-specific
   tests pass or skip explicitly by capability.
9. `vklite` presentation tests include resolved recreate-state coverage
   (`test_vklite_swapchain_recreate_resolved_state`).
10. Frame pool release closes lingering exported wait semaphore FDs on Unix.

### PRES-000 baseline verification (code-audited)

1. Verified raw presentation call sites in `src/canvas/swapchain_sink.c`:
   `vkGetPhysicalDeviceSurfaceCapabilitiesKHR`, `vkGetPhysicalDeviceSurfaceFormatsKHR`,
   `vkGetPhysicalDeviceSurfacePresentModesKHR`, `vkCreateSwapchainKHR`,
   `vkGetSwapchainImagesKHR`, `vkAcquireNextImageKHR`, `vkQueuePresentKHR`,
   `vkDestroySwapchainKHR`.
2. Verified native surface destroy remains in window backend:
   `src/window/backend_glfw.c` calls `vkDestroySurfaceKHR`, which matches target ownership.
3. Verified stream/video wait-value path is active:
   `dvz_canvas_stream_submit()` forwards `wait_value` to `dvz_stream_submit()`,
   then `src/video/video_sink.c` forwards it to encoder backends via
   `dvz_video_encoder_submit(wait_value)`.
4. Verified handle export path currently originates in canvas presentation code:
   `src/canvas/swapchain_sink.c` writes `frame->memory_fd` and exports `frame->wait_semaphore_fd`
   from the canvas timeline semaphore.
5. Result: current branch still matches pre-migration architecture; M2 migration must remove the
   raw swapchain/surface Vulkan call sites from canvas while preserving stream/video semantics.

### Post-M2/M3 status delta (code-audited)

1. `PRES-040`/`PRES-050` completed: canvas swapchain flow is wrapper-based (`dvz_surface_*`,
   `dvz_swapchain_*`) with no direct surface/swapchain Vulkan calls in `src/canvas/*`.
2. `PRES-082` completed: GLFW recovery test validates out-of-date -> recreate -> resume flow.
3. `PRES-055` completed: deterministic handle-refresh policy is in place and tested.
4. `PRES-060` completed: canvas swapchain path now uses an explicit runtime-state model and enforces
   deterministic `DEVICE_LOST -> FATAL_DEVICE_LOST` transition handling.
5. `PRES-070` completed: queue/semaphore/fence ownership is now enforced with explicit submit-failure
   handling (`vkQueueSubmit2` result propagation into canvas runtime transitions, including
   `DEVICE_LOST` fatal handling).
6. `PRES-075` completed: timeline wait-semaphore handle export is prepared before stream
   start/update (instead of post-present), startup/recreate ordering comments are in place, and
   first-start + forced-export-failure coverage now includes both probe-based and real video sink
   startup/submit integration tests, including recreate/update fallback closure.
7. `PRES-085` completed: capture-mode validation is split into backend-neutral offline/headless
   contract coverage and backend-specific encoder paths, with explicit capability-based skip reasons.
8. `PRES-090` completed: dead/unused swapchain sink state was removed, slot setup and
   submit/present/acquire status handling were extracted into dedicated helpers, wrapper init
   failure paths now tear down partial state, recreate preflight/config/status flow was split into
   dedicated helpers, cleanup internals now use focused slot-state/runtime reset helpers, acquire
   and submit dispatch now use dedicated helper paths, acquire slot selection/status mapping and
   present dispatch/preflight are split into focused helpers, and test controls are now
   instance-scoped.
9. `PRES-100` completed: final validation gate is green for build, canvas, vklite, and targeted
   video tests in the current environment.


## Target architecture (for this phase)

1. `window` layer:
   1. Creates native window + `VkSurfaceKHR` where applicable (GLFW path).
   2. Emits resize/scale/input events to router.
2. `vklite` presentation layer:
   1. `DvzSurface` wraps native surface handle and queried capabilities/formats/modes.
   2. `DvzSwapchain` wraps create/recreate/acquire/present and swapchain image views.
3. `canvas` orchestration layer:
   1. Drives acquire, draw callback, stream submit, and presentation timing.
   2. Performs offscreen render-target management and copy/blit policy.
4. `stream` sink pipeline:
   1. Retains sink registry and sink lifecycle.
   2. Maintains deterministic submission semantics for presentation and video sinks.
5. `video` layer:
   1. Consumes frame memory/sync handles.
   2. Waits on timeline values where supported; uses explicit fallback behavior otherwise.


## API and contract model

### Public API (minimal additions)

Finalized public headers:
1. `include/datoviz/vklite/surface.h`
2. `include/datoviz/vklite/swapchain.h`

Finalized `vklite.h` update:
1. Include the two headers above from `include/datoviz/vklite.h`.

Finalized public functions:
1. `dvz_surface_init()`
2. `dvz_surface_wrap_native()`
3. `dvz_surface_refresh()`
4. `dvz_surface_destroy()`
5. `dvz_swapchain_init()`
6. `dvz_swapchain_device()`
7. `dvz_swapchain_config()`
8. `dvz_swapchain_recreate()`
9. `dvz_swapchain_acquire()`
10. `dvz_swapchain_present()`
11. `dvz_swapchain_destroy()`

Finalized public signatures (implemented in M1):
```c
typedef enum DvzPresentStatus
{
    DVZ_PRESENT_STATUS_OK = 0,
    DVZ_PRESENT_STATUS_RECREATE,
    DVZ_PRESENT_STATUS_SKIP_ZERO_EXTENT,
    DVZ_PRESENT_STATUS_DEVICE_LOST,
    DVZ_PRESENT_STATUS_ERROR,
} DvzPresentStatus;

bool dvz_surface_init(DvzSurface* surface, DvzGpu* gpu, uint32_t queue_family);
bool dvz_surface_wrap_native(DvzSurface* surface, VkSurfaceKHR surface_khr, DvzWindow* window);
bool dvz_surface_refresh(DvzSurface* surface);
void dvz_surface_destroy(DvzSurface* surface);

bool dvz_swapchain_init(DvzSwapchain* swapchain, DvzGpu* gpu, DvzSurface* surface);
bool dvz_swapchain_device(DvzSwapchain* swapchain, VkDevice device);
bool dvz_swapchain_config(DvzSwapchain* swapchain, DvzSwapchainConfig config);
DvzPresentStatus dvz_swapchain_recreate(DvzSwapchain* swapchain, uvec2 size);
DvzPresentStatus dvz_swapchain_acquire(
    DvzSwapchain* swapchain, VkSemaphore image_available, uint64_t timeout_ns, uint32_t* image_idx);
DvzPresentStatus dvz_swapchain_present(
    DvzSwapchain* swapchain, VkQueue present_queue, uint32_t image_idx, VkSemaphore render_finished);
void dvz_swapchain_destroy(DvzSwapchain* swapchain);
```

API semantics and ownership:
1. `dvz_surface_wrap_native()` never creates or destroys `VkSurfaceKHR`; ownership remains in `window`.
2. `dvz_surface_destroy()` only clears cached capabilities/formats/modes; it must never destroy native surface.
3. `dvz_swapchain_destroy()` always destroys swapchain and swapchain image views owned by `vklite`.
4. `dvz_swapchain_acquire()` and `dvz_swapchain_present()` never allocate heap memory in steady-state.
5. `DVZ_PRESENT_STATUS_RECREATE` is the only recoverable resize/out-of-date signal returned to canvas.
6. `DVZ_PRESENT_STATUS_SKIP_ZERO_EXTENT` is returned when window extent is zero and frame must be skipped.
7. Any failure path must emit a diagnostic with object id, frame index, and Vulkan result code.

Public API non-goals:
1. No new public video API is required for this phase unless a concrete blocker appears.
2. Video synchronization specifics are internal integration contracts, validated by tests.

### Internal integration contracts (must be explicit in code comments/tests)

1. Ownership/lifecycle contract:
   1. `window` owns native surface lifetime.
   2. `vklite` owns swapchain and swapchain image views.
   3. `canvas` owns offscreen render targets and stream frame metadata.
2. Synchronization contract:
   1. `canvas` is authoritative for per-frame timeline `wait_value`.
   2. Stream sink submit ordering for live capture is deterministic.
   3. Handle refresh behavior (`memory_fd`, `wait_semaphore_fd`, related frame metadata) is
      deterministic on recreate/resize.
3. Capture mode contract:
   1. Live screencast mode requires presentation + video synchronization path.
   2. Offline/headless mode must support encoding without swapchain present dependency.

Synchronization ordering invariants:
1. Live mode ordering per frame `N`:
   1. `acquire(N)` completes.
   2. draw callback enqueues GPU work for `N`.
   3. stream submit signals timeline value `wait_value(N)`.
   4. video sink receives `wait_value(N)` before handle export for frame `N` is consumed.
   5. present for frame `N` is submitted after render-finished semaphore for `N`.
2. Offline/headless ordering per frame `N`:
   1. draw callback enqueues GPU work for `N`.
   2. stream submit signals timeline value `wait_value(N)`.
   3. video sink waits on `wait_value(N)` or explicit fallback semaphore before encode.
   4. no swapchain present is required.
3. Handle-refresh invariants on recreate:
   1. any changed handle (`memory_fd`, `wait_semaphore_fd`, image metadata) invalidates previous sink bindings.
   2. sink registry receives refresh notification before next encode submission.
   3. frame `N+1` must not encode using handles exported before latest recreate boundary.

### Presentation state machine contract

Runtime states:
1. `UNINITIALIZED`
2. `READY`
3. `ACQUIRE_PENDING`
4. `DRAW_PENDING`
5. `PRESENT_PENDING`
6. `RECREATE_PENDING`
7. `SUSPENDED_ZERO_EXTENT`
8. `FATAL_DEVICE_LOST`

Transition rules:
1. `UNINITIALIZED -> READY` after successful surface/swapchain init.
2. `READY -> ACQUIRE_PENDING` at frame begin in live mode.
3. `ACQUIRE_PENDING -> DRAW_PENDING` on `DVZ_PRESENT_STATUS_OK` from acquire.
4. `ACQUIRE_PENDING -> RECREATE_PENDING` on `DVZ_PRESENT_STATUS_RECREATE`.
5. `ACQUIRE_PENDING -> SUSPENDED_ZERO_EXTENT` on `DVZ_PRESENT_STATUS_SKIP_ZERO_EXTENT`.
6. `DRAW_PENDING -> PRESENT_PENDING` after stream submit success in live mode.
7. `DRAW_PENDING -> READY` after stream submit success in offline/headless mode.
8. `PRESENT_PENDING -> READY` on `DVZ_PRESENT_STATUS_OK` from present.
9. `PRESENT_PENDING -> RECREATE_PENDING` on `DVZ_PRESENT_STATUS_RECREATE`.
10. `RECREATE_PENDING -> READY` after recreate success and handle-refresh completion.
11. `SUSPENDED_ZERO_EXTENT -> RECREATE_PENDING` when extent becomes non-zero.
12. any state -> `FATAL_DEVICE_LOST` on unrecoverable device loss.

Vulkan result mapping:
1. `VK_SUCCESS` maps to `DVZ_PRESENT_STATUS_OK`.
2. `VK_SUBOPTIMAL_KHR` and `VK_ERROR_OUT_OF_DATE_KHR` map to `DVZ_PRESENT_STATUS_RECREATE`.
3. zero extent maps to `DVZ_PRESENT_STATUS_SKIP_ZERO_EXTENT` without Vulkan submit/present calls.
4. `VK_ERROR_DEVICE_LOST` maps to `DVZ_PRESENT_STATUS_DEVICE_LOST`.
5. all other non-success results map to `DVZ_PRESENT_STATUS_ERROR`.


## Non-goals for this refactor phase

1. No DRP/WebGPU command model work in this document.
2. No scene/renderer high-level API rewrite in this document.
3. No rearchitecture of stream/video subsystem beyond synchronization and lifecycle correctness.


## Migration map (file/symbol level)

Moves from canvas to vklite:
1. Move surface capability queries out of `src/canvas/swapchain_sink.c` into `src/vklite/surface.c`.
2. Move swapchain create/recreate image enumeration out of `src/canvas/swapchain_sink.c` into
   `src/vklite/swapchain.c`.
3. Move acquire/present Vulkan entrypoint usage out of `src/canvas/swapchain_sink.c` into
   `src/vklite/swapchain.c`.

Canvas responsibilities that remain in place:
1. Frame loop orchestration in `src/canvas/*` remains authoritative.
2. Offscreen render-target and copy/blit policy in `src/canvas/*` remains in canvas.
3. Stream sink submission ordering and callback wiring remain in canvas/stream integration code.

Expected raw Vulkan presentation call sites after migration:
1. Allowed in `src/vklite/surface.c` and `src/vklite/swapchain.c`.
2. Not allowed in `src/canvas/*`.

Deletion/deprecation targets by M5:
1. Remove duplicate/obsolete canvas helpers that duplicate `vklite` surface/swapchain logic.
2. Remove direct includes in canvas that are only needed for raw swapchain/surface Vulkan calls.
3. Update comments and plan references that still describe canvas-owned swapchain internals.


## Test matrix and environment gating

Build/test commands (canonical):
1. `just clean`
2. `just build`
3. `./dvztest vklite`
4. `./dvztest canvas`
5. `./dvztest stream`
6. `./dvztest video`

Test matrix:
1. `vklite_surface_query`:
   1. Preconditions: Vulkan instance/device available.
   2. Assertions: queried formats/modes/capabilities are cached and refreshable.
2. `vklite_swapchain_recreate`:
   1. Preconditions: window+surface available.
   2. Assertions: recreate destroys old image views, builds new views, and returns expected status.
3. `canvas_present_recovery`:
   1. Preconditions: live mode with swapchain path.
   2. Assertions: `OUT_OF_DATE/SUBOPTIMAL` transitions to recreate path and resumes steady-state.
4. `canvas_zero_extent_suspend`:
   1. Preconditions: resizable window.
   2. Assertions: zero extent produces skip state, non-zero extent resumes without crash/leak.
5. `video_wait_value_propagation`:
   1. Preconditions: video sink configured.
   2. Assertions: sink observes monotonically increasing `wait_value` aligned to submitted frames.
6. `video_handle_refresh_after_recreate`:
   1. Preconditions: recreate event during live capture.
   2. Assertions: sink refresh triggers before next encode, stale handles are never consumed.
7. `offline_headless_encode`:
   1. Preconditions: headless/offline path enabled.
   2. Assertions: encode succeeds without swapchain present dependency.

Environment gating policy:
1. If required Vulkan capabilities are missing, test must skip with explicit reason string.
2. If platform lacks desktop presentation support, live-only tests skip and headless tests still run.
3. If encoder backend is unavailable, video tests skip with explicit backend/capability reason.
4. Skip is valid; silent pass/fail without capability report is not valid.


## Milestone plan

## M0 - Contract freeze and scaffolding

### Goal
Freeze architecture and integration contracts before code movement.

### Deliverables
1. Confirm and document module boundaries (`window`, `vklite`, `canvas`, `stream`, `video`).
2. Add a short contract block in this doc for ownership, synchronization, and capture modes.
3. Create implementation task IDs in this doc for agent execution.

### Tests
1. None required beyond baseline build.

### Exit criteria
1. Contract sections are complete and internally consistent.
2. Execution order is explicit and unambiguous.


## M1 - VKLite presentation API bring-up

### Goal
Introduce vklite surface/swapchain public API and core implementation.

### Deliverables
1. Add:
   1. `include/datoviz/vklite/surface.h`
   2. `include/datoviz/vklite/swapchain.h`
2. Update `include/datoviz/vklite.h`.
3. Add:
   1. `src/vklite/surface.c`
   2. `src/vklite/swapchain.c`
4. Move surface capability/format/present-mode query logic into vklite.
5. Move swapchain create/recreate/acquire/present logic into vklite.

### Tests
1. Build with new headers/sources.
2. Add and run `vklite_surface_query` and `vklite_swapchain_recreate` tests.

### Exit criteria
1. New vklite presentation headers compile and are exported.
2. vklite can acquire/present through its own API.
3. Core vklite tests pass before canvas migration begins.


## M2 - Canvas migration and boundary enforcement

### Goal
Make canvas orchestration-only for presentation flow.

### Deliverables
1. Replace raw swapchain/surface Vulkan calls in `src/canvas/swapchain_sink.c` with vklite API
   calls.
2. Keep copy/blit/render-target orchestration in canvas.
3. Normalize window-to-vklite surface handoff (accessor/wrapper usage).
4. Enforce lifecycle ordering during destroy/recreate paths.

### Tests
1. Run `test_canvas_glfw_present_recovery`.
2. Run `canvas_zero_extent_suspend`.
3. Run presentation-layer regression tests from M1 after migration.
4. Run the forced/induced `OUT_OF_DATE` path and assert recreate + resume.
5. Run `test_canvas_swapchain_failfast_slot_init`.

### Exit criteria
1. No raw Vulkan swapchain/surface calls remain in canvas.
2. Acquire/present works through canvas using vklite-backed path.
3. Presentation regression tests remain green post-migration.
4. End-to-end recovery path is validated (`OUT_OF_DATE/SUBOPTIMAL -> recreate -> resume`).

Status: complete.


## M3 - Synchronization and video integration hardening

### Goal
Finalize synchronization semantics and video sink integration.

### Deliverables
1. Define and enforce sink submit ordering for live capture.
2. Define and enforce frame-handle refresh policy:
   1. when handles change
   2. how stream sinks update/restart
3. Ensure timeline wait value propagation is consistent from canvas to video backends.
4. Ensure external semaphore fallback behavior is explicit and logged.

### Tests
1. Run `test_canvas_video_wait_value_propagation`.
2. Run `test_canvas_video_handle_refresh_after_recreate`.
3. Run backend behavior checks for timeline wait/fallback paths.
4. Run `test_canvas_handle_refresh_order` (refresh-before-submit contract).
5. Run `test_canvas_device_lost_fatal_transition`.

### Exit criteria
1. Video encoding does not consume stale handles after recreate/resize.
2. Synchronization behavior is deterministic and documented.

Status: in progress (`PRES-055`/`PRES-060` done, `PRES-070`/`PRES-075` pending).


## M4 - Capture modes completion (live and offline/headless)

### Goal
Validate both supported capture modes as first-class workflows.

### Deliverables
1. Live screencast mode:
   1. Desktop canvas with presentation and video sink.
2. Offline/headless mode:
   1. Encode-capable path without swapchain present dependency.
3. Add/adjust tests and runbook notes for both modes.

### Tests
1. Run live desktop capture validation path with capability gating.
2. Run `offline_headless_encode`.

### Exit criteria
1. Both modes pass with explicit capability checks and expected behavior.
2. Mode-specific failures are observable and diagnosable.


## M5 - Cleanup, diagnostics, and validation gate

### Goal
Finalize implementation quality and remove obsolete logic.

### Deliverables
1. Remove dead/duplicate canvas presentation helpers.
2. Add/verify diagnostics around recreate/synchronization boundaries.
3. Ensure no steady-state per-frame heap allocations in acquire/present hot path.

### Tests
1. `just build`
2. `./dvztest vklite`
3. `./dvztest canvas`
4. `./dvztest stream`
5. `./dvztest video`

### Exit criteria
1. All required test filters pass.
2. Module boundaries and synchronization contracts are satisfied.
3. Documented plan checklist is fully complete.


## Agent task board

Canonical status and progress are tracked in the live task board above. This section defines the
immutable task ID inventory and wording.

1. `PRES-000` Baseline verification of current raw Vulkan call sites and handle flow.
2. `PRES-005` Freeze API signatures, return semantics, and state-machine mapping in headers/docs.
3. `PRES-010` Add public vklite presentation headers and exports.
4. `PRES-020` Implement `vklite` surface wrapper.
5. `PRES-030` Implement `vklite` swapchain wrapper.
6. `PRES-080` Add presentation-layer tests (surface/swapchain/recreate + API contract guards).
7. `PRES-040` Integrate window-canvas-vklite surface handoff and lifecycle.
8. `PRES-050` Migrate canvas presentation path to vklite API.
9. `PRES-082` Add end-to-end present recovery test (OUT_OF_DATE/SUBOPTIMAL -> recreate -> resume).
10. `PRES-055` Implement deterministic handle-refresh and sink-restart policy.
11. `PRES-060` Harden resize/out-of-date/zero-extent state machine.
12. `PRES-070` Finalize queue/semaphore/fence ownership rules.
13. `PRES-075` Finalize video synchronization and sink ordering contract.
14. `PRES-085` Add capture-mode validation tests (live + offline/headless).
15. `PRES-090` Cleanup dead code and boundary violations.
16. `PRES-100` Final validation gate.


## Canonical execution order

1. M0 (`PRES-000`)
2. M1 (`PRES-005`, `PRES-010`, `PRES-020`, `PRES-030`, `PRES-080`)
3. M2 (`PRES-040`, `PRES-050`, `PRES-082`)
4. M3 (`PRES-055`, `PRES-060`, `PRES-070`, `PRES-075`)
5. M4 (`PRES-085`)
6. M5 (`PRES-090`, `PRES-100`)


## Completion checklist

1. Vklite surface/swapchain public headers and sources are present and wired.
2. Public API signatures and return semantics are frozen and reflected in headers.
3. Canvas no longer directly uses raw Vulkan swapchain/surface APIs.
4. Window/canvas/vklite ownership and destroy/recreate ordering are enforced.
5. Synchronization contract (`wait_value`, handle refresh, sink ordering) is implemented.
6. Presentation state machine behavior matches documented transition and result mapping rules.
7. End-to-end present recovery (`OUT_OF_DATE/SUBOPTIMAL -> recreate -> resume`) is validated.
8. Live screencast and offline/headless capture modes are both validated.
9. `just build` and required `dvztest` filters pass.
10. Test gating emits explicit skip reasons when capabilities are missing.
11. Plan artifacts and code comments reflect final behavior.
