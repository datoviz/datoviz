# RC4 Composed Paint Queries And WebGPU Device Lifecycle

Status: approved RC4 implementation allocation, 2026-11-17. This implementation slice preserves the normative [polygon paint contract](../semantics/POLYGON_PAINT.md) and [GPU query architecture](../interaction/GPU_QUERY_SYSTEM.md); it does not claim either work item is implemented. The two issue drafts below are exact proposed GitHub title/body text, prepared for maintainer review and not published.

## Current Boundary

RC3 ships the existing unlit, ordered source-over polygon rendering and conservative panel-wide `DVZ_QUERY_STATUS_UNSUPPORTED_VISUAL_FAMILY` for rendered paint queries. Hidden/empty paint, disabled draw spans, and provably zero effective material alpha do not block queries; holes or an uncovered requested pixel do not permit background fallback while drawable paint remains. Stored semantic IDs do not establish rendered picking support.

Native scientific query composition landed on `rc4/integration` in `7f7a3dcc162b42cf2da43f0e4f1ed845113de6ba` ("Compose native scientific queries in one identity depth pass"). Reuse its generic query orchestration and family-owned contributions for RC4 paint picking; do not build another query executor. Its existing scientific depth policy is not itself the ordered, depth-free paint contract.

`web/wasm/scene.js` currently implements `recoverRuntime()` by destroying the DRP2 runtime, resetting WASM runtime emission, and calling `renderInitial()` with the existing `this.gpu.device`. `web/wasm/session.js` serializes rendering and invokes this recovery after render errors. Recreating a runtime on the same device is not replacement of a lost `GPUDevice`.

The bounded RC3 browser investigation isolated a default canvas-presentation backing failure in the tested Chrome/Linux configuration; explicit ANGLE/Vulkan passes the focused routes. This is configuration-specific rendering proof, not an implementation of device-loss replacement or a complete diagnosis of Chromium internals. Keep that RC3 harness/provider fix separate from RC4 lifecycle work. If persistent provider failure prevents browser proof, only the maintainer-approved narrowly scoped RC3 browser-validation exclusion applies: record exact affected routes, command/environment, diagnostic, and unproven behavior. It is not passing evidence, does not remove native/Python or exact-artifact checks, and does not retrospectively change older evidence.

## Issue Draft 1: Composed GPU Paint Picking

Title: RC4: Compose GPU paint picking with semantic region IDs

Proposed milestone: `v0.4.0rc4` (draft; not yet created).

Body:

RC3 ships ordered polygon rendering while rendered paint queries return a conservative panel-wide `DVZ_QUERY_STATUS_UNSUPPORTED_VISUAL_FAMILY`. Implement the normative topmost-positive-alpha paint hit contract in RC4, reusing the composed query foundation from `7f7a3dcc162b42cf2da43f0e4f1ed845113de6ba` and the existing family-owned query operations. See `spec/scene/semantics/POLYGON_PAINT.md` and `spec/scene/slices/RC4_PAINT_QUERY_DEVICE_LIFECYCLE.md`.

The query must use the same normalized parent/item/role order, draw spans, transforms, clipping, visibility, and GPU coverage rules as visible paint. Use request/frame-scoped GPU hit tokens with a generation-stamped lookup table to resolve semantic region IDs, including 64-bit user IDs, rather than forcing user identity into a raw triangle or 32-bit token. A supported mixed composition must distinguish a covered unsupported contribution from a genuine uncovered miss, so an unsupported foreground cannot silently expose a background hit. Preserve the shared-buffer polygon representation and decode semantic region IDs rather than internal mesh triangle IDs. Paint is composed after world/volume and before UI, without scene-depth testing; do not reuse scientific depth ordering as paint ordering. Explicitly admitted image paint must compose under the same declared policy. Unsupported families, targets, or backend/profile combinations retain clear diagnostics rather than a CPU geometry fallback or independent per-visual winner heuristic.

Acceptance:

- Compare overlapping single polygons with the equivalent collection under both item and collection order, including competing parent attachment order, stroke-over-fill edges, and admitted image interleaving. Select the last covered contribution with positive effective alpha, even when its source-over weight is smaller than the background's.
- Exercise alpha 0, fractional alpha, and 255, material opacity/base-color alpha, visibility, disabled spans, query-capability restrictions, and fixed-controller attachments. Define and test unsupported mixed contributions explicitly before enabling a supported route.
- Match holes, clipped boundaries, antialiased stroke edges, and transformed geometry to visible coverage. Share visible/query coverage logic where practical and add pixel-level regressions against drift.
- Verify semantic region IDs after recoloring, style/visibility changes, topology edits, and attachment reorder. Query the intended current frame after incremental updates; stale asynchronous readback must not report a previous frame as current.
- Prove native GPU execution and focused regressions, refresh bindings before Python validation if the public surface changes, and add portable C/Python/WebGPU packet and live-browser evidence for each newly supported portable route. A missing browser run is a recorded limitation, not parity proof.

## Issue Draft 2: WebGPU Device Ownership And Loss Replacement

Title: RC4: Own WebGPU device generations and replace lost devices

Proposed milestone: `v0.4.0rc4` (draft; not yet created).

Body:

The current `WasmScene.recoverRuntime()` destroys/recreates the DRP2 runtime on `this.gpu.device`; it resets emission but does not acquire a replacement `GPUDevice`. Define device ownership and implement loss-aware replacement within the existing `web/drp2` and `web/wasm` runtime/session boundaries in RC4. See `spec/scene/slices/RC4_PAINT_QUERY_DEVICE_LIFECYCLE.md`. The RC3 configured-provider rendering fix does not close this architectural gap: a runtime reset still reuses the same device, so actual device replacement needs its own loss-injection and recovery proof.

Assign one explicit owner to each device and its generation. Distinguish Datoviz-owned devices from externally borrowed devices: recreate owned devices through the existing acquisition path; expose a clear replacement/notification contract for borrowed devices without destroying them or silently taking ownership. Handle `device.lost` and uncaptured errors with generation-aware callbacks and one serialized recovery transaction. Quiesce submissions, animation, resize/input-triggered work, and query/readback completion for the obsolete generation before rebuilding resources and configuring presentation on the replacement device. Preserve retained scene state and restart only after a successful current-generation frame.

Acceptance:

- Inject device loss and verify a distinct, valid `GPUDevice` is acquired for an owned device, canvas configuration and all device-bound resources use that replacement, and a subsequent visible frame plus query/readback succeeds. Runtime reconstruction on the old device must fail this test.
- Deliver late loss/error callbacks, queued render work, and readback completions from an old generation after replacement; they must neither mutate the current session nor trigger another recovery or publish stale query results.
- Combine loss with resize, input, animation, concurrent render requests, and scene/session disposal. Prove one recovery owner, no obsolete submissions, no duplicate animation loops, and no callbacks resurrecting disposed state.
- Bound retries and recovery duration; handle adapter/device acquisition failures with an actionable terminal diagnostic and an explicit retry path rather than an infinite reset loop. Distinguish recoverable runtime errors, device loss, and validation errors.
- Verify borrowed-device ownership and replacement separately, including that Datoviz never destroys the borrowed device. Test intentional disposal without accidental recovery and preserve scene/controller state through successful replacement.
- Add deterministic lifecycle tests and real-browser loss/replacement evidence with exact browser/provider details. Preserve existing runtime/packet checks; skips or provider failures are not successful loss-recovery proof.
