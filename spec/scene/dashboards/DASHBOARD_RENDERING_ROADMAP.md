# Dashboard Rendering Roadmap

> **Status:** exploratory roadmap for v0.5+.
> **Scope:** fast retained interactive dashboards built on the Datoviz scene, DRP2, and app stack.
> **Primary proving ground:** [`../examples/scenarios/v05/DASHBOARD_AND_STREAMING.md`](../examples/scenarios/v05/DASHBOARD_AND_STREAMING.md).


## Summary

Dashboard work should be proven in standalone examples before it becomes public API. The immediate
goal is to pressure-test retained updates, streaming resources, trace rendering, linked panels,
overlays, interaction, and telemetry without forcing each application to rebuild those pieces.

The first implementation should stay C-first because the engine questions are resource lifetime,
partial uploads, frame pacing, DRP2 technique selection, and instrumentation. Python convenience can
follow once the C semantics are stable.

Scenario proving grounds live in
[`../examples/scenarios/v05/DASHBOARD_AND_STREAMING.md`](../examples/scenarios/v05/DASHBOARD_AND_STREAMING.md).
Reusable resource, update, and telemetry requirements should be promoted into pipeline,
interaction, validation, or API specs before becoming public semantics.


## Canonical Dependencies

| Topic | Use |
|---|---|
| DRP2 capability tiers | [`../../drp2/CAPABILITIES.md`](../../drp2/CAPABILITIES.md) |
| Frame lifecycle | [`../pipeline/FRAME_LIFECYCLE.md`](../pipeline/FRAME_LIFECYCLE.md) |
| Invalidation and partial rebuilds | [`../pipeline/INVALIDATION_AND_CACHING.md`](../pipeline/INVALIDATION_AND_CACHING.md) |
| Controller binding and linked panels | [`../decisions/CONTROLLER_BINDING_MODEL.md`](../decisions/CONTROLLER_BINDING_MODEL.md) |
| Camera/controller behavior | [`../interaction/CAMERA_CONTROLLERS.md`](../interaction/CAMERA_CONTROLLERS.md) |
| Touch and gesture input | [`../integration/future/TOUCH_SUPPORT.md`](../integration/future/TOUCH_SUPPORT.md) |
| Diagnostics and telemetry shape | [`../validation/DIAGNOSTICS.md`](../validation/DIAGNOSTICS.md) |
| Transparency and overlays | [`../semantics/TRANSPARENCY.md`](../semantics/TRANSPARENCY.md) |


## Proving Grounds

| Example | What it proves |
|---|---|
| Streaming DAQ viewer | many analog/digital channels, appendable samples, ring wrap, scrolling/sweep cursor modes, one-draw trace paths, partial uploads, workload controls |
| Image embedding LOD | large retained point clouds, image thumbnail LOD, hover/selection readouts, async asset pressure |
| Semantic embedding atlas | labels, semantic metadata, search, selection, metadata cards, linked overview/detail interaction |

Reusable pieces should be extracted only after examples show stable cross-domain value.


## Design Goals

Dashboard support should prioritize:

1. retained objects that are cheap to update;
2. persistent GPU resources with bounded lifetime;
3. partial resource updates and append/circular data flows;
4. stable frame pacing under continuous updates;
5. linked panels, shared controllers, cursors, and overlays;
6. clean separation between data generation, storage, rendering technique, and interaction;
7. queryable telemetry from user code;
8. C-first core behavior with later Python wrappers.

Dashboards should mutate retained scene state. They should not rebuild scene structure, pipelines,
descriptors, or GPU resources every frame.


## Candidate Primitives

| Primitive | Responsibility | First home |
|---|---|---|
| Dirty range tracking | track byte/element ranges, merge adjacent ranges, preserve wraparound, expose minimal uploads | example-local or `src/common` if reused |
| Appendable ring resource | fixed-capacity CPU/GPU resource, append samples, track write index/wrap count, expose dirty ranges | example-local before public API |
| Streaming update API | update byte ranges, append to fixed-capacity resources, preserve identity, report upload size/count | scene/resource model |
| Frame/resource telemetry | FPS, CPU/GPU timing when available, uploaded bytes, update count, draw count, descriptor refreshes, transient resources, readbacks | app/scene query API |

Open primitive choices: byte versus element ranges, wrapped range representation, automatic versus
caller-controlled merging, and public versus internal placement.


## Candidate Visual Families

| Visual/composition | Purpose | Notes |
|---|---|---|
| Timeseries/trace | appendable analog traces with fixed visible history, ring/scroll/sweep modes, channel metadata, stacked/overlaid layout | name should be `timeseries`, `trace`, or `strip_chart`, not `daq` |
| Digital trace/step line | horizontal high/low segments, transitions, pulses, dense event-like behavior | mode of timeseries or separate visual depending on technique sharing |
| Event raster | sparse spikes, TTL pulses, logs, triggers, alarms, annotations | supports lanes, event type colors, picking, retention windows |
| Min/max envelope | high-density signal extrema when many samples map to one pixel column | CPU first, GPU/compute later |
| Cursor/marker/selection overlays | time cursor, sweep cursor, ranges, thresholds, row highlights, crosshairs | must layer cleanly above data and preserve picking/depth/blending rules |
| Strip chart composition | trace + grid + labels + cursor + history + optional overview/detail | only after lower-level trace and overlay pieces are proven |


## Rendering Technique Candidates

Technique selection can be exposed in examples for benchmarking. Public dashboard semantics should
remain backend-neutral and adapt through DRP2 capability tiers.

| Technique | Strengths | Risks / limits | WebGPU posture |
|---|---|---|---|
| Instanced line-strip from interleaved data | one draw, no CPU transposition, natural DAQ layout, no index buffer | shader ring addressing; digital steps may need another path | strongest baseline: ordinary instancing and buffers |
| Indexed strip with primitive restart | one indexed draw, compact strips, no fake connectors | large/dynamic index buffers; restart value constraints | portable enough to test; restart value fixed by `stripIndexFormat` |
| Per-channel draw/indirect loop | clean topology, natural wrap splitting | command cost and feature support | do not require native multi-draw indirect |
| Shader-discard connector masking | simple non-indexed legacy baseline | wasted assembly/raster work; discard can hurt picking, depth, derivatives, transparency | benchmark only |
| Expanded segments/quads | linewidth, antialiasing, joins, caps, digital steps, high-DPI quality | more vertices/uploads; CPU or GPU expansion cost | likely best portable quality path |
| Sampled buffer/texture path | raw layout preserved, GPU ring addressing, base for filtering | stricter binding/alignment/resource rules | plausible with storage buffers and careful layout |
| GPU decimation/envelope | scalable long histories, preserves extrema | synchronization and capability complexity | advanced tier with CPU/render-only fallback |


## Composition Helpers

| Helper | Required behavior |
|---|---|
| Panel grids/shared controllers | express shared time axes, vertical navigation, overview/detail, and screen-anchored overlays without manual callbacks |
| Linked interaction groups | propagate pan/zoom, cursor, selected time range, selected channel, reset/fit |
| Overlay layers | explicit ordering such as background, grid, data, events, selection, cursors, labels, UI |
| Stable object identity | update trace 37, cursor A, panel 2, or marker group without recreating visuals |
| Declarative update batches | coalesce append samples, cursor moves, labels, and selection changes into efficient frame plans |

The minimal native overlay layout target is tracked in
[`../proposals/active/SCREEN_SPACE_OVERLAY_LAYOUT.md`](../proposals/active/SCREEN_SPACE_OVERLAY_LAYOUT.md).


## Extraction Strategy

1. Implement the standalone C DAQ dashboard/stress example.
2. Keep rendering technique adapters private to the example initially.
3. Add telemetry needed to measure uploads, draws, frame pacing, descriptors, and transient
   resources.
4. Extract dirty-range or ring-buffer helpers only when repeated code appears.
5. Promote a minimal timeseries/trace visual after one technique proves useful.
6. Add digital/event lane support once analog streaming is stable.
7. Add shared-controller, cursor, overlay, and linked-panel helpers.
8. Add high-density envelope/decimation after simple path limits are measured.
9. Add Python convenience after C API and semantics are stable.


## Open Questions

| Question | Suggested direction |
|---|---|
| Core versus examples | keep resource updates, telemetry, and stable mutation APIs low-level; keep full widgets in app/examples until clearly reusable |
| Public technique selection | examples expose exact techniques; public visuals choose defaults and accept hints, not hard Vulkan-shaped contracts |
| GUI ownership | provide app-layer overlays/debug panels; keep ImGui optional; avoid becoming a full GUI framework |
| WebGPU constraints | keep semantics backend-neutral; prioritize instanced interleaved traces, expanded geometry, min/max envelope, primitive restart as option, indirect paths as advanced optimizations |
| C-first or Python-first | validate engine/resource behavior in C first; add Python wrappers later |
| Time/sample indexing | distinguish logical sample index, physical ring index, sample-rate time, explicit timestamps, and display time |
| Channel metadata | support common name/color/type/offset/gain/visibility/group/order; keep richer domain metadata external |


## Next Decisions

The next useful decisions are the first C DAQ example architecture, initial trace technique,
minimum scene/DRP2 features for instanced interleaved traces, telemetry struct shape, dirty-range
placement, smallest `DvzTimeseries` surface, and WebGPU compatibility constraints.
