# Physiology Signal Workbench

## Summary

Example name: `PHYSIOLOGY_SIGNAL_WORKBENCH`

Build an offline biomedical signal-analysis scene centered on ECG inspection. The example renders
linked time-series panels, annotation markers, derived analysis views, and probe readouts. Stage 1
uses a compact prepared ECG cache, with a deterministic annotation-aware synthetic fallback. EEG
and EMG are later modes that reuse the same workbench structure.

This differs from a live DAQ viewer: the goal is offline, annotation-aware analysis of recorded
clinical or laboratory signals, not ring-buffer acquisition.

## Feature Pressure

- Dense 2D path rendering for long multichannel signals.
- Shared X-axis panzoom across linked panels.
- Independent per-panel Y gain and row offsets.
- Annotation markers, labels, selection, and linked highlights.
- Exact sample probe and event readout.
- Level-of-detail switching between envelope and raw samples.
- Derived panels such as RR intervals, beat morphology, or spectrograms.

## Data And Resources

Stage 1 should load a local ECG cache derived from a standard PhysioNet-style dataset, preferably
MIT-BIH Arrhythmia Database or PTB-XL.

```text
~/.cache/datoviz/physiology/ecg_mitbih_100/
  metadata.json
  signals_f32.bin             # n_channels x n_samples
  time_f64.bin                # optional; sample_rate may be enough
  channel_names.txt
  channel_units.txt           # optional
  annotations_i64.bin         # sample indices
  annotation_type_u16.bin     # class ids
  annotation_label.txt        # optional label table
  lod_min_f32.bin             # optional envelope
  lod_max_f32.bin             # optional envelope
  rr_interval_f32.bin         # optional derived panel
  rr_sample_i64.bin           # optional sample index per RR interval
```

`metadata.json` records source, record name, sample rate, dimensions, duration, channel names,
units, annotation classes, cache layout, and license/citation metadata.

The preparation script may download/load WFDB data, choose an excerpt, normalize units, compute
LOD envelopes, derive RR intervals, and export the cache. Runtime reads only local arrays.

Synthetic fallback must be deterministic and annotation-aware: ECG-like channels, QRS complexes,
beat-to-beat variability, premature/missed beats, baseline wander, artifacts, and beat labels.

## Scene And Runtime Behavior

Recommended layout:

```text
+--------------------------------------------------------------+
| overview timeline with selected visible range                 |
+--------------------------------------------------------------+
| stacked ECG traces, shared time axis                          |
+--------------------------------------------------------------+
| derived panel: RR intervals or beat morphology                |
+--------------------------------------------------------------+
| annotation raster or marker overlays                          |
+--------------------------------------------------------------+
```

Encodings:

| Panel | Data |
|---|---|
| Stacked traces | time vs normalized amplitude plus row offset; channel color/name |
| Annotation raster | event time vs class/channel row; class color/shape |
| Derived ECG panel | beat time/index vs RR interval, heart rate, or beat waveform |
| Probe | time, sample index, channel, amplitude/unit, nearest annotation |

Zoomed-out views may use min/max envelope LOD and hidden/aggregated labels. Zoomed-in views draw
raw samples, labels, beat markers, and exact probe values. LOD resources may be precomputed or
computed at load time for small excerpts.

Controls should cover dataset, view mode, time range, gain, spacing, filter, color mode,
annotations, LOD mode, play cursor, speed, reset view, and capture.

Picking supports:

- sample probe: pointer X -> time/sample, nearest channel row, exact value, crosshair guides;
- annotation pick: nearest marker, label/sample index, highlight in linked panels, optional center.

Latest-request-wins behavior applies when hover probes use asynchronous readback.

Frame-plan shape:

- static setup uploads visible trace/envelope geometry, overview, annotations, and derived panels;
- pan/zoom updates visible trace geometry only when the subset or LOD changes;
- small pans inside a prepared envelope may only update panel transforms;
- selection/probe updates overlay and highlight resources, not full signals;
- gain/filter changes distinguish cheap style updates from regenerated trace geometry.

Expected DRP2 categories: path buffers, envelope/annotation/derived buffers, point or primitive
draws for markers, optional image draws for spectrograms, linked panzoom updates, pick/readback
requests, and optional app/canvas capture.

## Minimal Target

1. Load or synthesize one ECG excerpt.
2. Render 2-12 stacked ECG channels.
3. Draw beat annotations as markers or vertical lines.
4. Provide overview-to-detail linked time navigation.
5. Provide hover/click sample or event readout.
6. Use LOD envelopes for the overview or zoomed-out trace view.

## Validation

- Smoke-run the example with a bounded frame count.
- Confirm ECG traces, channel labels, and annotations align in time.
- Confirm selected beats highlight consistently across all panels.
- Confirm probe values match the active raw or LOD representation.
- Confirm LOD transitions do not change semantic time/sample readout.
- Confirm selection-only changes update overlays without full signal reupload.
- Report loaded sample count, visible sample count, and frame rate while panning/zooming.

Later EEG mode should add many-channel traces, montage grouping, interval events, spectrograms,
and artifact annotations. Later EMG mode should add high-frequency raw traces, rectified/envelope
views, activation windows, and optional high-density channel grids.
