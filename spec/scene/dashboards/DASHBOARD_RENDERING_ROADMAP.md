# Dashboard Rendering Roadmap

> **Status:** exploratory roadmap for v0.5+.
> **Scope:** fast retained interactive dashboards built on the Datoviz scene, DRP2, and app stack.
> **Primary proving ground:** `spec/scene/examples/dashboards/STREAMING_DAQ_VIEWER.md`.
> **Additional proving grounds:** `spec/scene/examples/dashboards/IMAGE_EMBEDDING_LOD.md` and
> `spec/scene/examples/dashboards/SEMANTIC_EMBEDDING_ATLAS.md`.


## Summary

Datoviz v0.4 is currently focused on stabilizing the scene -> DRP2 -> runtime path. The next
natural step is to make complex, fast, interactive dashboards easier to build without forcing each
application to reinvent streaming buffers, trace visuals, linked panels, cursors, overlays, and
performance instrumentation.

The streaming DAQ viewer is a concrete pressure test, but the underlying problems are broader:

- electrophysiology and neuroscience acquisition;
- telemetry and industrial monitoring;
- sensor arrays;
- audio and signal processing;
- finance and market data;
- streaming logs and event timelines;
- simulation diagnostics;
- scientific dashboards with many linked panels;
- embedding explorers with large retained point clouds, image thumbnail LOD, semantic labels,
  search, selection, and metadata cards.

This note collects candidate reusable pieces that could eventually move from examples into Datoviz
itself. The guiding principle is to prove the abstractions in standalone examples first, then
extract the pieces that are clearly reusable and stable.


## Design Goals

Dashboard support should prioritize:

- retained objects that are cheap to update;
- persistent GPU resources with bounded lifetime;
- partial resource updates;
- appendable and circular data flows;
- stable frame pacing under continuous updates;
- linked panels and shared cameras;
- overlay layers for cursors, selections, labels, and grid lines;
- clean separation between data generation, data storage, rendering technique, and interaction;
- instrumentation visible from user code;
- C-first implementation for core behavior, with Python convenience once the API is stable.

Dashboards should mostly mutate retained scene state. They should not rebuild scene structure,
pipelines, descriptors, or GPU resources every frame.


## Relationship To The Streaming DAQ Viewer

The streaming DAQ viewer should remain a standalone example and benchmark first. It should not
immediately define the public dashboard API.

The DAQ viewer can test:

- many stacked analog and digital channels;
- continuously appended samples;
- interleaved acquisition layout;
- ring-buffer wraparound;
- scrolling and sweep cursor modes;
- one-draw-call trace rendering paths;
- partial upload behavior;
- draw-call and upload instrumentation;
- GUI controls for technique and workload selection.

The example should be structured so reusable pieces are easy to identify:

```text
synthetic/replay input -> ring buffer -> rendering technique adapter -> Datoviz resources
```

Once the example proves which pieces are general, those pieces can be promoted into the scene,
visual, app, or common layers.


## Candidate Low-Level Primitives

### Dirty Range Tracking

Many dashboard workloads need to update only a small part of a retained resource each frame.

A reusable dirty-range helper could track byte or element ranges, merge adjacent ranges, preserve
wraparound information, and expose the minimal set of uploads required for a frame.

Possible uses:

- streaming traces;
- event rasters;
- scrolling image strips;
- log timelines;
- particle or marker buffers;
- live tables or status panels backed by GPU resources.

Open design questions:

- Should dirty ranges be tracked in logical elements, bytes, or both?
- Should wraparound be represented as one wrapped range or split into two normal ranges?
- Should range merging be automatic or controlled by the caller?
- Where should this live: `src/common`, scene internals, or a public utility module?


### Appendable Ring Resource

A fixed-capacity appendable ring resource would make streaming data flows explicit.

Possible responsibilities:

- own or reference a fixed-size CPU-side ring buffer;
- append new samples or records;
- track write index and wrap count;
- expose dirty ranges for upload;
- optionally own the matching GPU resource;
- optionally support interleaved and channel-major logical addressing.

Sketch:

```c
DvzRingResource* ring = dvz_ring_resource(capacity, stride, flags);
dvz_ring_resource_append(ring, count, data);
dvz_ring_resource_dirty_ranges(ring, &range_count, ranges);
dvz_ring_resource_clear(ring);
```

This should probably start as an internal helper or example-local helper before becoming public.


### Streaming Resource Update API

The scene API should make partial updates a normal user operation, not a backend-specific trick.

Useful concepts:

- update this resource byte range this frame;
- append to this fixed-capacity resource;
- update these two ranges because a ring write wrapped;
- preserve resource identity while changing contents;
- report upload size and update count.

The API should avoid encouraging whole-resource uploads when the logical change is small.


### Frame And Resource Telemetry

Fast dashboards need feedback about what the renderer is doing.

Useful per-frame counters:

- FPS and frame time;
- CPU update time;
- GPU submission time if available;
- uploaded bytes;
- upload count;
- dirty range count;
- draw count;
- vertex count;
- index count;
- pipeline creation count;
- descriptor refresh count;
- transient resource creation/destruction count;
- readback count and latency.

Telemetry should be available to examples and applications, not only debug logs. A simple text
overlay or ImGui panel can be an app-layer convenience, but the counters themselves should be
queryable.


## Candidate Visual Families

### Timeseries Or Trace Visual

A general timeseries visual is the most obvious reusable result from the DAQ work.

Possible capabilities:

- append samples;
- fixed visible history;
- ring-buffer, scrolling, and sweep display modes;
- interleaved and channel-major input layouts;
- per-channel offset, gain, color, visibility, and name;
- stacked or overlaid layout;
- analog continuous lines;
- digital step traces;
- selected-channel highlighting;
- optional event markers;
- optional per-channel reorder/grouping.

Possible C API sketch:

```c
DvzTimeseries* ts = dvz_timeseries(panel, DVZ_TIMESERIES_LAYOUT_INTERLEAVED);
dvz_timeseries_channels(ts, channel_count, names, colors);
dvz_timeseries_capacity(ts, sample_rate, visible_seconds);
dvz_timeseries_mode(ts, DVZ_TIMESERIES_MODE_SWEEP);
dvz_timeseries_append(ts, sample_count, interleaved_samples);
```

Advanced configuration could remain optional:

```c
dvz_timeseries_technique(ts, DVZ_TIMESERIES_TECH_INSTANCED);
dvz_timeseries_channel_gain(ts, channel, gain);
dvz_timeseries_channel_visible(ts, channel, visible);
dvz_timeseries_reorder(ts, order_count, order);
```

The name should probably be `timeseries`, `trace`, or `strip_chart`, not `daq`. DAQ is a domain
example; streaming traces are more general.


### Digital Trace Or Step-Line Mode

Digital signals are not just analog lines with two values. They need horizontal high/low segments,
vertical transitions, pulse widths, and often dense event-like behavior.

This could be:

- a mode of the timeseries visual;
- a separate `digital_trace` visual;
- a specialization built from a segment or quad visual.

The right choice depends on whether the rendering technique is shared with analog traces or needs
a distinct geometry path.


### Event Raster Visual

Sparse events are common in dashboards: spikes, TTL pulses, messages, triggers, dropped frames,
packet events, alarm events, and annotations.

An event raster visual could support:

- per-channel event times;
- event duration or instantaneous markers;
- color by event type;
- vertical lanes;
- selection and picking;
- ring-buffer or time-window retention;
- optional aggregation when events are too dense.


### Envelope Or Min-Max Visual

For high-density signals, drawing every sample can be wasteful and visually misleading. A min/max
envelope visual can preserve spikes and extrema when many samples map to one pixel column.

Possible stages:

1. CPU min/max decimation for simplicity.
2. GPU prepass or compute decimation for large histories.
3. Hybrid path that switches between raw line and envelope depending on zoom.


### Cursor, Marker, And Selection Overlays

Most dashboards need standard overlays:

- vertical time cursor;
- sweep cursor;
- selected time range;
- trigger windows;
- threshold lines;
- row highlights;
- event markers;
- crosshair/ruler overlays.

These may deserve reusable overlay visuals or app-layer widgets. They should be easy to layer above
data without disturbing the main visual's depth, blending, or picking behavior.


### Strip Chart Composition

A strip chart could be a higher-level composition around timeseries data:

- trace visual;
- grid;
- labels;
- cursor;
- fixed visible history;
- append API;
- optional overview/detail panels.

This should probably come after lower-level trace and overlay pieces are proven.


## Rendering Techniques To Compare

The same user-facing visual may support multiple internal techniques. Technique selection can start
as a debug or benchmark control before becoming an advanced public option.

Backend portability matters. The public dashboard semantics should not assume that the native
Vulkan implementation technique is always available. DRP2 should be able to choose a backend
technique from capability tiers, with WebGPU-friendly fallbacks where needed.

### Instanced Line-Strip From Interleaved Data

This is a strong first candidate for DAQ-like layouts where samples arrive as:

```text
t0: ch0 ch1 ch2 ...
t1: ch0 ch1 ch2 ...
t2: ch0 ch1 ch2 ...
```

One draw can render one strip per channel:

```c
vkCmdDraw(cmd, sample_count, channel_count, 0, 0);
```

The shader derives sample and channel indices from `gl_VertexIndex` and `gl_InstanceIndex`, then
addresses:

```text
value_index = physical_sample * channel_count + channel_index
```

Advantages:

- one draw call;
- no CPU-side transposition;
- no index buffer;
- no artificial connectors between channels;
- natural fit for interleaved DAQ buffers.

Limitations:

- requires shader-side ring-buffer addressing;
- digital step traces may need a different expansion path;
- the current scene/DRP2 resource model must expose the right buffer access pattern.

This is also the strongest baseline for WebGPU because it uses ordinary instancing, ordinary
buffers, and one direct draw call. It avoids relying on primitive restart, multi-draw indirect, or
wide-line behavior.


### Indexed Line-Strip With Primitive Restart

Primitive restart is a topology-side way to put multiple strips in one indexed draw by inserting
special restart indices between strips.

Advantages:

- one draw call;
- no fake connector geometry;
- useful baseline for compact strip rendering;
- can address interleaved source data without transposition by choosing appropriate indices.

Limitations:

- requires indexed rendering;
- index buffers can be large;
- ring-buffer wrap may require dynamic indices or shader indirection;
- on MoltenVK/Metal, primitive restart is effectively always enabled, so max-value indices must not
  be used as real sample indices.

WebGPU supports indexed strip primitive restart, but the restart value is fixed by the pipeline's
strip index format: `0xFFFF` for 16-bit indices or `0xFFFFFFFF` for 32-bit indices. This technique
is portable enough to test, but it should remain an implementation option rather than the core
dashboard abstraction.


### Multi-Draw Or Indirect Per-Channel Strips

Draw one strip per channel, ideally through indirect or multi-draw support.

Advantages:

- clean topology;
- no primitive restart;
- no fake connectors;
- natural wrap splitting.

Limitations:

- backend feature support matters;
- regular per-channel draws may or may not be acceptable at stress sizes;
- command generation cost needs measurement.

For WebGPU, direct and indirect draws exist, but core WebGPU does not provide a single multi-draw
command that consumes many draw records from one buffer. A backend may still issue a loop of draw or
indirect draw calls, but portable dashboard performance should not depend on native multi-draw
indirect semantics.


### Shader-Discard Connector Masking

This is a useful legacy-style baseline. A single continuous stream carries a span or channel
attribute. Artificial connector fragments are detected through interpolation and discarded.

Advantages:

- simple non-indexed data path;
- one draw call;
- useful comparison with older approaches.

Limitations:

- connector geometry is still assembled and rasterized;
- long connectors can waste fragment work;
- discard can interact poorly with picking, depth, antialiasing, derivatives, transparency, and
  postprocessing.


### Expanded Segment Or Quad Mesh

Generate explicit segments or quads for each trace.

Advantages:

- best control over linewidth, antialiasing, joins, caps, and digital steps;
- avoids backend-dependent wide-line behavior;
- can support high-quality visual styling.

Limitations:

- more vertices;
- larger uploads;
- CPU expansion may become expensive;
- GPU expansion requires additional infrastructure.

This is likely the most portable high-quality path for WebGPU. Native line topologies are useful
for thin traces, but thick lines, antialiasing, joins, caps, and high-DPI quality should be treated
as explicit geometry expansion rather than relying on backend-specific wide-line support.


### Sampled Buffer Or Texture Path

Keep raw samples in a storage-like buffer or texture-like resource. The shader computes positions
from raw values and ring-buffer metadata.

Advantages:

- preserves raw DAQ layout;
- can move ring addressing to the GPU;
- foundation for GPU filtering and decimation.

Limitations:

- depends on exposed shader/resource capabilities;
- line topology still needs clean strip breaks;
- analog, digital, and event channels may need separate paths.

This is plausible on WebGPU because storage buffers and texture-like resources are first-class, but
buffer usage, binding sizes, and alignment rules are stricter than native Vulkan. The public API
should express the desired trace behavior, while DRP2 handles the backend-specific resource layout.


### GPU Decimation Or Envelope Path

Use compute or a prepass to build per-pixel min/max envelopes, threshold spans, or reduced line
vertices.

Advantages:

- scalable for long histories and high sample rates;
- preserves extrema better than naive subsampling;
- useful for stress dashboards.

Limitations:

- more complex synchronization;
- harder to expose portably;
- should come after simpler paths are benchmarked.

WebGPU includes compute passes and indirect compute dispatch, so this is not fundamentally blocked.
It should still be a later capability tier with a render-only or CPU-decimated fallback.


## Dashboard Composition Helpers

Complex dashboards need more than visuals.

### Panel Grids And Shared Cameras

Users should be able to express:

- these panels share the same time axis;
- these panels share vertical channel navigation;
- this overview panel controls this detail panel;
- this panel has data coordinates, while this overlay is screen-anchored.

This should not require manual callback plumbing in every application.


### Linked Interaction Groups

Useful linked interactions:

- pan/zoom propagation;
- selected time range propagation;
- selected channel propagation;
- cursor propagation;
- synchronized reset/fit operations.

The goal is to make multi-panel dashboards predictable without requiring a large GUI framework.


### Overlay Layers

Dashboards usually need separate conceptual layers:

```text
background bands
grid
data
events
selections
cursors
labels
UI
```

Datoviz should make layer ordering and coordinate space explicit enough that overlays remain stable
through pan, zoom, resize, and high-DPI changes.

The minimal native layout target is tracked in
[`../proposals/SCREEN_SPACE_OVERLAY_LAYOUT.md`](../proposals/SCREEN_SPACE_OVERLAY_LAYOUT.md). That
proposal keeps dashboard cards and readouts inside the scene path without requiring HTML/CSS or a
general GUI framework.


### Stable Object Identity

Dashboard applications repeatedly update specific objects: trace 37, cursor A, selected row, panel
2, event marker group, and so on.

The API should preserve stable handles and predictable ownership. Users should not need to recreate
visuals just to change data, style, visibility, or selection state.


### Declarative Update Batches

Dashboards often update many small pieces every frame. It may be useful to support a batched update
style:

```text
begin dashboard update
append trace samples
move cursor
update labels
change selected channel
end dashboard update
```

This does not need to become a full declarative UI system. The important property is that the scene
can coalesce updates and emit efficient frame plans.


## User API Direction

The common path should be simple and direct:

```c
DvzTimeseries* ts = dvz_timeseries(panel, DVZ_TIMESERIES_LAYOUT_INTERLEAVED);
dvz_timeseries_channels(ts, channel_count, names, colors);
dvz_timeseries_capacity(ts, 1000.0, 10.0);
dvz_timeseries_mode(ts, DVZ_TIMESERIES_MODE_RING);
dvz_timeseries_append(ts, sample_count, interleaved_samples);
```

Advanced controls can remain available without burdening simple examples:

```c
dvz_timeseries_technique(ts, DVZ_TIMESERIES_TECH_INSTANCED);
dvz_timeseries_channel_order(ts, order_count, order);
dvz_timeseries_channel_gain(ts, channel, gain);
dvz_timeseries_channel_visible(ts, channel, visible);
dvz_timeseries_color_mode(ts, DVZ_TIMESERIES_COLOR_BY_GROUP);
```

Python should eventually expose a friendly wrapper:

```python
viewer = dvz.TimeseriesViewer(channels=128, sample_rate=1000, seconds=10)
viewer.append(samples)
```

But the first serious implementation should be C, because the work is mostly about validating the
engine path, resource update model, rendering techniques, and performance instrumentation.


## Extraction Strategy

The recommended order is:

1. Implement the standalone C DAQ dashboard/stress example.
2. Keep the rendering technique adapters private to the example at first.
3. Add telemetry needed to measure the example.
4. Extract a dirty-range or ring-buffer helper if repeated code appears.
5. Promote a minimal timeseries/trace visual after one technique is clearly useful.
6. Add digital/event lane support once analog streaming is stable.
7. Add dashboard composition helpers for shared cameras, cursors, overlays, and linked panels.
8. Add high-density envelope/decimation after the simple path's limits are measured.
9. Expose Python convenience only after the C API and semantics are stable.

This keeps v0.5 dashboard work grounded in measured behavior rather than prematurely committing to
a large public widget API.


## Open Questions

### What belongs in core Datoviz versus examples?

Low-level primitives such as dirty ranges, partial updates, and telemetry are likely broadly useful.
Full dashboard widgets may be too opinionated for core Datoviz.

Suggested direction:

- core: resource update primitives, telemetry, stable scene mutation APIs;
- visuals: timeseries, event raster, envelope, cursor/marker overlays;
- app/examples: DAQ viewer, dashboard presets, GUI panels, domain-specific controls.


### Should rendering technique selection be public?

Technique selection is valuable for benchmarks, but too much public control can freeze internal
implementation details.

Suggested direction:

- examples expose technique selection for investigation;
- public visuals choose a default technique automatically;
- advanced users may request a technique through a hint rather than a hard contract;
- telemetry reports the selected technique.


### How much GUI should Datoviz own?

Dashboards need controls, but Datoviz should avoid becoming a full GUI framework.

Suggested direction:

- provide app-layer helpers for common overlays and debug panels;
- keep ImGui or another GUI integration optional;
- focus core Datoviz on rendering, interaction, and scene updates;
- make examples demonstrate GUI patterns without requiring them in headless or embedded contexts.


### How should WebGPU constraints influence the design?

Some Vulkan techniques may not map cleanly to WebGPU, especially around indirect draws, dynamic
state, storage buffer patterns, or primitive restart assumptions.

Suggested direction:

- keep user semantics backend-neutral;
- benchmark native Vulkan techniques freely in examples;
- design fallback techniques for WebGPU;
- avoid exposing Vulkan-only technique details as mandatory public API.

Known WebGPU considerations:

- buffer usages are explicit and validation is stricter; streaming uploads should be expressed as
  queue writes or staging/copy updates rather than assuming persistent mapped GPU buffers;
- storage buffers are available, but binding sizes, dynamic offsets, and alignment constraints must
  be respected;
- direct and indirect draw calls exist, but portable performance should not depend on native
  multi-draw indirect;
- non-zero `firstInstance` in indirect draw arguments is optional and should not be required for the
  baseline dashboard path;
- indexed strip primitive restart exists, with restart values fixed by `stripIndexFormat`;
- timestamp queries are optional, so telemetry must have CPU-side counters and optional GPU timing;
- thick, antialiased, high-DPI traces should use expanded geometry rather than native wide-line
  assumptions;
- compute-based decimation is plausible but should be an advanced path with simpler fallbacks.

Suggested WebGPU-friendly technique priority:

1. instanced line-strip from interleaved data;
2. expanded segment or quad traces for quality;
3. CPU or GPU min/max envelope;
4. indexed primitive restart as a tested option;
5. indirect or multi-draw style paths only as advanced optimizations.


### Should dashboard support be C-first or Python-first?

Python is the likely user-friendly entry point, but C is better for validating the v0.4/v0.5 engine
architecture.

Suggested direction:

- implement the stress example and reusable primitives in C first;
- add Python bindings after the semantics are stable;
- keep Python examples focused on ease of use rather than engine validation.


### How should time and sample indexing be represented?

Streaming dashboards often mix sample index, wall-clock time, acquisition time, display time, and
ring-buffer physical index.

Suggested direction:

- distinguish logical sample index from physical ring index;
- allow sample-rate based time coordinates by default;
- support explicit timestamps for irregular streams later;
- make cursor and selection APIs operate in a well-defined coordinate space.


### How much should Datoviz know about channel metadata?

Channels need names, colors, gains, visibility, groups, order, and type. But too much metadata can
make the visual too domain-specific.

Suggested direction:

- support a small common metadata model: name, color, type, offset, gain, visible, group id;
- let applications keep richer domain metadata externally;
- provide hooks for labels and legends without owning the whole domain model.


## Suggested Next Discussion Topics

Useful next decisions:

- define the first standalone C DAQ example architecture;
- decide which rendering technique to prototype first;
- list the minimum scene/DRP2 features needed for instanced interleaved traces;
- define a telemetry struct that examples and tests can query;
- decide whether dirty-range tracking should be public, private, or example-local initially;
- sketch the smallest possible `DvzTimeseries` API;
- identify WebGPU compatibility constraints before the API becomes too Vulkan-shaped.
