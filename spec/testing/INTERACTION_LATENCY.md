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


## Frames-In-Flight Experiment

The current swapchain canvas allocates one frame slot per swapchain image and rotates slots modulo the image count. This couples swapchain image availability to the maximum CPU/GPU work allowed in flight. On a surface with four images, ordinary FIFO can therefore queue multiple stale interaction frames.

After the latency telemetry is stable, separate `slot_count` from `image_count` behind an internal configuration or benchmark-only environment override. Keep swapchain image views, layouts, and render-finished semaphores per image; keep command buffers, acquire semaphores, and in-flight fences per frame slot. Test one, two, and the current image-count number of slots. This is an experiment until validation and latency data establish a safe default.

The experiment must preserve binary semaphore reuse rules, resize/recreate behavior, device-loss handling, captures, live sinks, and validation-layer cleanliness. It must not introduce a parallel renderer or presentation path.


## Commit Comparison

Extend `tools/compare_present_benchmarks.py` with a latency workload while preserving the existing paired, randomized, same-machine methodology. Store raw samples and aggregate metrics in the JSON report. Compare latency using paired p95 deltas with an independently configurable threshold; do not combine latency and throughput into one scalar score.

A valid comparison requires matching machine fingerprints, resolved present mode, display configuration, workload parameters, and frame-slot configuration. The Markdown report should show throughput and latency verdicts separately. Historical reference JSON may be retained per machine, but a two-worktree comparison remains the preferred check because it reduces environmental drift.


## Validation

The implementation is complete when it has:

1. parser and statistical unit tests, including unavailable optional present-completion timing;
2. deterministic controller-path coverage showing one sample per consumed sequence;
3. Vulkan validation runs for FIFO, immediate, and any supported latest-ready mode;
4. a paired self-comparison that produces no material regression;
5. a manual sanity check where ordinary multi-slot FIFO reports worse freshness than immediate or capped latest-ready on hardware that exhibits the original symptom.

Do not add latency assertions to ordinary CI. CI should validate output shape and invariants; per-machine comparisons should classify performance regressions.
