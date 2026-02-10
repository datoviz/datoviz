# Datoviz v0.4-dev Presentation Stack Refactor Plan

This document is the execution plan for completing the presentation stack refactor before broader
renderer work. It is designed for agent-driven implementation in small, testable increments.

Primary objective:
1. Move Vulkan surface and swapchain mechanics out of `canvas` into `vklite`.
2. Keep `window` responsible for platform surface lifecycle.
3. Keep `canvas` responsible for frame orchestration, stream integration, and draw callback flow.
4. Preserve and formalize video synchronization for both live screencast and offline/headless
   encoding paths.


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


## Current baseline snapshot (branch reality)

1. Active modules in `src/CMakeLists.txt`: `input`, `window`, `canvas`, `stream`, `video`, `vk`,
   `vklite`.
2. Canvas currently owns raw Vulkan presentation logic in `src/canvas/swapchain_sink.c`:
   `vkGetPhysicalDeviceSurface*`, `vkCreateSwapchainKHR`, `vkGetSwapchainImagesKHR`,
   `vkAcquireNextImageKHR`, `vkQueuePresentKHR`.
3. `include/datoviz/vklite/surface.h` and `include/datoviz/vklite/swapchain.h` do not exist yet.
4. Video sink integration already exists via stream sinks (`src/video/video_sink.c`) and uses
   timeline/value-based synchronization.


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

## Public API (minimal additions)

Planned public headers:
1. `include/datoviz/vklite/surface.h`
2. `include/datoviz/vklite/swapchain.h`

Planned `vklite.h` update:
1. Include the two headers above from `include/datoviz/vklite.h`.

Planned public functions:
1. `dvz_surface_init()`
2. `dvz_surface_wrap_native()`
3. `dvz_surface_refresh()`
4. `dvz_surface_destroy()`
5. `dvz_swapchain_init()`
6. `dvz_swapchain_config()`
7. `dvz_swapchain_recreate()`
8. `dvz_swapchain_acquire()`
9. `dvz_swapchain_present()`
10. `dvz_swapchain_destroy()`

Public API non-goals:
1. No new public video API is required for this phase unless a concrete blocker appears.
2. Video synchronization specifics are internal integration contracts, validated by tests.

## Internal integration contracts (must be explicit in code comments/tests)

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


## Non-goals for this refactor phase

1. No DRP/WebGPU command model work in this document.
2. No scene/renderer high-level API rewrite in this document.
3. No rearchitecture of stream/video subsystem beyond synchronization and lifecycle correctness.


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
2. Add vklite tests for surface query and swapchain create/recreate basics.

### Exit criteria
1. New vklite presentation headers compile and are exported.
2. vklite can acquire/present through its own API.


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
1. Canvas frame acquire/submit smoke tests.
2. Resize and out-of-date recovery tests.

### Exit criteria
1. No raw Vulkan swapchain/surface calls remain in canvas.
2. Acquire/present works through canvas using vklite-backed path.


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
1. Live capture synchronization smoke test (presentation + video sink).
2. Handle-change/recreate tests verifying sink refresh correctness.
3. Backend behavior checks for timeline wait/fallback paths.

### Exit criteria
1. Video encoding does not consume stale handles after recreate/resize.
2. Synchronization behavior is deterministic and documented.


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
1. Live desktop capture test path.
2. Offline/headless encode test path.

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

1. `PRES-000` Baseline verification of current raw Vulkan call sites and handle flow.
2. `PRES-010` Add public vklite presentation headers and exports.
3. `PRES-020` Implement `vklite` surface wrapper.
4. `PRES-030` Implement `vklite` swapchain wrapper.
5. `PRES-040` Integrate window-canvas-vklite surface handoff and lifecycle.
6. `PRES-050` Migrate canvas presentation path to vklite API.
7. `PRES-055` Implement deterministic handle-refresh and sink-restart policy.
8. `PRES-060` Harden resize/out-of-date/zero-extent state machine.
9. `PRES-070` Finalize queue/semaphore/fence ownership rules.
10. `PRES-075` Finalize video synchronization and sink ordering contract.
11. `PRES-080` Add presentation-layer tests (surface/swapchain/recreate/recovery).
12. `PRES-085` Add capture-mode validation tests (live + offline/headless).
13. `PRES-090` Cleanup dead code and boundary violations.
14. `PRES-100` Final validation gate.


## Canonical execution order

1. M0 (`PRES-000`)
2. M1 (`PRES-010`, `PRES-020`, `PRES-030`)
3. M2 (`PRES-040`, `PRES-050`)
4. M3 (`PRES-055`, `PRES-060`, `PRES-070`, `PRES-075`)
5. M4 (`PRES-080`, `PRES-085`)
6. M5 (`PRES-090`, `PRES-100`)


## Completion checklist

1. Vklite surface/swapchain public headers and sources are present and wired.
2. Canvas no longer directly uses raw Vulkan swapchain/surface APIs.
3. Window/canvas/vklite ownership and destroy/recreate ordering are enforced.
4. Synchronization contract (`wait_value`, handle refresh, sink ordering) is implemented.
5. Live screencast and offline/headless capture modes are both validated.
6. `just build` and required `dvztest` filters pass.
7. Plan artifacts and code comments reflect final behavior.
