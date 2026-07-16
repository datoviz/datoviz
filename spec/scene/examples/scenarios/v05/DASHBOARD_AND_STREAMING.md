# v0.5 Dashboard And Streaming Scenarios

> **Example status:** v0.5 planning bundle
> **Target:** C examples where they prove engine behavior, GSP/VisPy2 examples for Python UX
> **Data:** synthetic first, prepared public caches when needed
> **Validation:** smoke, interaction checklist, update/resource stability, and screenshot capture


## `streaming_signal_workbench`

Merged track for the old physiology signal workbench and streaming DAQ viewer. It should prove
dense traces, sustained updates, linked X panzoom, overlays, and readable axes.

The v0.4 experimental baseline now lives in
[`examples/c/showcases/streaming_daq.c`](../../../../../examples/c/showcases/streaming_daq.c). It
keeps the acquisition source, bounded SPSC queue, display ring, raw line-list technique, partial
uploads, synchronization telemetry, and native GUI entirely example-local. It proves the engine
path without making ring buffers or time-series helpers public API.

Needed before full version: ring-buffer semantics, discontinuity handling at wrap, many-trace
layout/stacking helpers, shared cursor/selection semantics, and reusable update/telemetry policy.


## `toy_dicom_viewer`

Medical/scientific volume workbench over shared 3D textures.

Needed before full version: oriented slice shader, shared 3D sampled-field binding across panels,
window/level uniforms, crosshair overlays, slice dragging, 3D volume view, and value probes.


## `market_microstructure`

Dense operational dashboard for bars/candles, order-book heatmaps, trades, crosshair, tooltips, and
streaming replay.

Needed before full version: bars/candles visual or primitive convention, visible-range/LOD
aggregation, linked panels, heatmap-cell/trade picking, and streaming replay policy.


## `embedding_explorers`

Merged track for image embedding LOD, semantic embedding atlas, and their shared data-model note.

Needed before full version: preprocessing bundle convention, thumbnail/image sprite LOD,
large-point rendering, label LOD, search/query sidecar, selected-card overlays, and clear ownership
between Datoviz rendering proof and GSP/VisPy2 Python workflows.
