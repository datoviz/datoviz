# Scene Pick/Probe Execution

> **Status:** `FIRST REQUEST-RESOLUTION SLICE COMPLETE`
> - **Completed on:** `2026-05-11`
> - **Scope:** wire real scene pick/probe requests into the existing DRP2/runtime readback path


## What Shipped

The scene now has a real request-consumption path after retained figure rendering has populated the
runtime resource state.

Implemented pieces:

1. `dvz_figure_process_requests()` in the public scene API.
2. App draw-path integration: after a successful main figure execute, the app calls the figure
   request processor against the same DRP2 runtime.
3. Point pick resolution:
   - consumes queued `dvz_panel_pick()` requests,
   - emits a dedicated auxiliary DRP2 stream,
   - renders a point-only picking pass,
   - copies one pixel to a readback buffer,
   - downloads and decodes the payload,
   - enqueues a `DvzPickResult`.
4. First image probe resolution:
   - consumes queued `dvz_panel_probe()` requests,
   - attempts a dedicated auxiliary DRP2/readback path,
   - enqueues a `DvzProbeResult`.
5. Focused scene test coverage for the end-to-end request path.


## Point Pick Path

The current point-pick implementation is request-driven and intentionally narrow.

Current rules:

1. supported visual family: `point`,
2. supported resolved target: `ITEM`,
3. supported hit mode: frontmost single hit through panel z-order iteration,
4. payload encoding: one logical item id encoded into RGBA8 and decoded CPU-side.

Operational flow:

1. main figure render executes normally through the existing retained scene -> DRP2 -> runtime path,
2. request processor scans pending scene pick requests for that figure,
3. for each compatible point visual, it emits a one-panel picking stream,
4. the pick stream uses the same DRP2 runtime boundary and `QueueSubmit.readbacks`,
5. CPU downloads the 4-byte payload with `dvz_drp2_runtime_download_buffer()`,
6. scene maps that payload to `visual_id`, `resolved_target`, and `resolved_id`.

This is a clean DRP2 path in the sense that the request stream itself is emitted as DRP2, executed
by the DRP2 runtime, and resolved from DRP2 readback bytes. It is not yet the final architecture,
because it uses a dedicated auxiliary stream per request instead of integrating picking work into
the main figure stream or a batched request pass.


## Probe Caveat

The first image probe slice is intentionally less complete than point pick.

Current behavior:

1. it goes through the same request queue consumption and auxiliary request execution flow,
2. it attempts a DRP2/render/readback probe path first,
3. if that path does not produce a hit, it currently falls back to scene-side texture sampling to
   produce the probe result.

So the request/result plumbing is live, but probe positioning and fully GPU-resolved probe semantics
are not finished yet.


## Why The Slice Is Structured This Way

This implementation avoided destabilizing the main retained render path while still proving the
scene-owned request lifecycle.

Chosen constraints:

1. no always-on picking target,
2. no broad DRP2 pipeline-format expansion for integer attachments yet,
3. no change to the main figure render stream contract,
4. no public API expansion beyond the request-processing hook.


## Validation

Validation run for this slice:

1. `just build`
2. `./build/testing/dvztest_scene test_scene_process_pick_probe_requests`
3. `just test scene`
4. `git diff --check`


## Follow-Up

The next picking/probing work should proceed in this order:

1. replace the probe fallback with a fully GPU-resolved probe path,
2. formalize request freshness/supersession rules for repeated hover-style requests,
3. move point picking from RGBA8 payload encoding toward a more explicit payload abstraction,
4. batch compatible pick/probe requests instead of emitting one auxiliary stream per request,
5. widen beyond point/image to mesh/object/face/instance semantics.
