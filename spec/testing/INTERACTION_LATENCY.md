# Interaction Latency Benchmark

Status: active implementation contract.


## Purpose

The presentation throughput benchmarks answer how many frames Datoviz can produce. They do not answer whether pointer-driven interaction displays recent input. A FIFO swapchain may sustain the monitor refresh rate while presenting several queued, stale frames, so interaction latency needs a separate benchmark and regression metric.


## Metric Names And Claims

The initial portable benchmark records these monotonic CPU intervals:

1. `input_to_render_start_ms`: age of the newest benchmark input sample when rendering starts.
2. `slot_wait_ms`: time blocked waiting for the selected canvas frame slot to become reusable.
3. `acquire_wait_ms`: time blocked acquiring a swapchain image.
4. `input_to_submit_ms`: age of the newest benchmark input sample after queue submission and `vkQueuePresentKHR` return.

These are input-to-submission and queue-pressure proxies. They are not physical input-to-photon measurements and must not be labeled as display or presentation latency. `vkQueuePresentKHR` returning does not mean that the image has been scanned out.

An optional later WSI timing tier may add a distinct `input_to_present_complete_ms` metric when the selected device and presentation path expose a reliable present-ID completion mechanism. Unsupported platforms must report that metric as unavailable rather than substitute submit time.


## Controlled Workload

Use the `start/scatter` scenario because it exercises the production pointer router, panzoom controller, scene frame plan, DRP2 runtime, canvas, and swapchain path. The latency workload must inject deterministic pointer press, drag, and release events through the same input API used by a native window. Directly changing panzoom state remains useful for throughput profiling but is not an interaction-latency workload.

Each injected event carries a monotonic source timestamp and a sequence number. The rendered frame consumes the newest sequence known at render start. Samples are emitted only after warmup, and a sequence contributes at most one latency sample. Coalesced intermediate pointer events are expected; the benchmark measures freshness, not event delivery count.

The workload must run for a fixed event count or duration and terminate without manual input. It must report the requested and resolved present modes, swapchain image count, frame-slot count, event count, rendered sample count, and p50/p95/p99/max values for every available interval.


## Runtime Instrumentation Boundary

Keep the instrumentation opt-in and allocation-free in the steady-state frame path after initialization.

1. The scenario runner owns synthetic event generation and source timestamps.
2. The app associates the latest relevant input timestamp and sequence with a view, then records render-start and post-submit timestamps.
3. The canvas records slot-fence and image-acquire wait durations around the existing waits without adding synchronization.
4. The reporting layer aggregates samples and prints one machine-readable summary line.

Do not add `vkDeviceWaitIdle`, queue-idle waits, readbacks, or extra fences to obtain measurements. Instrumentation must observe the current synchronization path without changing it.


## Frames-In-Flight Policy

The swapchain canvas separates reusable frame slots from swapchain images. Ordinary FIFO presentation defaults to one reusable frame slot because deeper FIFO queues display stale interaction state without increasing the display cadence. Other presentation modes default to one slot per swapchain image. `DvzCanvasConfig.frame_slot_count` expresses the deterministic low-level policy: `DVZ_CANVAS_FRAME_SLOT_COUNT_PRESENT_MODE_DEFAULT` selects the mode default, `DVZ_CANVAS_FRAME_SLOT_COUNT_AUTOMATIC` selects one slot per image, and a positive value limits `slot_count` to `min(requested, image_count)`.

The app layer owns the process-wide `DVZ_MAX_FRAMES_IN_FLIGHT=auto|N` override and maps it to explicit Canvas configuration. Canvas does not read environment variables. Swapchain image views, layouts, and render-finished semaphores remain per image; command buffers, acquire semaphores, in-flight fences, offscreen/depth resources, and Canvas stream-frame entries remain per frame slot. Test one, two, and the current image-count number of slots.

The experiment must preserve binary semaphore reuse rules, resize/recreate behavior, device-loss handling, captures, live sinks, and validation-layer cleanliness. It must not introduce a parallel renderer or presentation path.


## Default Presentation Policy

Continuous interaction scheduling exposed a presentation-freshness regression in ordinary FIFO: the app could keep submitting frames while a drag was active, and FIFO preserved stale controller states ahead of newer input. Frame-slot limits reduce CPU/GPU queue depth but cannot control the window-system presentation queue. Forced-continuous FIFO remaining sluggish while immediate and FIFO latest-ready are smooth distinguishes this failure from frame-demand, controller, scene, or renderer throughput defects.

App-owned native windows request FIFO latest-ready by default when the build and runtime support it. FIFO latest-ready is paced per view at the refresh rate reported by the window backend so it presents the newest ready frame without submitting thousands of redundant frames per second. A temporarily unknown refresh rate uses a conservative 60 Hz pacing fallback rather than unlimited submission. If latest-ready is unavailable, Canvas follows the surface preference order: mailbox when supported, then mandatory FIFO. The resolved ordinary-FIFO fallback uses one frame slot.

Window refresh rate is a runtime metric and may change when a window moves between monitors. An explicit positive app FPS cap overrides refresh-derived pacing. Explicit present-mode and frame-slot overrides remain authoritative. Window reports display facts, app selects scheduling policy, Canvas executes explicit frame-slot configuration, and vklite resolves Vulkan surface capabilities.

Every requested frame in the app-owned native loop passes through per-view scheduler admission, regardless of whether the request comes from continuous interaction, a dirty scene, an event, animation, replay, a posted callback, a query, or another one-shot invalidation. Repeated requests coalesce while a paced view waits, the deadline advances only after a successfully presented frame, and a deferred or failed frame retains its pending state without advancing the cadence. Explicit immediate mode without a cap remains unbounded, fixed-count runs and direct `dvz_view_render_once()` calls remain unpaced, and external surfaces remain host-driven.


## Commit Comparison

Extend `tools/compare_present_benchmarks.py` with a latency workload while preserving the existing paired, randomized, same-machine methodology. Store raw samples and aggregate metrics in the JSON report. Compare latency using paired p95 deltas with an independently configurable threshold; do not combine latency and throughput into one scalar score.

Run a same-machine comparison with `just compare-interaction <reference> [candidate]`. The workload uses ordinary FIFO intentionally so queued stale-frame regressions remain visible; use `--latency-threshold-pct` to change the default 10% practical threshold.

A valid comparison requires matching machine fingerprints, resolved present mode, display configuration, workload parameters, and frame-slot configuration. The Markdown report should show throughput and latency verdicts separately. Historical reference JSON may be retained per machine, but a two-worktree comparison remains the preferred check because it reduces environmental drift.


## Validation

The implementation is complete when it has:

1. parser and statistical unit tests, including unavailable optional present-completion timing;
2. deterministic controller-path coverage showing one sample per consumed sequence;
3. Vulkan validation runs for FIFO, immediate, and any supported latest-ready mode;
4. a paired self-comparison that produces no material regression;
5. a manual sanity check where ordinary multi-slot FIFO reports worse freshness than immediate or capped latest-ready on hardware that exhibits the original symptom.

Do not add latency assertions to ordinary CI. CI should validate output shape and invariants; per-machine comparisons should classify performance regressions.
