# Earthquake Aftershock Explorer

> **Example status:** informative pressure test
> **Target:** C showcase plus preparation script
> **Data:** Ridgecrest 2019 prepared cache with synthetic fallback
> **Validation:** smoke, coordinate/depth/replay/selection checklist

See [../SHARED_POLICIES.md](../SHARED_POLICIES.md) for shared worked-example policy.


## Summary

Render an earthquake aftershock sequence as a local 3D crust event catalog with magnitude-scaled
points, depth/time color modes, replay, fault paths, and linked panels as later stages. The default
data should be a prepared Ridgecrest 2019 cache derived offline from USGS ComCat; runtime should not
depend on live network access.


## User-Visible Result

- A local crust panel shows aftershocks below a `z = 0` surface plane.
- Point size encodes magnitude; color encodes depth, elapsed time, magnitude, or distance.
- Replay shows cumulative growth or a sliding time window.
- Optional fault traces, selected-event projection line, and linked time/depth panels provide
  geophysical context.
- Clicking an event reports catalog metadata and highlights the event in all visible panels.


## Feature Pressure Points

- 3D point rendering with semantic size/color encodings.
- Local lon/lat/depth projection into stable kilometer coordinates.
- Time replay through visibility/opacity/filter updates.
- Linked 2D/3D panels sharing selection and replay state.
- Picking in a semantically meaningful sparse point cloud.
- Path, point, surface/image, colorbar, annotation, and camera features in one workflow.


## Required Data And Resources

Default sequence: Ridgecrest, California 2019 aftershocks. It is compact, scientifically
recognizable, fault-aligned, depth-rich, and cacheable. Optional later sequences include 2011 Tohoku,
a smaller teaching dataset, or the synthetic fallback.

Suggested cache:

```text
~/.cache/datoviz/earthquakes/ridgecrest_2019/
  metadata.json
  events_f32.bin
  lonlat_f64.bin
  time_i64.bin
  event_id_u64.bin
  fault_trace_f32.bin
  fault_trace_offsets.bin
```

Prepared event fields:

```text
event_id, utc_time, elapsed_seconds, longitude, latitude, depth_km, magnitude,
x_km, y_km, distance_from_mainshock_km
```

Metadata should record source, USGS query URL, sequence name, mainshock id/time, `latitude0`,
`longitude0`, projection, magnitude minimum, start/end time, cache version, and preparation script.

Synthetic fallback should create one mainshock, intersecting strike-slip planes, Omori-like event
times, Gutenberg-Richter-like magnitudes, dipping-plane depths, and small location noise.


## Coordinate And Visual Model

Default coordinates:

```text
x = east/west kilometers relative to sequence center
y = north/south kilometers relative to sequence center
z = depth kilometers, positive downward
```

Default encodings:

| Quantity | Encoding |
| --- | --- |
| hypocenter | 3D point below surface |
| magnitude | bounded nonlinear point radius |
| depth/time/magnitude/distance | selectable color mode with colorbar |
| mainshock | larger highlighted marker |
| fault trace | path on `z = 0` surface |
| replay age | opacity or visibility |

First radius mapping may be linear with clamping; later versions may use an energy-like
`10^(0.3 * M)` mapping with strong bounds.


## Scene Layout

Minimum viable layout:

```text
3D local crust panel: surface plane, mainshock, fault paths, aftershock cloud
```

Preferred layout:

```text
3D local crust panel
magnitude vs elapsed time
depth vs elapsed time or cumulative count
```

Controls should cover sequence, 3D/top/cross-section view, color mode, cumulative/sliding replay,
play/time/speed/window, magnitude/depth filters, faults/surface/labels toggles, reset camera, and
optional clip export.


## Cross-Section And Replay

Cross-section mode is preferred but not required for the first slice. It projects events onto a
fault-normal/fault-parallel or user-selected profile:

```text
x = distance along profile
y = depth_km
```

Replay modes:

- cumulative: all events up to current time visible;
- sliding window: only recent events bright or visible.

Offline replay should be deterministic enough for video export.


## Interactivity And Readout

Picking should identify the nearest visible event in the active panel. Readout should include event
id, UTC time, elapsed time, magnitude, depth, latitude/longitude, local x/y, and distance from the
mainshock. Selection highlights the event in the crust panel and any magnitude-time, depth-time,
cumulative-count, or cross-section panel.


## Scene And Runtime Behavior

Use the normal scene pipeline; see [../../pipeline/FRAME_PLAN.md](../../pipeline/FRAME_PLAN.md) and
[../../drp2/](../../../drp2/).

- Initial setup uploads event positions/colors/sizes, surface plane/image, fault paths, and linked
  panel data.
- Replay updates event visibility/opacity/active subset and linked time cursor.
- Selection updates highlight marker, vertical projection line, and linked panel markers.
- Magnitude/depth filters may rebuild active subsets but should not rebuild static catalog data.


## Minimal Implementation Target

1. Load or generate prepared event arrays.
2. Normalize local kilometer coordinates into stable scene scale.
3. Render points below a simple surface plane in an oblique arcball view.
4. Update visibility or opacity from a replay cursor.
5. Keep linked panels minimal but synchronized once added.
6. Add event picking through the current scene request path when available.


## Validation

- Smoke run loads or generates events, renders nonblank crust view, advances replay, and tears down.
- Coordinate projection and depth-positive-down orientation are visually checked.
- Point size/color match magnitude/depth/time modes and colorbar labels.
- Replay is deterministic and does not rebuild static event data.
- Selection highlight and readout remain stable after filters/replay changes.
