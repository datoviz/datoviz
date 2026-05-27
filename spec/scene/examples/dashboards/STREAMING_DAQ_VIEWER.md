# Streaming DAQ Viewer Example Specification

> **Example status:** informative pressure test
> **Target:** Python dashboard example
> **Data:** deterministic synthetic stream by default, optional cached replay
> **Validation:** bounded smoke run plus linked-panel, wraparound, and update checks

## Summary

Build a dense real-time data acquisition viewer with many digital and analog time series updating
through a fixed-size ring buffer. The example should work without external data and use a
deterministic neuroscience/behavior-style generator by default. Optional replay data may be loaded
from `datoviz/data`, but missing replay data must never prevent startup.

The primary pressure is dynamic retained resources: frequent partial updates, ring-buffer
wraparound, many line/segment traces, overlays, and stable frame rate without recreating visuals.

## User-Visible Result

- Dense oscilloscope / logic-analyzer display with stacked rows.
- Default: 128 digital channels, 16 analog channels, 1 kHz logical sample rate, 10 seconds visible.
- New samples generated in frame-sized chunks; a vertical write cursor shows the current write
  position.
- Digital channels render as step traces; analog channels render as continuous traces.
- Optional row bands, grid, channel labels, event markers, selected-channel highlight, and
  performance readouts.
- Ring-buffer mode is required; scrolling and sweep-cursor modes are optional after the ring path is
  stable.

## Feature Pressure Points

- Persistent GPU resources updated with dirty ranges.
- Correct line topology across ring-buffer wrap, with no spurious connector segment.
- Interleaved DAQ layout support without mandatory CPU transposition.
- Multiple trace rendering strategies behind one data generator.
- Overlay stage for cursor, grid, events, labels, and selection.
- 2D camera pan/zoom with dense geometry.
- Runtime statistics for FPS, generated samples, uploaded bytes, dirty ranges, draw calls, vertices,
  indices, active technique, and write index.

## Required Data And Resources

Synthetic defaults:

```text
n_digital_channels = 128
n_analog_channels  = 16
sample_rate_hz     = 1000
visible_seconds    = 10
chunk_size         = 16-128 samples per frame
seed               = fixed
```

Recommended state:

```text
values float32[n_channels, n_samples]
interleaved_values float32[n_samples, n_channels], optional but important
channel_types digital|analog
channel_names string[n_channels]
colors float32[n_channels, 4]
row_offsets float32[n_channels]
write_index int
sample_counter int64
```

Synthetic channel classes:

- Digital TTL: trial start/end, stimulus, frame sync, camera sync, laser pulse, reward, lick,
  wheel encoder, photodiode, dropped-frame indicator.
- Sparse events: Poisson-like spikes, bursts, refractory spike trains, packet events, synchronized
  pulses.
- Analog: noisy sine waves, drift, random walk, wheel velocity, photometry, breathing, ECG-like
  pulses, artifacts, clipped intervals.
- Structured trials: stimulus, delayed photodiode, response window, licks, reward, and regular
  camera sync.

Optional replay cache:

```text
datoviz/data/streaming_daq_demo.npz
digital uint8[n_digital_channels, n_samples]
analog float32[n_analog_channels, n_samples]
channel_names, channel_types
sample_rate float32
events optional table
```

## Scene Shape And Runtime Behavior

Scene shape:

```text
Window
└── One 2D panel
    ├── Digital step traces
    ├── Analog line traces
    ├── Write cursor overlay
    ├── Optional grid/row/event overlays
    └── Optional text or ImGui statistics
```

Required ring-buffer behavior:

- Advance the write cursor smoothly.
- Write only new logical samples into the circular buffer.
- Split draw ranges or use topology breaks at the wrap boundary.
- Keep old samples visible until overwritten.
- Pause/resume and reset/clear without corrupting state.

Rendering techniques to keep comparable:

| Technique | Role |
|---|---|
| Instanced line strip from interleaved data | Preferred first high-throughput probe |
| Indexed line strip with primitive restart | Compact one-draw baseline |
| Multi-draw or indirect per-channel strips | Clean topology and natural wrap splits |
| Shader-discard connector masking | Legacy-style benchmark baseline |
| Expanded segment/quad mesh | Highest quality steps, joins, caps, and thick traces |
| Sampled buffer/texture path | Raw DAQ layout and GPU addressing |
| GPU decimation/envelope path | Future stress and long-history path |

Controls:

```text
Space pause/resume
R reset buffer
C clear traces
F fit view
Mouse wheel zoom
Drag pan
Click row select channel
```

GUI controls should cover channel counts, sample rate, chunk size, gain, visible duration,
ring-buffer capacity, active technique, labels/grid/events, generator preset, memory layout,
stress preset, and optional replay mode.

## Minimal Implementation Target

- Synthetic generator only.
- Ring-buffer mode only.
- 128 digital and 16 analog channels at 1 kHz for 10 seconds.
- Persistent resources for traces and cursor.
- Dirty update of the newly written range, or the smallest practical whole-resource fallback.
- Readable digital step traces and analog lines.
- Correct wrap behavior over multiple cycles.
- Basic 2D camera or fit controls.

## Validation / Acceptance Criteria

- Runs without external files.
- Opens an interactive Datoviz window with many stacked traces.
- Traces update continuously; cursor advances and wraps correctly.
- No line connects incorrectly across the wrap boundary.
- Pause/resume, reset/clear, pan/zoom, and resize remain stable.
- Visuals and large buffers are not recreated every frame.
- Bounded update cost is visible through statistics or instrumentation.
- Stress preset can increase channel count/history without unbounded memory growth.

## Links

- [Shared example policies](../POLICIES.md)
- [Dashboard rendering roadmap](../../dashboards/DASHBOARD_RENDERING_ROADMAP.md)
- [Frame plan](../../pipeline/FRAME_PLAN.md)
- [Resource model](../../pipeline/RESOURCE_MODEL.md)
- [Invalidation and caching](../../pipeline/INVALIDATION_AND_CACHING.md)
- [DRP2 specs](../../../drp2/)
