# Streaming DAQ Viewer Example Specification

> **Agent Pickup**
> - **Category:** `dashboards`
> - **Implementation target:** Dense multi-panel example with predictable controls and a synthetic or cached data mode.
> - **Data policy:** Synthetic fallback is mandatory; public or local replay data may be optional.
> - **Preprocessing:** Use a deterministic Python preparation script when real data is downloaded or converted.
> - **Validation:** Bounded smoke run plus manual interaction checklist for linked panels, picking, and updates.


## Summary

This example demonstrates a real-time, high-density data acquisition (DAQ) viewer for Datoviz v0.4. It shows many digital and analog time series updating continuously in a fixed-size ring buffer, with a moving write cursor and optional scrolling display mode.

The example should be implemented in Python and should work out of the box. It should not depend on external data by default. It should generate deterministic, realistic synthetic DAQ data that resembles a neuroscience or behavioral acquisition session. If a prepared replay dataset is later added to the `datoviz/data` repository, the example may optionally download and use it, but synthetic data must remain the default and fallback.

The purpose is to stress the v0.4 scene API and rendering architecture in a realistic, update-heavy visualization workload:

- many stacked time series;
- frequent partial data updates;
- ring-buffer wraparound;
- dynamic GPU resources;
- 2D camera interaction;
- overlays, labels, cursor, and event markers;
- stable frame rate without recreating resources every frame.

The exact Datoviz v0.4 Python API is not yet fixed, so this document describes required behavior and data flow without relying on specific function names.

## Example name

Suggested filename:

```text
STREAMING_DAQ_VIEWER.md
```

Suggested Python example name:

```text
streaming_daq_viewer.py
```

Suggested gallery title:

```text
Streaming DAQ Viewer
```

## User-facing behavior

The application opens a Datoviz window showing a dense DAQ display with many stacked channels.

By default:

- 128 digital channels are shown;
- 16 analog channels are shown;
- the logical sample rate is 1 kHz;
- the visible history is 10 seconds;
- the visual update rate is tied to the render loop, typically 30-120 Hz;
- samples are generated in chunks at every frame;
- new samples overwrite old samples in a circular buffer;
- a vertical write cursor indicates the current write position.

The viewer should look like a high-density oscilloscope / logic analyzer / acquisition dashboard.

Digital channels are drawn as step traces. Analog channels are drawn as continuous traces. Rows should be stacked vertically, with optional row bands, channel labels, event markers, and a subtle grid.

The display should remain readable and visually attractive on a laptop screen.

## Main visualization modes

The example should support at least the first mode. The second mode is optional but recommended.

### 1. Ring-buffer mode

The x-axis represents buffer index or time modulo the ring-buffer duration.

A write cursor moves from left to right. When it reaches the end of the visible buffer, it wraps back to the beginning. New samples overwrite old samples in place.

This mode is the main architectural test because it avoids shifting the entire dataset every frame. Only the newly written buffer range should be marked dirty and uploaded.

Important behavior:

- the cursor should advance smoothly;
- wraparound should be visually correct;
- the trace should not tear or connect incorrectly across the wrap boundary;
- optional fading or background shading may indicate which samples are newest.

### 2. Scrolling oscilloscope mode

The x-axis represents recent time, for example the last 10 seconds. New samples enter from the right and old samples disappear on the left.

This mode is visually familiar but may require camera or transform updates every frame. It is optional and can be implemented after ring-buffer mode.

## Synthetic data model

The data should be deterministic and realistic, not pure random noise.

Use a fixed seed random generator so that screenshots and tests are reproducible.

Recommended defaults:

```text
n_digital_channels = 128
n_analog_channels  = 16
sample_rate_hz     = 1000
visible_seconds    = 10
n_samples          = sample_rate_hz * visible_seconds
chunk_size         = 16-128 samples per rendered frame, depending on dt
```

The implementation should maintain separate logical arrays for digital and analog signals, or one unified array with per-channel type metadata.

Suggested CPU-side storage:

```text
values         float32[n_channels, n_samples]
channel_types  enum/string[n_channels]  # digital or analog
channel_names  string[n_channels]
colors         float32[n_channels, 4]
row_offsets    float32[n_channels]
write_index    int
sample_counter int64
```

Digital values may be stored as `uint8` internally, but the renderable buffer may use `float32` for simplicity.

## Simulated channel classes

The synthetic generator should create recognizable acquisition patterns.

### Digital TTL channels

Examples:

- trial start;
- trial end;
- stimulus on/off;
- visual frame sync;
- camera frame sync;
- laser pulse;
- reward valve;
- lick detector;
- wheel encoder phase A;
- wheel encoder phase B;
- audio trigger;
- photodiode threshold;
- synchronization pulse;
- dropped-frame indicator.

Digital channels should be binary and drawn as step plots.

### Sparse event channels

Some channels should show sparse events:

- Poisson-like spikes;
- bursts;
- refractory-period spike trains;
- packet-like event groups;
- occasional synchronized pulses across several channels.

These are useful for testing dense vertical transitions and channel-level patterns.

### Analog channels

Analog traces should include a mix of:

- noisy sine waves;
- slow drift;
- random walk;
- wheel velocity-like signal;
- photometry-like signal;
- breathing-like oscillation;
- ECG-like sharp pulses;
- transient artifacts;
- clipped or saturated intervals.

Analog traces should be normalized per channel and vertically scaled to fit within their row.

### Structured trial timeline

The generator should include a repeated trial structure. For example:

```text
trial_start        short pulse
stimulus           high for 0.5-2.0 seconds
photodiode         delayed copy of stimulus with jitter
response_window    high after stimulus onset
lick_detector      sparse events during response window
reward             short pulse after successful response
camera_sync        regular pulses at 30/60 Hz
```

The timeline should make the display visually meaningful and easier to debug.

## Optional downloaded replay data

External data are not required. If a small prepared dataset is later added to `datoviz/data`, the example may optionally use it.

Suggested file:

```text
datoviz/data/streaming_daq_demo.npz
```

Suggested contents:

```text
digital        uint8[n_digital_channels, n_samples]
analog         float32[n_analog_channels, n_samples]
channel_names  string[n_channels]
channel_types  string[n_channels]
sample_rate    float32 scalar
events         structured event table, optional
```

The example should:

1. look for the file in the local Datoviz cache;
2. if missing and replay mode is requested, download it from `datoviz/data`;
3. if unavailable, fall back to procedural synthetic generation;
4. never fail just because the replay file is missing.

Default behavior should be synthetic generation, not download.

## Rendering design

The implementation should choose a rendering strategy that matches the available v0.4 API. The exact visual classes are not prescribed.

### Required visuals

The scene should contain:

1. one or more line/segment visuals for digital step traces;
2. one or more line visuals for analog traces;
3. an overlay cursor visual;
4. optional grid lines;
5. optional row background bands;
6. optional text labels;
7. optional event marker overlays.

### Digital traces

Digital traces should be rendered as step plots.

Each channel row has a low and high y coordinate:

```text
y_low  = row_y - digital_amplitude
y_high = row_y + digital_amplitude
```

Transitions should include vertical segments so the digital signal is visually correct.

Implementation options:

- expand each digital sample into CPU-generated line vertices;
- generate one vertex per sample and rely on a shader or line-strip topology if supported;
- use a specialized step-line visual if available;
- use instanced quads for high/low spans if that is more efficient.

For the first implementation, CPU-generated line vertices are acceptable, provided resources are not recreated every frame.

### Analog traces

Analog channels should be continuous line strips.

Each analog sample is mapped to:

```text
x = sample_index_or_time
y = row_offset + analog_gain * normalized_value
```

The analog signal should not visually overlap neighboring rows by default.

### Ring-buffer split

In ring-buffer mode, the trace may need to be drawn as two ranges:

```text
[0, write_index)
[write_index, n_samples)
```

This avoids drawing a spurious line segment across the wrap boundary.

The renderer may handle this by:

- issuing two draw ranges;
- using two visuals per trace group;
- inserting degenerate break markers if supported;
- using NaN breaks if supported by the line visual;
- using an index buffer that excludes the wrap segment.

The specification requires correct visual behavior, not a particular implementation.

## Resource update requirements

This example is primarily a dynamic resource test.

The implementation should avoid:

- recreating visuals every frame;
- recreating GPU buffers every frame;
- uploading the entire dataset every frame when only a small chunk changed;
- allocating unbounded temporary arrays;
- blocking GPU/CPU synchronization in the update loop.

The expected model is:

1. allocate CPU-side arrays and corresponding scene resources once;
2. at each frame, generate a chunk of new samples;
3. write those samples into the ring buffer;
4. update only the dirty region of the renderable buffers if the API supports partial updates;
5. otherwise update the smallest practical resource subset;
6. update cursor and optional labels;
7. render the scene.

If the current API only supports whole-resource updates, the example should still be structured so that replacing this with dirty-range updates later is straightforward.

## Camera and layout

Use a 2D camera.

The default view should show all channels vertically and the full ring buffer horizontally.

Suggested coordinate system:

```text
x in [0, visible_seconds]
y in [0, n_channels]
```

or:

```text
x in [0, n_samples]
y in [0, n_channels]
```

The camera should support:

- horizontal zoom into a shorter time range;
- vertical zoom into fewer channels;
- pan in both directions;
- reset view;
- optional lock to full time range;
- optional lock to full channel range.

The y-axis should place channel 0 at the top or bottom consistently. The choice does not matter, but labels and interaction must match it.

## Overlays

The example should include at least a write cursor overlay.

Recommended overlays:

- vertical write cursor;
- grid lines every fixed time interval;
- row separators;
- alternating row bands;
- trigger/event markers;
- selected channel highlight;
- text labels on the left;
- current value labels on the right.

Overlays should be rendered in screen or panel space where appropriate, and should not interfere with the main trace rendering.

## Interaction and UI controls

Use Datoviz v0.4 interaction mechanisms and/or ImGui controls if available.

Recommended controls:

```text
Space          pause/resume
R              reset buffer
C              clear traces
F              fit all channels and time range
M              switch ring-buffer / scrolling mode, if both are implemented
Mouse wheel    zoom
Drag           pan
Click row      select channel
```

Recommended ImGui controls:

- pause/resume checkbox;
- number of visible digital channels;
- number of visible analog channels;
- logical sample rate;
- update chunk size;
- vertical gain;
- time window duration;
- show/hide labels;
- show/hide grid;
- show/hide event markers;
- ring-buffer versus scrolling mode;
- synthetic generator preset;
- replay dataset mode, optional.

The UI should remain optional. The example should still run if ImGui is unavailable.

## Performance targets

The example should be parameterized so that it can run on modest hardware and also stress high-end GPUs.

Suggested presets:

### Light

```text
64 digital channels
8 analog channels
5 seconds visible
1 kHz sample rate
```

### Default

```text
128 digital channels
16 analog channels
10 seconds visible
1 kHz sample rate
```

### Stress

```text
512 digital channels
64 analog channels
20 seconds visible
2-10 kHz logical sample rate
```

The default preset should be smooth on a typical development machine.

The stress preset may be enabled through a command-line flag or UI control.

## Visual quality requirements

The default scene should be visually clean:

- dark background;
- thin but readable traces;
- muted row grid;
- brighter selected channel or active event lines;
- no excessive color saturation;
- no flickering at the cursor;
- labels readable when enabled;
- high-DPI friendly layout where possible.

The example should still be useful if text rendering is not yet ready in v0.4. In that case, labels can be omitted or replaced by simple tick-like marks.

## Testing and validation

The example should help validate the following behavior:

- stable frame rate during continuous streaming;
- no unbounded memory growth;
- correct ring-buffer wraparound;
- no unwanted line segment across the wrap boundary;
- partial buffer updates or at least bounded update cost;
- correct redraw after resize;
- correct camera pan/zoom behavior;
- cursor remains aligned with the write index;
- pause/resume does not corrupt the buffer;
- reset/clear correctly resets state;
- screenshot capture works;
- optional video capture works if Datoviz video output is available.

## Suggested implementation outline

The implementation agent may follow this structure.

### 1. Parse parameters

Use simple defaults and optional command-line arguments:

```text
--channels-digital
--channels-analog
--sample-rate
--seconds
--preset light|default|stress
--mode ring|scroll
--seed
--replay optional_path_or_url
```

### 2. Initialize synthetic generator

Create deterministic channel metadata, trial schedule, and random generator state.

### 3. Allocate CPU arrays

Allocate ring-buffer arrays for raw values and renderable vertices.

### 4. Create Datoviz scene

Create:

- window/canvas;
- scene;
- one panel;
- 2D camera;
- line/segment visuals;
- overlay visuals;
- optional UI.

### 5. Create persistent resources

Create resources for:

- digital trace vertices;
- analog trace vertices;
- colors;
- row positions;
- cursor line;
- grid lines;
- optional labels/events.

Do this once at startup.

### 6. Frame update

At each frame:

1. compute elapsed time;
2. determine how many logical samples to generate;
3. generate sample chunk;
4. write into ring buffer;
5. update renderable vertex buffers for dirty ranges;
6. move cursor;
7. update optional statistics/UI;
8. ask the scene to render.

### 7. Cleanup

Release Datoviz resources normally.

## Pseudocode

This pseudocode intentionally avoids committing to exact v0.4 API names.

```python
import numpy as np

rng = np.random.default_rng(seed)

cfg = Config(
    n_digital=128,
    n_analog=16,
    sample_rate=1000,
    visible_seconds=10,
    mode="ring",
)

n_channels = cfg.n_digital + cfg.n_analog
n_samples = int(cfg.sample_rate * cfg.visible_seconds)

state = DaqState(
    values=np.zeros((n_channels, n_samples), dtype=np.float32),
    write_index=0,
    sample_counter=0,
)

generator = SyntheticDaqGenerator(cfg, rng)

app = create_datoviz_app()
scene = app.create_scene()
panel = scene.create_panel()
camera = panel.create_2d_camera()

# Create persistent visuals/resources.
digital_visual = panel.create_line_or_segment_visual(...)
analog_visual = panel.create_line_visual(...)
cursor_visual = panel.create_overlay_line(...)
grid_visual = panel.create_overlay_grid(...)

resources = create_trace_resources(scene, cfg, state)
bind_resources_to_visuals(resources, digital_visual, analog_visual, cursor_visual)

paused = False
last_time = now()

def on_frame(dt):
    if not paused:
        n_new = compute_sample_count(dt, cfg.sample_rate)
        chunk = generator.next(n_new, state.sample_counter)

        start = state.write_index
        end = (start + n_new) % n_samples

        write_chunk_into_ring_buffer(state.values, chunk, start, end)
        update_render_vertices_for_dirty_range(resources, state, start, n_new)

        state.write_index = end
        state.sample_counter += n_new

    update_cursor_resource(cursor_visual, state.write_index)
    update_camera_if_needed(camera)
    scene.update(dt)
    scene.render()

app.run(on_frame=on_frame)
```

## Notes for the v0.4 architecture

This example should be treated as a pressure test for the scene/resource model.

The implementation should make it easy to observe whether the v0.4 API supports:

- persistent resources;
- dirty resource tracking;
- partial buffer updates;
- multiple visuals sharing common metadata;
- overlay stages;
- 2D camera control;
- event routing;
- optional text rendering;
- efficient line rendering;
- stable rendering after frequent updates.

If the first implementation must upload full buffers every frame, this limitation should be documented in a comment, and the code should isolate the upload path so that dirty-range updates can replace it later.

## Acceptance criteria

The example is complete when:

1. it runs without external files;
2. it opens an interactive Datoviz window;
3. it shows many stacked digital and analog traces;
4. traces update continuously;
5. the write cursor advances and wraps correctly;
6. the display remains stable after multiple wraparounds;
7. pause/resume works;
8. reset/clear works;
9. camera pan/zoom or equivalent navigation works;
10. the implementation does not recreate all visuals every frame;
11. the code is clear enough to serve as an architectural reference for future streaming examples.

## Possible future extensions

- live socket or ZeroMQ stream input;
- Lab Streaming Layer input;
- NI-DAQmx mock adapter;
- binary replay format;
- GPU-side sample-to-line expansion;
- compute-shader filtering or thresholding;
- trigger-based view mode;
- channel grouping and collapsing;
- event search;
- linked raster/spike view;
- video export of the streaming display;
- WebGPU browser version using the same scene/DRP architecture.
