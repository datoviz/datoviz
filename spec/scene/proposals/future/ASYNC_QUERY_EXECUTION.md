> **Execution Status**
> - **Status:** `FUTURE QUERY PROPOSAL`
> - **Updated on:** `2026-09-16`
> - **Purpose:** define bounded asynchronous completion for interactive scene queries after v0.4 without changing GPU query semantics or freshness authority.

# Asynchronous Query Execution

## Decision

Queued interactive queries should submit GPU work without synchronously completing its readback inside the rendering frame. Completion is polled later through a bounded retained-resource ring, and existing panel/request-scope freshness rules decide whether a completed result may be published.

The explicit synchronous query helper remains available for tests, tools, and callers that deliberately require immediate completion. It must not define the ordinary pointer-interaction path.

## Ownership Boundary

The scene query layer owns request identity, freshness, semantic decoding, and result publication. FramePlan and DRP2 own backend-neutral query commands and readback correlation. The runtime owns submission, completion primitives, mapped/downloaded bytes, and device-loss reporting. The app scheduler observes pending completion demand but does not interpret query payloads.

Native fences, WebGPU callbacks, mapped-buffer details, and other backend completion mechanisms must not appear in the public scene API or alter `DvzQueryResult` semantics.

## Request Lifecycle

An accepted asynchronous request moves through these internal states:

1. `QUEUED`: accepted and eligible for same-scope latest-wins coalescing before submission.
2. `SUBMITTED`: owns one retained execution slot and correlation identity.
3. `COMPLETED`: backend work and readback finished successfully and bytes are available for decode.
4. `OBSOLETE`: a newer same-scope request, generation change, or destroyed owner prevents publication; resources remain retained until safe reuse.
5. `FAILED`: execution, device, mapping, download, or decode failed and produces the existing typed terminal status when the request is still publishable.
6. `RETIRED`: result publication or suppression is complete and the execution slot may be reused.

Completion order may differ from submission order. Only the newest publishable freshness serial for a panel and request scope may mutate item interaction or enter the public result queue. Obsolete completions are counted and retired without application-visible state mutation.

## Bounded Work

The implementation must use fixed or explicitly bounded limits for:

- retained execution/readback slots;
- submissions admitted per frame;
- completions decoded per frame;
- pending requests per panel and request scope;
- bytes retained by query resources.

When capacity is exhausted, same-scope pointer requests should remain latest-wins: an unsubmitted obsolete request may be replaced, but submitted resources are not reused until backend completion makes reuse safe. Unrelated request scopes must not be starved indefinitely by pointer motion.

Capacity pressure must be observable through submitted, completed, coalesced, obsolete, failed, and capacity-deferred counters. It must not create an unbounded CPU queue or allocate one new runtime object set per pointer event.

## Scheduling

Outstanding query completion is a bounded scheduler demand distinct from scene dirtiness and animation. A submitted request may require later completion polling even when no scene visual changed, but the app must not busy-render continuously while waiting.

The native path should use the existing event/pacing mechanism or a bounded completion-poll cadence. A backend callback may request one owner-thread wakeup through the established posted-work path. WebGPU completion must re-enter scene processing on the owning thread rather than mutate scene state from a backend callback.

One or more frames of query latency are acceptable. Unbounded latency, FIFO publication of stale hover results, and unpaced polling are not.

## Generations And Teardown

Each submitted request must retain or record enough identity to reject completion after:

- panel, figure, visual, or scene destruction;
- source geometry or query-schema replacement;
- render-target resize or recreation;
- runtime reset or device loss;
- request-scope freshness advancement.

Destruction cancels publication immediately but may defer resource reclamation until submitted work is no longer in flight. No teardown path may wait on or dereference an application object merely to publish an obsolete result.

## Synchronous Helper

`dvz_panel_query_now_px()` remains a deliberately synchronous convenience. It may use the same plan construction and execution machinery, but it owns a separate completion policy and must not drain, reorder, or publish unrelated asynchronous requests.

Tests may use the synchronous helper for deterministic semantic assertions. Interactive examples and controllers should exercise the queued path.

## Implementation Milestones

1. Add phase timings, resource-shape counters, correlation identity, and deterministic freshness tests without changing execution behavior.
2. Separate submission from download/decode behind an internal query-execution slot and prove one deferred request end to end.
3. Add a bounded slot ring, later-frame completion polling, stale-result suppression, and capacity diagnostics.
4. Integrate scheduler wakeup without continuous rendering and validate native and WebGPU completion behavior.
5. Retain the synchronous helper and migrate interactive ITEM/FACE workloads to asynchronous completion.

## Validation

Implementation requires deterministic coverage for:

- in-order and out-of-order completion;
- multiple request scopes on one panel and across panels;
- coalescing before submission and obsolescence after submission;
- ring saturation and fair bounded progress;
- resize, geometry replacement, runtime reset, device loss, and destruction with work in flight;
- no stale item-state mutation or public result publication;
- no per-request retained-resource growth in steady state;
- native and WebGPU capability behavior;
- an interactive large-mesh benchmark showing that query completion no longer synchronously stalls the rendering frame.

Wall-clock thresholds remain machine-local benchmark evidence. Ordinary CI should assert lifecycle, freshness, capacity, allocation, command, and upload invariants.

## Non-Goals

This proposal does not add CPU picking, promise zero-latency results, define mesh-part identity, expose backend synchronization objects, or introduce atlas-specific request modes. Mesh-part ITEM identity is specified separately in [MESH_PART_STATE.md](MESH_PART_STATE.md), and exact query semantics remain owned by [../../interaction/GPU_QUERY_SYSTEM.md](../../interaction/GPU_QUERY_SYSTEM.md).
