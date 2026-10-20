# User Input Recording And Replay

Status: direction and detailed contract approved on 2026-10-20; initial implementation validated on Linux; manual interaction and other platforms remain separate gates. This is an opt-in diagnostic facility, not an RC release gate. Existing [input routing](../../interaction/INPUT_ROUTING.md), [keyboard/text](../../interaction/KEYBOARD_INPUT.md) and [latency measurement](../../../testing/INTERACTION_LATENCY.md) contracts remain authoritative. The remaining benchmark acceptance requirements below are not implied by successful input delivery.

## Decision

Record original host input once before GUI consumption, then replay through the same host dispatch path. Use W3C event definitions where representable, with one explicit native capture envelope; support WebDriver Actions through an interchange adapter. Do not use an ImGui protocol, serialize C structures, create a parallel gesture recognizer or repurpose DVZR rendering recordings.

The canonical artifact preserves every event observed at the declared Datoviz host boundary. It cannot recover hardware samples already coalesced by the OS. Derived gestures, embedded-viewport forwarding and resulting render commands are consequences, not additional replay inputs.

## Current implementation boundary

| Boundary | Initial implementation |
| --- | --- |
| Native originals | `datoviz/window/input.h`: owner-thread ingress before GUI arbitration, owned subscriptions, replay exclusion, cancellation, virtual focus/cursor/metrics/time and queue diagnostics. GLFW-generated focus-loss releases are consequences of the recorded cancellation. |
| GUI replay | Datoviz-owned adapter with observed layout and modifier tokens; no live GLFW input/time polling, cursor warping, clipboard or shell access during replay. Unsupported platform operations invalidate playback. |
| Codec | `datoviz.input_trace`: immutable bounded manifest and canonical UTF-8 JSONL, duration including idle tail, event and manifest SHA-256, complete publication after validation. Native text spans are copied by declared length. |
| Actions | `datoviz.input_actions`: strict discrete single-mouse import/export with proven coordinate equivalence, integer coordinates and millisecond times. Other actions and lossy conversions fail explicitly. |
| Runtime | `datoviz.input_replay`: bounded Recorder, one fixed-geometry, matching-platform GLFW-host Player, paced ordinary scheduling through `dvz_app_step`, controlled fixed logical input/GUI ticks, exclusive finalization hook and explicit queue drain. |

Paced GUI time advances at actual GUI processing frames; controlled GUI time uses declared ticks. Gesture timestamps remain independent of real input-arrival profiling timestamps. Controlled playback fixes input/GUI cadence, but does not freeze arbitrary scene animations or worker execution. Finalization pumps run outside the measured input phase while replay still excludes live input. Scheduler iteration duration includes waiting and must not be called CPU frame or GPU time.

The initial runtime reports input delivery and queue completion. Application segment barriers, expected scientific checkpoints, source-generation/capture proof and end-to-end latency acceptance remain separate requirements. In particular, a complete input report is not an equal-work benchmark verdict. Cross-build comparisons require the fixture and evidence below; the IBL diagnostic adapter currently marks that verdict unavailable when rendered proof is absent.

Linux validation covers 97 pure codec/player tests, binding freshness and ABI checks, 175 app-related native checks with synchronization validation, focused input/window/controller checks, 28 GUI checks and four Valgrind ingress regressions without errors or retained heap. An actual private-display GLFW recording contains 33 mouse observations; paced and controlled playback deliver every observation and rotate the scientific camera. Repeated controlled runs produce exactly equal final scientific state. These are correctness checks, not a before/after performance comparison or rendered scientific checkpoint proof.

## Standards Profile

The manifest identifies the native adapter platform; raw native offsets must not silently cross platform-specific wheel normalization. Portable Actions need explicit native binding before playback. The manifest pins these dated references; updating a reference requires a profile revision and compatibility review rather than following a moving `latest` URL.

| Reference | Role and limits |
| --- | --- |
| [Pointer Events 3, Recommendation, 2026-06-30](https://www.w3.org/TR/2026/REC-pointerevents3-20260630/) | Pointer identity/type and button-state semantics. This native profile additionally requires full fractional-coordinate preservation; PE3 does not independently mandate that representation. DOM targeting, propagation and browser conformance are not claimed. |
| [KeyboardEvent code Values, Recommendation, 2025-04-22](https://www.w3.org/TR/2025/REC-uievents-code-20250422/) and [UI Events, Working Draft, 2026-02-21](https://www.w3.org/TR/2026/WD-uievents-20260221/) | Physical `code`, layout-dependent `key`, `location`, modifiers and repeat vocabulary. Unknown physical identity remains unknown. |
| [Pointer Events 4, Working Draft, 2026-10-08](https://www.w3.org/TR/2026/WD-pointerevents4-20261008/) | Informative combined mouse/wheel definitions, including fractional deltas and explicit `deltaMode`. Draft-only fields are not required. |
| [WebDriver Actions, Working Draft, 2026-07-02](https://www.w3.org/TR/2026/WD-webdriver2-20260702/#actions) | Standard action payloads for supported import/export. Browser contexts, integer-constrained fields, millisecond durations and movement interpolation are not a lossless native recorder. |

This is a W3C-aligned native capture profile, not a WebDriver server or a DOM implementation. The only Datoviz-specific vocabulary describes capture identity, native fidelity, host environment and benchmark execution. Standard action payloads are not silently extended or redefined.

## Capture Boundary And Host Ownership

```text
live host adapter ─┐
                  ├─ host ingress / virtual input state ─ GUI arbitration ─ scene routing
replay player ────┘                │
                              recorder
```

The ingress owns ordered pointer/button/key state, focus, host dimensions and coordinate conversion. It snapshots originals before GUI consumption, including GUI-only actions. A record identifies the logical host window, not a native pointer or transient scene handle. GUI viewport hit testing, capture and source forwarding run normally after ingress; recording that forwarded stream again would duplicate input.

Platform adapters must preserve live coordinate, modifier and wheel behavior while moving it behind this shared seam. Native wheel offsets retain their source convention before the existing GUI/scene-specific normalization; replay must apply each normalization exactly once. Host management remains host-owned: recorded resize/scale/focus changes update virtual metrics and use the existing host notification path, not router notification alone.

Router subscriptions remain useful for diagnostic counts and state checkpoints. They are insufficient for full-session capture because GUI consumption precedes router delivery. Existing `dvz_view_emit_*` helpers also do not replace host ingress: they currently dispatch into the canvas router and stamp current time. The recorder/player must not change the documented typed/union distinction or re-interpret already-derived gestures.

## Artifact And Event Model

The proposed diagnostic encoding is a UTF-8 JSON manifest plus ordered JSON Lines observations. JSON is an interchange/storage boundary; dispatch uses typed values. The manifest names the format/profile version, required capabilities, pinned standard references, host-adapter convention, trace-local windows, initial neutral input state, logical/framebuffer geometry and scale, fixture identity, event count, integrity digest and completion status. Application fixtures and experiment settings are separate from input records.

| Observation field | Required behavior |
| --- | --- |
| `seq` | Strictly increasing global safe JSON integer, preserving order across windows and equal timestamps. |
| `time_ns` | Nonnegative, nondecreasing monotonic offset encoded as a decimal string, independent of wall-clock date and JSON floating-point limits. |
| `window` | Stable trace-local host identifier, explicitly bound to one replay host. |
| `event` | Tagged value with the applicable W3C fields below; unsupported native facts live under a `datoviz` namespace. |

An informative motion observation, with coordinates in the manifest's declared logical client space, is:

```json
{"seq":12,"time_ns":"8340000","window":"host-0","event":{"type":"pointermove","pointerId":1,"pointerType":"mouse","clientX":401.25,"clientY":210.5,"button":-1,"buttons":0,"ctrlKey":false,"shiftKey":false,"altKey":false,"metaKey":false}}
```

The native profile preserves original double coordinates in native client units, with an explicit native-to-logical transformation in the host metrics; it avoids a lossy conversion and round trip. W3C field names describe the event vocabulary; their native profile units are not declared CSS pixels by inference. Required capabilities include precision, wheel convention, key identity and text commits. A consumer rejects an unknown required capability rather than ignoring it. The executable [manifest](../../interaction/input_manifest_v1.schema.json) and [observation](../../interaction/input_observation_v1.schema.json) schemas define allowed fields; the pure codec additionally enforces ordering, transitions, bounds, capability use and integrity.

| Input family | Fidelity rule |
| --- | --- |
| Pointer movement and buttons | Preserve fractional position, pointer identity/type, modifiers and every button transition. Implement W3C first-button `pointerdown`, last-button `pointerup`, and chord transitions through `pointermove` with changed `button`/`buttons`; inverse mapping must recover each native edge. Do not rename every native press to `pointerdown`. |
| Wheel | Preserve both axes and original fractional offsets. Populate standard `deltaX/Y/Z` and `deltaMode` only when the adapter can prove their units/sign mapping. GLFW exposes [scroll offsets](https://www.glfw.org/docs/latest/input#scroll_event), not CSS pixel/line/page units; otherwise retain `datoviz` native offsets and a versioned convention identifier. No guessed conversion or wheel accumulation. |
| Keyboard | Preserve physical identity, press/repeat/release, modifiers and available standard `code`/`location`. Record layout-derived `key` only when observed, never fabricate it from a US-position token. Keep native identity under a declared capability where standard mapping is unavailable. |
| Text commit | Preserve each committed UTF-8 span and its boundary separately from key events. Use an explicitly namespaced commit tag; projecting it to a browser editing operation is optional and cannot imply full IME support. Copy borrowed text before dispatch returns. |
| Host environment | Explicitly namespaced focus, enter/leave, resize and scale observations preserve ordered transitions and both logical/framebuffer metrics. They are distinct from GUI widget focus and internal pointer capture. |

The initial profile supports mouse, keyboard and committed text in one selected host window, including embedded scientific viewports and sidebar controls. The envelope permits multiple windows, but replay requires a declared capability and complete bindings. Touch/pen, clipboard-dependent operations, file dialogs, drag/drop and IME preedit are not silently approximated; require a supported deterministic adapter/fixture or fail capture/replay with an unsupported-capability result.

## Session Lifecycle And Failure

Capture starts only after the declared fixture is ready and keys/buttons are neutral; start/stop controls are excluded by explicit segment boundaries. Initial pointer position and focus are snapshotted. Stop closes a complete artifact with integrity metadata. Stopping mid-gesture either records an explicit cancellation policy or leaves a non-benchmarkable incomplete session; the player must not invent a successful release.

Dispatch and input state follow the existing owner-thread contract. Encoding/file IO must not block input callbacks: copy records into bounded owned storage, flush outside dispatch, and mark the entire capture incomplete on overflow or IO failure. Never silently discard input, report an incomplete file as valid or serialize `user_data`, callbacks, raw pointers or ABI-dependent bytes. Report recording overhead separately.

Validate manifest, completion/integrity, versions/capabilities, event ordering, finite values, UTF-8, input-state transitions and limits before benchmark playback. Truncation, malformed observations, missing fixtures, unsupported conventions or divergent required checkpoints fail the run with a diagnostic and last processed sequence. Partial diagnostic inspection is allowed only under an explicit non-benchmark mode.

Replay requires exclusive ownership of input to its selected host. Live input must be rejected or abort replay by declared policy; a separate cancellation control remains available. Focus loss/cancellation clears held state and capture through a defined sequence without accidentally activating clicks or text commands. Stop removes subscriptions and queued replay work before their host/user data is released. Application workers retain their normal cancellation/shutdown contracts.

## GUI Isolation And Clocks

The existing GLFW GUI adapter polls keys/modifiers, cursor, focus, metrics and time and may warp the cursor. A Datoviz-owned GUI input/frame adapter must supply recorded virtual state during replay and prevent OS polls or cursor warps from overriding it. Keep GLFW presentation and vendored ImGui intact. Native/headless/browser adapters share the profile; a backend without complete isolation reports the missing capability rather than claiming comparable replay.

Keep three time domains explicit: recorded logical event time for gesture recognition; declared logical GUI/scene processing time for controlled replay; and real monotonic time for scheduling/profiling. Existing scene fixed-step/external clocks may supply animation time but do not control ImGui time or pointer recognition automatically. Rebase scheduled time onto the replay's monotonic origin; retain actual delivery times separately. Do not feed synthetic logical timestamps into wall-time latency subtraction. A test clock belongs to the player/adapter boundary, not a global replacement for Vulkan waits or host presentation time.

Viewport forwarding currently reads GUI state once per frame, and GUI event trickling can defer transitions. Therefore identical offered host events do not guarantee identical accepted scene input or scientific work. Record offered/delivered counts, accepted scene/gesture counts and state checkpoints as diagnostics; never inject those derived diagnostics as additional inputs.

## Replay Experiments

| Mode | Execution and permitted claim |
| --- | --- |
| Paced host replay | Offer originals in sequence against the recorded relative schedule, using ordinary app scheduling. Deliver late events in order without player-side dropping/coalescing or silently stretching the schedule. Report lateness/backlog and offered versus accepted events. This compares offered user workload; frame-dependent GUI forwarding and asynchronous completion may differ. |
| Controlled replay | Use fixed logical input batches, GUI pump cadence/time and scene-clock policy, independent of achieved render FPS. Specify ordering of dispatch, GUI processing, scene work and measurement boundaries; disable or reproduce event trickling consistently. Check declared fixture/readiness barriers while continuing owner-thread progress. Report completed-work costs, not original-session latency. |

Input production must never advance once per completed rendered frame unless that dependency is explicitly the controlled workload being studied. A future executable fixture must supply the pump/batch policy; absence of such a policy is unsupported controlled replay, not a default guess. Readiness barriers are declared segment boundaries, have timeouts and do not replace ordinary execution with guessed sleeps. Native code treats application readiness as a generic predicate/checkpoint; it knows nothing about IBL sessions, regions or slices.

The authoritative recording has no sampling cap. Optional motion reduction creates a separately identified derivative with its parent digest, algorithm/rate, retained boundaries and loss report. Preserve button/key/text/wheel events and ordering. Arcball rotations are path-dependent; equivalent endpoints do not establish equivalent orientation or workload. Coordinate rescaling or layout adaptation likewise creates an explicitly different experiment.

Use the [existing latency metric names and synchronization boundary](../../../testing/INTERACTION_LATENCY.md). Timed results identify build/library, fixture and trace digests, replay policy, layout/DPI, presentation mode, validation/profiler settings, warmup, counts and failures. CPU/GPU intervals overlap; GPU completion and queue submission are not input-to-photon evidence. Correctness runs and instrumentation-overhead comparisons precede performance claims; CI checks invariants rather than portable timing thresholds.

## WebDriver Actions Interchange

Import/export keeps standard `actions`, input-source identifiers, pauses, pointer/key/wheel actions and durations unchanged for the declared supported subset. Native host binding and fixture metadata remain outside the standard payload. This does not require a remote server, browser session or BiDi transport. A browser adapter may later use [BiDi input](https://www.w3.org/TR/2026/WD-webdriver-bidi-20261008/#module-input), whose action behavior references WebDriver; it is not the native trace format.

Exact interchange succeeds only if every requested value and ordering relationship is representable under the pinned standard. Coordinate/wheel rounding, timing quantization, pointer interpolation, text-to-key synthesis, unit conversion and loss of host/environment observations require rejection by default. An explicitly requested approximation produces a derived artifact with a per-field loss report and cannot be labelled the original benchmark trace. Duration-zero samples avoid interpolated intermediate motion but do not by themselves preserve native clock precision or GUI batching. Repeated standard key-down actions and native repeat/text events must not be conflated.

## Implementation Boundaries And Acceptance

Land separately: shared host ingress preserving live behavior; replay-safe GUI state/frame preparation; owned recorder/validator/player; controlled processing and application readiness hooks; Actions interchange. Public API layouts and executable schemas now implement the initial boundary above; additional segment/checkpoint capabilities remain reviewed deliverables. After detailed acceptance, promote the shared contract to the canonical interaction spec and leave this proposal as a short decision record. No current canonical semantics have moved here.

| Gate | Required evidence |
| --- | --- |
| Input parity | Original/derived cardinality unchanged; chorded buttons, fractional motion/wheel, physical repeat plus non-ASCII/multi-scalar commits, focus loss and cancel; existing nested-dispatch/allocation-failure invariants remain valid. |
| Host/GUI isolation | Mixed sidebar and scientific-view recording exercises the common ingress once; external typing/mouse movement cannot affect an isolated replay or warp the desktop pointer. Live and replay GUI metrics/clock state use the declared provider. |
| Trace integrity | Roundtrip preserves values/order/commit boundaries; malformed/truncated/oversized/unknown-required data and overflow/IO failure cannot yield a successful benchmark. |
| Timing and outcomes | Fake-clock tests cover ties, lateness and backlog; two builds receive matching offered sequences. Controlled tests verify pump boundaries, accepted gestures, final camera/cursor/selection and readiness; divergence is explicit. |
| Performance evidence | Same-machine repeated paired runs identify the policy and completed work; primary runs omit profilers, diagnostic runs capture CPU/GPU, and validation-enabled correctness remains a separate required check. |

The approved initial capabilities and fixed input/GUI pump contract are implemented above; additional benchmark acceptance capabilities require explicit fixtures and proof. Default scope remains one host with complete GUI/scene coverage, fixed geometry, full-fidelity capture and opt-in execution; unsupported host features fail explicitly. Routine symbol/codec organization does not require a new architecture decision.
