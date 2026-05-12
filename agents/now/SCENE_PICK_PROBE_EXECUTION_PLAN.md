# Scene Pick/Probe Execution Plan

> **Status**
> - **State:** `HISTORICAL PLAN SNAPSHOT`
> - **Scope:** wire real pick/probe resolution through the existing scene -> frame-plan -> DRP2 ->
>   runtime readback path
> - **Written on:** `2026-05-11`
> - **Target:** first end-to-end GPU-backed pick/probe slice for active `scene` + `drp2`

This file is now historical context rather than the current execution guide.
For the shipped behavior and remaining caveats, read
[../done/SCENE_PICK_PROBE_EXECUTION.md](/home/cyrille/GIT/Viz/datoviz/agents/done/SCENE_PICK_PROBE_EXECUTION.md)
and then
[V0_4_NEXT_STEPS.md](/home/cyrille/GIT/Viz/datoviz/agents/now/V0_4_NEXT_STEPS.md).


## Objective

Turn the current retained request/result queues into a real execution path:

1. `dvz_panel_pick()` / `dvz_panel_probe()` queue scene-owned requests,
2. figure emission converts the pending requests into one-frame readback work,
3. the runtime executes that work through the existing DRP2 readback primitive,
4. scene code downloads and decodes the bytes,
5. resolved `DvzPickResult` / `DvzProbeResult` entries become visible through
   `dvz_scene_poll_pick()` / `dvz_scene_poll_probe()`.


## Current Gap

The section below reflects the original pre-implementation plan. Several listed gaps are now closed
in code.

The current codebase already has the pieces below, but they are not connected:

1. `scene` owns pending pick/probe queues plus deterministic result queues.
2. `frame_plan` can describe picking renders, copies, and named readback requests.
3. `converter` emits `QueueSubmit.readbacks` and the DRP2 runtime can execute them.
4. tests already prove the low-level GPU primitive (`test_technique_picking`) and the scene
   readback path for raw bytes.

What is still missing:

1. scene-owned metadata that maps one emitted readback to one pending request,
2. request-driven frame-plan emission from real scene state,
3. post-execute download/decoding logic in the live runtime path,
4. freshness rules so stale replies do not survive after newer requests supersede them.


## First Slice Rules

Keep the first implementation deliberately narrow:

1. request-driven only; no always-on picking pass,
2. point visuals first for pick resolution,
3. image visuals first for probe resolution,
4. one frontmost result per request,
5. one `uint32_t` readback payload per request in phase 1,
6. no public API expansion unless a missing hook blocks the real runtime path.


## Data Model Changes

Add scene-owned emitted-request bookkeeping distinct from the public pending queues.

Planned internal records:

1. emitted request entry for picks:
   - request id,
   - owning panel,
   - panel-relative coordinates,
   - frame-local readback buffer id or resource key,
   - resolved visual pointer/index for decode,
   - generation/freshness token.
2. emitted request entry for probes:
   - request id,
   - owning panel,
   - panel-relative coordinates,
   - target field/visual metadata needed to decode scalar/image probes,
   - generation/freshness token.

The public `DvzPickResult` / `DvzProbeResult` stay as the scene-facing payloads. The new internal
records exist only to bridge emitted GPU bytes back to those structs.


## Emission Plan

### Pick

For each pending panel pick request that can be resolved by the first slice:

1. add a picking render node for the panel,
2. render only visuals that advertise compatible pick capability,
3. copy exactly one pixel into a readback buffer,
4. append a frame-plan readback node with a deterministic request label,
5. register the emitted request metadata before the DRP2 stream is returned.

Phase-1 payload:

1. clear value `0` means miss,
2. non-zero value is `item_index + 1`,
3. scene decode maps the payload back to `visual_id`, `resolved_target`, and `resolved_id`.

### Probe

For each pending probe request that matches the first image/scalar slice:

1. emit a readback against the already-realized image/field path,
2. read back the minimal bytes needed for one probe result,
3. decode CPU-side into scalar/value metadata using existing retained field + scale state,
4. route the result to the scene probe queue with the original request id.

The first probe implementation may use different GPU mechanics from pick if that keeps the slice
smaller, but it should still flow through the same DRP2 readback submission/result routing.


## Runtime Resolution Hook

The real resolution step must happen in the live runtime path, not only in tests.

Initial hook point:

1. after `dvz_drp2_runtime_execute()` succeeds in the app/offscreen draw path,
2. iterate the emitted scene readback records for that figure/frame,
3. download readback bytes with `dvz_drp2_runtime_download_buffer()`,
4. decode bytes into scene-owned result structs,
5. enqueue results,
6. clear the emitted-readback bookkeeping for that frame.

To keep the boundary clean, scene should own the decode/routing helper and the app/runtime path
should only provide the runtime handle plus the figure/scene being executed.


## Freshness Rules

Stale results are worse than dropped results. The first implementation should enforce:

1. newer request with the same `request_id` supersedes older unresolved work,
2. panel hover-style request streams should only expose the newest matching result,
3. a request emitted in frame `N` must not be decoded twice,
4. scene destroy or figure destroy clears unresolved request bookkeeping safely.

If full request-id deduplication is too invasive for this slice, keep the logic instance-scoped and
at least drop any readback whose generation token no longer matches the latest emitted request entry.


## Tests To Add

Add focused coverage before broader feature work:

1. scene unit test: request emission creates the expected pick/probe frame-plan nodes and metadata,
2. scene runtime test: execute one point-pick readback and verify a real `DvzPickResult`,
3. scene runtime test: execute one image probe readback and verify a real `DvzProbeResult`,
4. freshness test: superseded request does not enqueue a stale result,
5. app/offscreen smoke test if the live callback path changes materially.

Prefer narrow tests that fail before the fix and keep existing readback fixture coverage intact.


## Non-Goals For This Slice

Do not expand into these yet unless the implementation forces a minimal preparatory hook:

1. multi-attachment or 64-bit logical pick payloads,
2. mesh face/vertex picking,
3. all-hits picking,
4. persistent always-on picking targets,
5. general selection policy automation beyond result queue delivery,
6. public blocking helpers or new user-facing probe APIs.


## Expected Follow-Up

Once this slice works end-to-end:

1. widen pick payload abstraction away from “single integer forever,”
2. add image/field probe richness and better coordinate reporting,
3. extend to mesh/object/face/instance identities,
4. decide whether richer payloads stay attachment-based or move to a mixed attachment/SSBO design.
