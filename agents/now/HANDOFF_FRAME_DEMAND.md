# Frame Demand And Interaction Pacing Handoff

Status: frame demand and opt-in frames-in-flight experiment implemented; Linux/X11 data favors one slot; Windows slot-count comparison and default-policy decision remain. Updated: 2026-08-02.

This handoff records the approved architecture and implementation contract for responsive on-demand interaction. It remains concrete enough for a lower-reasoning agent to maintain or extend without reopening the architecture question.

Frame-demand implementation checkpoints are `0f413c3fb` (`scene: add figure frame demand`) and `5930352c1` (`app: pace active interactions continuously`). FIFO latest-ready support is `427f00ae6`, the durable latency benchmark contract is `8781de956`, runtime telemetry is `c49e98e1a`, refreshed bindings are `e5d72e2e5`, same-machine comparison tooling is `f29a538c2`, the user-facing `fifo-latest` rename is `a7d612206`, and the opt-in frame-slot experiment is `44307644a`.

## Read First

1. Read [../../AGENTS.md](../../AGENTS.md), [START.md](START.md), and [STATUS.md](STATUS.md).
2. Read [../../spec/scene/pipeline/FRAME_LIFECYCLE.md](../../spec/scene/pipeline/FRAME_LIFECYCLE.md) and [../../spec/scene/pipeline/INVALIDATION_AND_CACHING.md](../../spec/scene/pipeline/INVALIDATION_AND_CACHING.md).
3. Follow [../rules/REPO_HYGIENE.md](../rules/REPO_HYGIENE.md), [../rules/C_CODING.md](../rules/C_CODING.md), and [../rules/GRAPHICS_SAFETY.md](../rules/GRAPHICS_SAFETY.md).
4. Preserve unrelated worktree changes, never stage `data`, run `git diff --check`, and pull immediately before every commit because another machine may update `v0.4-dev` concurrently.

## Problem

The normal `start/scatter` scenario now idles correctly after commit `5c5353050` because its synthetic frame callback is installed only for `DVZ_SCATTER_BENCHMARK=panzoom-v1`.

On-demand drag interaction still feels less smooth than continuous rendering. The current path requests a frame for each mouse event, so presentation cadence follows irregular OS pointer-event delivery instead of the active present cadence. Physical Linux testing showed that explicit continuous FIFO rendering holds approximately 60 FPS and feels smooth, while normal on-demand pan/zoom is responsive but feels event-paced.

This is a scheduling problem, not a scene-throughput problem. Profile builds sustain roughly 16k immediate-mode FPS after capability caching. The benchmark-only full-app artifact cache experiment in `b074f9bfc`, reverted by `832145933`, reduced scene emission from approximately 0.027 ms to 0.0001 ms without improving sustainable throughput. Do not reintroduce that cache.

## Approved Behavioral Contract

```text
idle figure                    -> wait for events and render nothing
one-shot mutation              -> render one successful frame
active controller interaction -> render continuously at the configured presentation cadence
interaction release           -> render the final requested frame, then return to idle
future controller motion       -> render continuously until the motion settles
```

Frame demand answers whether frames should continue. Cadence policy and present mode answer how quickly they should be produced. Keep those responsibilities separate.

## Architecture Decision

Add an internal, extensible scene frame-demand representation rather than a scatter-specific timer or a raw-pointer-event workaround.

The first slice needs only `DVZ_FRAME_DEMAND_INTERACTION`, but choose a bitmask shape that can later add `MOTION`, `ANIMATION`, and `STREAM` without changing the scheduler contract. Keep it internal; this slice does not require public headers, bindings, or `just ctypes`.

Recommended initial shape:

```c
typedef enum DvzFrameDemandFlags
{
    DVZ_FRAME_DEMAND_NONE = 0,
    DVZ_FRAME_DEMAND_INTERACTION = 1 << 0,
} DvzFrameDemandFlags;
```

Add a figure-level internal query such as `_scene_figure_frame_demand(const DvzFigure* figure)`. A focused `src/scene/core/frame_demand_internal.h` plus implementation is preferable if it keeps controller inspection out of `app.c`; a small implementation in the existing controller core is acceptable if it remains cohesive.

The ownership boundary is:

```text
controllers own interacting/motion state
figure aggregates scene frame demand
app scheduler chooses wait, one-shot, or continuous work
canvas and present mode provide actual pacing and back-pressure
```

## Controller Coverage

Aggregate only active controllers bound to panels in the target figure. Inspect `DvzPanel.panzoom`, `arcball`, `fly`, `turntable`, and `controllers[]` in `src/scene/core/_scene.h`; binding code populates the direct payload pointers in `src/scene/core/controllers.c`.

The first implementation must cover:

- panzoom: `panzoom->interacting`;
- arcball: `arcball->interacting`;
- fly: pointer interaction and held navigation state already represented by `_scene_fly_active()` semantics;
- turntable: `turntable->interacting`.

Avoid counting detached, inactive, or unrelated scene controllers. Deduplicate controller pointers if iterating `panel->controllers[]`, although a boolean result makes duplicate observation harmless.

Wheel and double-click are intentionally transient in the current controller dispatch: `src/scene/core/controllers.c` temporarily sets interaction state and restores it before returning. They should remain one-shot in this slice. A future smooth-wheel implementation will keep controller-owned `MOTION` demand active until its time-based integrator settles.

## App Scheduler Changes

The current `_app_has_continuous_work()` in `src/app/app.c` returns one app-global boolean, and the loop passes that boolean to every view. Do not make one interacting view render all unrelated views continuously.

Introduce a per-view continuous-work query, for example `_view_has_continuous_work(const DvzView* win)`, which combines:

- explicit `DVZ_APP_SCHEDULE_CONTINUOUS` configuration;
- the existing frame-callback behavior for that view;
- active replay for that view;
- the new frame demand of that view's figure;
- existing scene animation behavior, preserving current semantics until animations become figure-scoped.

Keep `_app_has_continuous_work()` as the aggregate `any view active` query used to choose host polling versus event waiting. In the render loop, recompute each view's own continuous state after host event processing and pass that per-view value to `_view_should_render()`. Use the per-view value when updating FPS-cap deadlines.

Update `_app_next_continuous_deadline()` so idle views do not contribute deadlines. The app-level aggregate remains necessary because one active view must prevent the host from blocking in `wait events`.

The loop already recomputes continuous work after polling. Therefore a drag event can set `interacting`, wake the loop, and enter continuous mode before rendering; a drag-stop event can clear it, render the final dirty frame, and allow the next iteration to wait.

Do not add `dvz_device_wait()`, `vkDeviceWaitIdle`, sleeps, synthetic pointer events, fixed post-event timers, or changes to swapchain synchronization.

## Existing Callback Contract

Current tests explicitly establish that registering a frame callback enables continuous scheduling: `test_app_offscreen_frame_callback_enables_continuous_scheduler()` in `src/scene/tests/app.c`.

Preserve that behavior in this slice. Separating callback observation from frame demand is a worthwhile later cleanup, but it requires auditing and migrating every live callback user and scenario `continuous_frames` declaration first. Do not silently broaden this task into that migration.

## Tests

Add focused non-GPU scene tests for figure demand where practical, plus scheduler integration tests beside the existing scheduler cases in `src/scene/tests/app.c`.

Required cases:

1. A figure with an idle bound panzoom controller reports no interaction demand.
2. Drag start or drag sets interaction demand.
3. Drag stop or release clears interaction demand.
4. Pointer movement without an active controller manipulation does not create continuous demand.
5. Arcball, fly, and turntable use the same demand contract.
6. `_dvz_app_has_continuous_work()` becomes true while one view's figure is interacting and false after release.
7. Per-view scheduling renders the interacting view continuously without making an unrelated clean view continuously eligible.
8. Existing frame-callback, animation, replay, explicit-continuous, FPS-cap, and one-shot invalidation tests remain green.

Prefer real controller event dispatch in tests over directly assigning `interacting` when a stable helper already exists. Direct assignment is acceptable for the narrow scheduler aggregation test if lower-level controller transition tests separately prove event semantics.

## Manual Acceptance Matrix

Build the profile target, then run normal FIFO scatter to verify demand transitions independently of presentation latency:

```sh
DVZ_PRESENT_MODE=fifo ./build-profile/examples/c/start/scatter
```

Verify one condition at a time:

1. With no input, the app becomes idle and stops reporting frames.
2. Starting a pan wakes immediately without a delayed first frame.
3. Holding a drag produces approximately the resolved FIFO/display cadence, around 60 FPS on the validation machine, even when pointer events are irregular.
4. Motion feels as smooth as `DVZ_APP_SCHEDULE=continuous`.
5. Releasing the drag displays the final position and returns to idle.
6. Wheel remains a correct one-shot update for now.

Physical testing on the NVIDIA RTX 5090 X11 system established that ordinary FIFO sustains 60 presented FPS and receives approximately 130 controller drag events/s but still feels sluggish because queued FIFO images display stale controller states. Immediate mode is responsive. Mailbox is unsupported on this surface and falls back to FIFO. The surface instead advertises `VK_KHR_present_mode_fifo_latest_ready`, which is smooth and tear-free when paired with a scheduler cap:

```sh
DVZ_PRESENT_MODE=fifo-latest DVZ_FPS_CAP=60 ./build-profile/examples/c/start/scatter
```

The FPS cap is intentional: FIFO latest-ready otherwise permits thousands of submitted frames/s while displaying only the newest ready frame at vblank. Use a cap matching the target display refresh rate. The Vulkan device must enable the optional FIFO-latest-ready extension and feature bit; validation layers reject merely passing the enum without both.

Physical Windows testing also found `fifo-latest` with `DVZ_FPS_CAP=60` smooth and responsive. This is useful cross-platform evidence, but it is not sufficient reason to make 60 FPS an unconditional default because the actual display may run at 75, 120, 144, or another refresh rate.

Run the deterministic benchmark workload to ensure it still owns explicit continuous frames:

```sh
DVZ_PRESENT_MODE=immediate DVZ_SCATTER_BENCHMARK=panzoom-v1 ./build-profile/examples/c/start/scatter --benchmark 600
```

The requirements line must include `frame-callbacks`, the output must include `scenario_benchmark_workload: panzoom-v1`, and all requested frames must complete.

## Validation And Commit Sequence

Use logical checkpoint commits only after the relevant checks pass.

1. Implement the internal demand query and its controller tests.
2. Implement per-view scheduler aggregation and scheduler tests.
3. Run the narrow tests while iterating.
4. Run `just build`, the relevant scene/app tests, `just present-check --frames 120`, and `git diff --check`.
5. Inspect `git status --short` and `git diff --cached --stat`; stage only this work and exclude concurrent edits, `data`, generated payloads, and binaries.
6. Pull immediately before each commit. Do not push unless the user explicitly requests a push in that turn.

## Future Smooth-Wheel Extension

Do not implement this extension in the first slice, but preserve the boundary for it.

A wheel event should update a controller-owned target and activate `DVZ_FRAME_DEMAND_MOTION`. Each presentation frame should advance current state toward the target using elapsed time, update the panel transform, and remain active until position and velocity tolerances are met. Repeated wheel events should retarget the existing motion rather than queue animations. Drag start should cancel or take ownership from wheel motion. Completion should snap exactly to the target, request the final frame, clear `MOTION`, and return the scheduler to idle.

The motion integrator belongs to the controller. The scheduler should know only that motion remains active and that another frame is required.

## Frames-In-Flight Experiment

The opt-in experiment is implemented at `44307644a`; it is not a default-policy change. Read [../../spec/testing/INTERACTION_LATENCY.md](../../spec/testing/INTERACTION_LATENCY.md) before maintaining or extending it.

`DvzCanvasSwapchain.image_count` and `slot_count` are now distinct. Unset or `auto` `DVZ_MAX_FRAMES_IN_FLIGHT` preserves the old image-count-sized pool; a positive explicit value resolves to `min(requested, image_count)`; zero and malformed values warn and fall back to current behavior.

The implemented ownership split is:

1. Command buffers, acquire semaphores, in-flight fences, offscreen/depth resources, and Canvas stream-frame entries are per `slot_count`.
2. Swapchain handles, image layouts, and `render_finished` binary semaphores remain per `image_count`; do not move `render_finished` ownership to frame slots.
3. Rotation is modulo `slot_count`, and active/last-presented slot and image indices are tracked explicitly for capture and diagnostics.
4. Live-image metadata refreshes whenever the rotating slot resource generation changes.
5. Interaction telemetry reports requested/resolved present modes, image count, and slot count; comparison reports reject known configuration mismatches and mark older baselines without configuration telemetry as unverified.

Do not add device-idle or queue-idle waits, extra fences, a second swapchain path, or a special scatter renderer. Do not change the default present mode or bake in a 60 FPS cap during this experiment.

Implemented automated coverage includes:

1. slot-count resolution for `auto`, one, two, a value above image count, zero, and malformed input;
2. actual acquire/submit/present cycles with one and two slots under validation layers;
3. resize/recreate with a slot count smaller than image count;
4. last-presented capture and live-sink bookkeeping after slot/image indices diverge;
5. existing canvas, vklite present, app scheduling, capture, and binding checks;
6. `just build`, `just present-check --frames 120`, and `git diff --check`.

The Linux/X11 RTX 5090 comparison used ordinary FIFO, baseline `c49e98e1a`, candidate `44307644a`, five paired runs, and 300 frames per run:

| Candidate slots | Baseline p95 input-to-submit | Candidate p95 input-to-submit | Paired delta | Verdict |
| ---: | ---: | ---: | ---: | --- |
| 1 | 115.8395 ms | 65.8953 ms | -43.17% | improvement |
| 2 | 116.3451 ms | 83.0723 ms | -28.59% | improvement |
| auto (4) | 115.4865 ms | 116.2318 ms | +0.06% | no material change |

Median p95 slot-wait remained approximately 16.2-16.5 ms and acquire-wait remained below 0.04 ms in these runs. These are CPU submission and queue-pressure proxies, not input-to-photon measurements. The historical baseline predates configuration fields, so its report-side configuration is marked unverified; its code uses the original coupled slot/image count, while candidate telemetry resolved ordinary FIFO with four images and one, two, or four slots as requested.

Reproduce or extend the comparison with:

```sh
DVZ_MAX_FRAMES_IN_FLIGHT=1 just compare-interaction c49e98e1a HEAD --runs 5 --frames 300
DVZ_MAX_FRAMES_IN_FLIGHT=2 just compare-interaction c49e98e1a HEAD --runs 5 --frames 300
just compare-interaction c49e98e1a HEAD --runs 5 --frames 300
```

The comparison tool forces ordinary FIFO for `scatter-interaction`. Keep raw reports build-local unless a durable release-evidence location is explicitly chosen.

## Next Decision

Run the same one-, two-, and auto-slot comparison on physical Windows, then make a separate default-policy decision. Linux evidence favors one slot for FIFO interaction and the auto control shows no implementation regression, but the override remains opt-in until cross-platform data is available. The expected direction is capped FIFO latest-ready when supported, a bounded-frames-in-flight FIFO fallback, explicit environment overrides, and eventually refresh-aware pacing instead of an unconditional 60 FPS cap.

## Completion Criteria

The frame-demand and opt-in frame-slot slices are complete: normal interactive views idle without input, active controller drags render continuously, release returns to idle, unrelated views remain idle, existing continuous sources retain their behavior, and one-/two-slot FIFO paths pass validation, resize, capture, and live-sink coverage. Windows comparison and the separate default-policy decision remain.
